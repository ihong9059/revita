import asyncio
from bleak import BleakClient, BleakScanner

DEVICE_NAME = "bleCom"
TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # peripheral TX (notify)
RX_CHAR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  # peripheral RX (write)
CHUNK_SIZE = 20


async def main():
    print(f"Scanning for '{DEVICE_NAME}'...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10)
    if not device:
        print("Device not found!")
        return

    print(f"Found: {device.name} ({device.address})")

    async with BleakClient(device) as client:
        print("Connected! Loopback active (Ctrl+C to stop)")

        async def echo_back(data):
            # Send in chunks
            for i in range(0, len(data), CHUNK_SIZE):
                chunk = data[i:i + CHUNK_SIZE]
                await client.write_gatt_char(RX_CHAR_UUID, chunk)

        def on_notify(sender, data):
            text = data.decode("utf-8", errors="replace")
            print(f"RX: {text}", end="", flush=True)
            asyncio.get_running_loop().create_task(echo_back(data))

        await client.start_notify(TX_CHAR_UUID, on_notify)

        try:
            while True:
                await asyncio.sleep(1)
        except KeyboardInterrupt:
            print("\nStopped.")


if __name__ == "__main__":
    asyncio.run(main())
