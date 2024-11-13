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
        
            # ターミナルから入力を受け取る
            text = input("Enter text to send: ")
            
            data = text.encode()
            
            # キャラクタリスティックにデータを書き込む
            await client.write_gatt_char(characteristic_uuid, data)
            print(f"Data written to {characteristic_uuid}: {data}")
            

# イベントループを実行して非同期関数を呼び出す
loop = asyncio.get_event_loop()
loop.run_until_complete(run())
