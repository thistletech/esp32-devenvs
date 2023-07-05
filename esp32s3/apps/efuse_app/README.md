# ESP32-S3 eFuse App

This is a sample app that outputs efuse values on an ESP32-S3 chip.

## How to build app

Build the Docker container image

```bash
# In repository directory's root
cd esp32s3/
docker build -f Dockerfile.esp32s3 -t esp32s3:latest --build-arg IDF_SDKCONFIG=<sdkconfig name in configs/> .
```

In side the built Docker container, the built second-stage bootloader (signed),
application (signed), and partition
table images are at the following locations, respectively.

- `/home/esp/efuse_app/build/bootloader/bootloader.bin`
- `/home/esp/efuse_app/build/efuse_app.bin`
- `/home/esp/efuse_app/build/partition_table/partition-table.bin`

## How to flash bootloader and app

Inside the IDF environment (source `export.sh` if you have not done so).

```bash
# Flash bootloader
# Adjust device node (-p option) as needed 
# ESP32-S3's bootloader shall be flashed at offset 0x0000
esptool.py --chip esp32s3 \
  --port=/dev/ttyUSB0 \
  --baud=460800 \
  --before=default_reset \
  --after=no_reset \
  --no-stub \
  write_flash \
  --flash_mode dio \
  --flash_freq 80m \
  --flash_size keep \
  0x0 /path/to/bootloader.bin
# Flash partition table and app
# Adjust device node (-p option) as needed 
esptool.py -c esp32s3 \
  -p /dev/ttyUSB0 \
  -b 460800 \
  --before=default_reset \
  --after=no_reset \
  --no-stub \
  write_flash \
  --flash_mode dio \
  --flash_freq 80m \
  --flash_size keep \
  0x20000 /path/to/efuse_app.bin \
  0x10000 /path/to/partition-table.bin
# Monitor console output
idf.py monitor
```