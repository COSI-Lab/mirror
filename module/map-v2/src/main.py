import random
import time
import asyncio
from contextlib import asynccontextmanager
import traceback

import aiohttp
from broadcaster import Broadcast
from fastapi import FastAPI, WebSocket
import geoip2.database
from cachetools import TTLCache

#broadcast = Broadcast("redis://localhost:6379")
broadcast = Broadcast('memory://')

LOKI_HOSTNAME = "localhost:1234"

ip_database = geoip2.database.Reader('./GeoLite2-City_20260324/GeoLite2-City.mmdb')
ip_cache = TTLCache(maxsize=-1, ttl=5*60)

def lookup_ip(ip_addr):
    if ip_cache.get(ip_addr) is not None:
        return None, None
    
    try:
        match = ip_database.city(ip_addr)
        return match.location.latitude, match.location.longitude
    except geoip2.errors.AddressNotFoundError:
        return None, None


async def loki_lookup(session, start, end):
    url = LOKI_HOSTNAME + "/loki/api/v1/query_range"
    params = {
        "query": "",
        "start": str(start),
        "end": str(end)
    }
    try:
        async with session.get(url, params=params) as response:
            response.raise_for_status()
            data = await response.json()
            return data
    except aiohttp.ClientError as e:
        print(f"Error fetching data from {url}: {e}")
        return []
    except asyncio.TimeoutError:
        print(f"Request to {url} timed out")
        return []


async def data_thread_task():
    async with aiohttp.ClientSession() as session:
        last_updated = time.time()
        while True:
            try:
                await asyncio.sleep(0.5)

                update_time = time.time()
                data = await loki_lookup(session, last_updated, update_time)
                last_updated = update_time

                for ip_addr, proj in data:
                    lat, lon = lookup_ip(ip_addr)
                    if lat is None:
                        print(f"couldn't look up: {ip_addr}")
                        continue
                    msg = "\n".join([proj, str(lat), str(lon)])
                    await broadcast.publish(channel="data", message=msg)

            except Exception as e:
                print("data thread exception:")
                print(e)
                traceback.print_exc()


@asynccontextmanager
async def lifespan(app: FastAPI):
    await broadcast.connect()
    task = asyncio.create_task(data_thread_task())
    yield
    task.cancel()
    await broadcast.disconnect()
    
app = FastAPI(lifespan=lifespan)


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()

    async with broadcast.subscribe(channel="data") as subscriber:
        async for event in subscriber:
            await websocket.send_text(event.message)

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8080, workers=1)
