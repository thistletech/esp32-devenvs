# ESP32 Void App

This is an app that does not do anything (and thus is "void"), but can be
flashed on an ESP32 chip to blow security related efuses according to the
`sdkconfig.defaults` settings.

## How to build app

Build the Docker container image

```bash
# In repository directory's root
cd esp32
docker build -f Dockerfile.esp32_fuseblower -t esp32fb:latest --build-arg IDF_SDKCONFIG=<sdkconfig name in configs/> .
```

If the `--build-arg` option is not provided, the IDF apps, including the void
app, will be built using [sdkconfig.vanilla](../../configs/sdkconfig.vanilla) as
the build configuration.

To build IDF apps using
[sdkconfig.dev_sbv2_withjtag](../../configs/sdkconfig.dev_sbv2_withjtag), use
command

```bash
# In repository directory's root
cd esp32
docker build -f Dockerfile.esp32_fuseblower -t esp32fb:latest --build-arg IDF_SDKCONFIG=sdkconfig.dev_sbv2_nojtag .
```

The built second-stage bootloader (signed), application (signed), and partition
table images are at the following locations, respectively.

- `/home/esp/apps/void_app/build/bootloader/bootloader.bin`
- `/home/esp/apps/void_app/build/void_app.bin`
- `/home/esp/apps/void_app/build/partition_table/partition-table.bin`

## How to blow efuses

On Linux host, run container

```bash
# Suppose /dev/ttyUSB0 is the device's usb port on host
docker run --rm -it --device=/dev/ttyUSB0 esp32fb:latest
```

Inside container

```bash
esp@c29e740b2630:~$ source ${HOME}/esp-idf/export.sh
esp@c29e740b2630:~$ cd apps/void_app
# Flash bootloader
# Adjust device node (-p option) as needed.
# ESP32-S2's bootloader shall be flashed at offset 0x1000
esp@c29e740b2630:~/apps/void_app$ esptool.py --chip esp32 \
    --port=/dev/ttyUSB0 \
    --baud=460800 \
    --before=default_reset \
    --after=no_reset \
    --no-stub \
    write_flash \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size keep \
    0x1000 build/bootloader/bootloader.bin
# Flash partition table and app
# Adjust device node (-p option) as needed 
esp@c29e740b2630:~/apps/void_app$ esptool.py -c esp32 \
    -p /dev/ttyUSB0 \
    -b 460800 \
    --before=default_reset \
    --after=hard_reset \
    --no-stub \
    write_flash \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size keep \
    0x20000 build/void_app.bin \
    0x10000 build/partition_table/partition-table.bin
# Should see "I'm the void app. I do nothing." in serial console output
esp@c29e740b2630:~/apps/void_app$ idf.py -p /dev/ttyUSB0 monitor
# Inspect efuse values using espefuse.py. We can use the espefuse tool when UART
# ROM download mode is left enabled. Note that "Secure UART ROM download mode"
# is not supported on ESP32, unlike on ESP32-S2 or ESP32-S3.
esp@c29e740b2630:~/apps/void_app$ espefuse.py summary
esp@c29e740b2630:~/apps/void_app$ espefuse.py dump
```

If flash encryption is enabled, to re-flash updated partitions, run

```bash
idf.py encrypted-flash monitor
```
