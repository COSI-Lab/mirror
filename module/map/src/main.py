import time
import json
import asyncio
from contextlib import asynccontextmanager
import traceback
import aiohttp
from broadcaster import Broadcast
from fastapi import FastAPI, WebSocket
import geoip2.database
from cachetools import TTLCache

broadcast = Broadcast('memory://')

LOKI_HOSTNAME = "loki:3100"
QUERY_ENDPOINT = "http://" + LOKI_HOSTNAME + "/loki/api/v1/query_range"
PROXY_QUERY = "{container=\"proxy\"} | json | __error__=`` | line_format `{{.request}} {{.remote_addr}}` | regexp `^GET /(?P<project>[^ /]*).*?(?P<ip>[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})` | keep project, ip"
RSYNC_QUERY = "{container=\"rsyncd\"} |= `rsync on` | regexp `^.*? rsync on (?P<module>[^ /]*).*? from .* \\((?P<ip>.*?)\\)` | keep module,ip"
MIRROR_FILE = "./mirror.json"

ip_cache = TTLCache(maxsize=10000, ttl=5*60)
ip_database = geoip2.database.Reader('./data/GeoLite2-City.mmdb')

mirror_projects = list(map(str.lower, json.load(MIRROR_FILE)['mirrors'].keys()))


def lookup_ip(ip_addr: str):
    """
    Lookup an IP address in the maxmind database
    @param ip_addr The IP address to look up
    """
    try:
        match = ip_database.city(ip_addr)
        return match.location.latitude, match.location.longitude
    except geoip2.errors.AddressNotFoundError:
        return None, None


async def get_loki_query(session, query, start, end):
    """
    Make a LogQL query to loki
    @param session 
    @param query The LogQL query
    @param start Start of time range
    @param end End of time range
    """
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
    except asyncio.TimeoutError:
        print(f"Request to {QUERY_ENDPOINT} timed out")


async def data_thread_task():
    """
    The thread spun up to handle a map session
    """
    async with aiohttp.ClientSession() as session:
        last_updated = time.time()
        while True:
            try:
                await asyncio.sleep(3)

                update_time = time.time()
                queries = [
                    get_loki_query(session, RSYNC_QUERY, last_updated, update_time),
                    get_loki_query(session, PROXY_QUERY, last_updated, update_time)
                ]
                responses = await asyncio.gather(*queries)
                data = []

                if responses[0] is not None:
                    data += [(entry["stream"]["ip"], entry["stream"]["module"]) for entry in responses[0]["data"]["result"]]
                if responses[1] is not None:
                    data += [(entry["stream"]["ip"], entry["stream"]["project"]) for entry in responses[1]["data"]["result"]]
                last_updated = update_time

                for ip_addr, project in data:
                    if project.lower() not in mirror_projects:
                        continue

                    cache_key = ip_addr + "-" + project
                    if ip_cache.get(cache_key) is not None:
                        continue
                    ip_cache[cache_key] = -1

                    lat, lon = lookup_ip(ip_addr)
                    if lat is None:
                        print(f"couldn't look up: {ip_addr}")
                        continue
                    msg = "\n".join([project, str(lat), str(lon)])
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
    uvicorn.run("main:app", host="0.0.0.0", port=8080, workers=1, lifespan="on")
