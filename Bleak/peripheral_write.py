import asyncio
from bleak import BleakClient

# デバイスのMACアドレスまたはBLEアドレス
device_address = "CD:AB:14:06:84:19"

# 書き込み先のキャラクタリスティックUUID
characteristic_uuid = "00004a02-0000-1000-8000-00805f9b34fb"  # 実際のUUIDに置き換えてください

# 初期値とインクリメントステップ
initial_value = 0
increment_step = 1

async def run():
    async with BleakClient(device_address) as client:

        paired = await client.pair()
        if paired:
            print("Successfully paired with the device.")
        else:
	        print(f"Device can not connect.")
	        exit(1);


        value = initial_value
        while paired:
            # インクリメントした値をバイト列に変換
            data_to_write = value.to_bytes(4, byteorder='little', signed=False)
            
            # キャラクタリスティックにデータを書き込む
            await client.write_gatt_char(characteristic_uuid, data_to_write)
            print(f"Data written to {characteristic_uuid}: {data_to_write} (Value: {value})")
            
            # インクリメント
            value += increment_step

            # 次の書き込みまで待機（例: 1秒）
            await asyncio.sleep(1)

# イベントループを実行して非同期関数を呼び出す
loop = asyncio.get_event_loop()
loop.run_until_complete(run())
