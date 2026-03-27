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
QUERY_ENDPOINT = LOKI_HOSTNAME + "/loki/api/v1/query_range"
PROXY_QUERY = "{container=\"proxy\"} | json | __error__=`` | line_format `{{.request}} {{.remote_addr}}` | regexp `^GET /(?P<project>[^ /]*).*?(?P<ip>[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})` | keep project, ip"
RSYNC_QUERY = "{container=\"rsyncd\"} |= `rsync on` | regexp `^.*? rsync on (?P<module>[^ /]*).*? from .* \((?P<ip>.*?)\)` | keep module,ip"

ip_database = geoip2.database.Reader('./GeoLite2-City_20260324/GeoLite2-City.mmdb')
ip_cache = TTLCache(maxsize=-1, ttl=5*60)

def lookup_ip(ip_addr):
    if ip_cache.get(ip_addr) is not None:
        return None, None
    ip_cache.update(ip_addr)
    
    try:
        match = ip_database.city(ip_addr)
        return match.location.latitude, match.location.longitude
    except geoip2.errors.AddressNotFoundError:
        return None, None


async def get_loki_query(session, query, start, end):
    params = {
        "query": query,
        "start": str(start),
        "end": str(end)
    }
    try:
        async with session.get(QUERY_ENDPOINT, params=params) as response:
            response.raise_for_status()
            data = await response.json()
            return data
    except aiohttp.ClientError as e:
        print(f"Error fetching data from {QUERY_ENDPOINT}: {e}")
        return []
    except asyncio.TimeoutError:
        print(f"Request to {QUERY_ENDPOINT} timed out")
        return []


async def data_thread_task():
    async with aiohttp.ClientSession() as session:
        last_updated = time.time()
        while True:
            try:
                await asyncio.sleep(0.5)

                update_time = time.time()
                queries = [
                    get_loki_query(session, RSYNC_QUERY, last_updated, update_time),
                    get_loki_query(session, PROXY_QUERY, last_updated, update_time)
                ]
                data = await asyncio.gather(*queries)
                data = data[0] + data[1]
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
