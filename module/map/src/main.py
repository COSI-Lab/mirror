import time
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

ip_cache = TTLCache(maxsize=10000, ttl=5*60)
ip_database = geoip2.database.Reader('./data/GeoLite2-City.mmdb')

def lookup_ip(ip_addr: str):
    """
    Lookup an IP address in the maxmind database
    @param ip_addr The IP address to look up
    """
    if ip_cache.get(ip_addr) is not None:
        return None, None
    ip_cache[ip_addr] = None
    
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
    return {'status': 'success', 'data': {'resultType': 'streams', 'result': [{'stream': {'ip': '128.153.144.199', 'project': 'alpine'}, 'values': [['1774647681003368759', 'GET /alpine/edge/community/aarch64/ HTTP/1.1 128.153.144.199']]}], 'stats': {'summary': {'bytesProcessedPerSecond': 67076, 'linesProcessedPerSecond': 234, 'totalBytesProcessed': 572, 'totalLinesProcessed': 2, 'execTime': 0.008528, 'queueTime': 0.000202, 'subqueries': 0, 'totalEntriesReturned': 1, 'splits': 0, 'shards': 1, 'totalPostFilterLines': 1, 'totalStructuredMetadataBytesProcessed': 16}, 'querier': {'store': {'totalChunksRef': 0, 'totalChunksDownloaded': 0, 'chunksDownloadTime': 0, 'queryReferencedStructuredMetadata': False, 'chunk': {'headChunkBytes': 0, 'headChunkLines': 0, 'decompressedBytes': 0, 'decompressedLines': 0, 'compressedBytes': 0, 'totalDuplicates': 0, 'postFilterLines': 0, 'headChunkStructuredMetadataBytes': 0, 'decompressedStructuredMetadataBytes': 0}, 'chunkRefsFetchTime': 0, 'congestionControlLatency': 0, 'pipelineWrapperFilteredLines': 0}}, 'ingester': {'totalReached': 1, 'totalChunksMatched': 1, 'totalBatches': 2, 'totalLinesSent': 1, 'store': {'totalChunksRef': 0, 'totalChunksDownloaded': 0, 'chunksDownloadTime': 0, 'queryReferencedStructuredMetadata': False, 'chunk': {'headChunkBytes': 572, 'headChunkLines': 2, 'decompressedBytes': 0, 'decompressedLines': 0, 'compressedBytes': 0, 'totalDuplicates': 0, 'postFilterLines': 1, 'headChunkStructuredMetadataBytes': 16, 'decompressedStructuredMetadataBytes': 0}, 'chunkRefsFetchTime': 239795, 'congestionControlLatency': 0, 'pipelineWrapperFilteredLines': 0}}, 'cache': {'chunk': {'entriesFound': 0, 'entriesRequested': 0, 'entriesStored': 0, 'bytesReceived': 0, 'bytesSent': 0, 'requests': 0, 'downloadTime': 0, 'queryLengthServed': 0}, 'index': {'entriesFound': 0, 'entriesRequested': 0, 'entriesStored': 0, 'bytesReceived': 0, 'bytesSent': 0, 'requests': 0, 'downloadTime': 0, 'queryLengthServed': 0}, 'result': {'entriesFound': 0, 'entriesRequested': 0, 'entriesStored': 0, 'bytesReceived': 0, 'bytesSent': 0, 'requests': 0, 'downloadTime': 0, 'queryLengthServed': 0}, 'statsResult': {'entriesFound': 0, 'entriesRequested': 0, 'entriesStored': 0, 'bytesReceived': 0, 'bytesSent': 0, 'requests': 0, 'downloadTime': 0, 'queryLengthServed': 0}, 'volumeResult': {'entriesFound': 0, 'entriesRequested': 0, 'entriesStored': 0, 'bytesReceived': 0, 'bytesSent': 0, 'requests': 0, 'downloadTime': 0, 'queryLengthServed': 0}, 'seriesResult': {'entriesFound': 0, 'entriesRequested': 0, 'entriesStored': 0, 'bytesReceived': 0, 'bytesSent': 0, 'requests': 0, 'downloadTime': 0, 'queryLengthServed': 0}, 'labelResult': {'entriesFound': 0, 'entriesRequested': 0, 'entriesStored': 0, 'bytesReceived': 0, 'bytesSent': 0, 'requests': 0, 'downloadTime': 0, 'queryLengthServed': 0}, 'instantMetricResult': {'entriesFound': 0, 'entriesRequested': 0, 'entriesStored': 0, 'bytesReceived': 0, 'bytesSent': 0, 'requests': 0, 'downloadTime': 0, 'queryLengthServed': 0}}, 'index': {'totalChunks': 0, 'postFilterChunks': 0, 'shardsDuration': 0, 'usedBloomFilters': False}}}}
    print(f"Querying Loki: {query} {start} to {end}")
    params = {
        "query": query,
        "start": str(start),
        "end": str(end)
    }
    try:
        async with session.get(QUERY_ENDPOINT, params=params) as response:
            response.raise_for_status()
            print("query finished")
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
                await asyncio.sleep(0.5)
                print("Starting data fetch...")

                update_time = time.time()
                queries = [
                    get_loki_query(session, RSYNC_QUERY, last_updated, update_time),
                    get_loki_query(session, PROXY_QUERY, last_updated, update_time)
                ]
                print("awaiting queries")
                responses = await asyncio.gather(*queries)
                print("both queries complete")
                data = []
                print(responses[0])
                print(responses[1])
                if responses[0] is not None:
                    data += [(entry["stream"]["ip"], entry["stream"]["module"]) for entry in responses[0]["data"]["result"]]
                if responses[1] is not None:
                    data += [(entry["stream"]["ip"], entry["stream"]["project"]) for entry in responses[1]["data"]["result"]]
                last_updated = update_time
                print("data processed")

                for ip_addr, project in data:
                    lat, lon = lookup_ip(ip_addr)
                    if lat is None:
                        print(f"couldn't look up: {ip_addr}")
                        continue
                    msg = "\n".join([project, str(lat), str(lon)])
                    await broadcast.publish(channel="data", message=msg)
                print("data sent")

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
