import asyncio
import logging
from bleak import BleakScanner

async def main(timeout=5.0):
    devices=dict()
    eol=""
    for d in await BleakScanner.discover(timeout=timeout, service_uuids=[]):
        if d.name and (d.name != 'Unknown'):
            print(f"Found {eol}{d.name=},\n\t{d.address=},\n\t{d.rssi=}")
            devices[d.address]  =  d
            eol="\n"
    return devices

asyncio.run(main())
