# ESP32-S3 Void App

This is an app that does not do anything (and thus is "void"), but can be
flashed on an ESP32-S3 chip to blow security related efuses according to the
`sdkconfig.defaults` settings.

## How to build app

Build the Docker container image

```bash
# In repository directory's root
cd esp32s3/void_app/
# Pick the sdkconfig flavor to symlink to sdkconfig.defaults. Below we use the
# one with secure boot enabled, and JTAG enabled
ln -Tsf ../fusing/efuse_configs/sdkconfig.dev-sbv2_with_jtag sdkconfig.defaults
cd ../esp32s3/
docker build -f Dockerfile.esp32s3 -t esp32s3:latest .
```

The built second-stage bootloader (signed), application (signed), and partition
table images are at the following locations, respectively.

- `/home/esp/void_app/build/bootloader/bootloader.bin`
- `/home/esp/void_app/build/void_app.bin`
- `/home/esp/void_app/build/partition_table/partition-table.bin`

## How to blow efuses

On Linux host, run container

```bash
# Suppose /dev/ttyUSB0 is the device's usb port on host
docker run --rm -it --device=/dev/ttyUSB0 esp32s3:latest
```

Inside container

```bash
esp@c29e740b2630:~$ . esp-idf/export.sh
esp@c29e740b2630:~$ cd void_app
# Flash bootloader
# Adjust device node (-p option) as needed 
# ESP32-S3's bootloader shall be flashed at offset 0x0000
esp@c29e740b2630:~/void_app$ esptool.py --chip esp32s3 \
    --port=/dev/ttyUSB0 \
    --baud=460800 \
    --before=default_reset \
    --after=no_reset \
    --no-stub \
    write_flash \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size keep \
    0x0 build/bootloader/bootloader.bin
# Flash partition table and app
# Adjust device node (-p option) as needed 
esp@c29e740b2630:~/void_app$ esptool.py -c esp32s3 \
    -p /dev/ttyUSB0 \
    -b 460800 \
    --before=default_reset \
    --after=no_reset \
    --no-stub \
    write_flash \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size keep \
    0x20000 build/void_app.bin \
    0x10000 build/partition_table/partition-table.bin
# Should see "I'm the void app. I do nothing." in serial console output
esp@c29e740b2630:~/void_app$ idf.py monitor
```

If flash encryption is enabled, to re-flash updated partitions, run

```bash
idf.py encrypted-flash monitor
```
