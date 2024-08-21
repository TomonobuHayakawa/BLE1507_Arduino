import time
import asyncio
from bleak import BleakClient

# デバイスのMACアドレスまたはBLEアドレス
device_address = "CD:AB:14:06:84:19"

# 書き込み先のキャラクタリスティックUUID
notify_characteristic_uuid = "00004a02-0000-1000-8000-00805f9b34fb"

# 通信を行う時間（秒）
timeout = 60

async def notification_handler(sender, data):
    print(f"Notification received from {sender}: {data}")

async def run():
    async with BleakClient(device_address) as client:
        print(f"Connected: {client.is_connected}")

        paired = await client.pair()
        if paired:
            print("Successfully paired with the device.")
        else:
            print(f"Device can not connect.")
            exit(1);

        # 通知を受け取るキャラクタリスティックの通知を開始
        await client.start_notify(notify_characteristic_uuid, notification_handler)
        print(f"Started notifications on {notify_characteristic_uuid}")

        start_time = asyncio.get_event_loop().time()

        while True:
            # 現在の時間を取得
            current_time = asyncio.get_event_loop().time()
            elapsed_time = current_time - start_time

            # タイムアウトの確認
            if elapsed_time > timeout:
                print("Timeout reached. Stopping notifications.")
                break

            # 次のループまで待機（例: 1秒）
            await asyncio.sleep(1)

        # 通知を停止
        await client.stop_notify(notify_characteristic_uuid)
        print(f"Stopped notifications on {notify_characteristic_uuid}")

# イベントループを実行して非同期関数を呼び出す
loop = asyncio.get_event_loop()
loop.run_until_complete(run())
