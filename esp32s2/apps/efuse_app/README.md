# ESP32-S2 eFuse App

This is a sample app that outputs efuse values on an ESP32-S2 chip.

## How to build app

Build the Docker container image

```bash
# In repository directory's root
cd esp32s2
docker build -f Dockerfile.esp32s2_fuseblower -t esp32s2fb:latest --build-arg IDF_SDKCONFIG=<sdkconfig name in configs/> .
```

If the `--build-arg` option is not provided, the IDF apps, including the void
app, will be built using [sdkconfig.vanilla](../../configs/sdkconfig.vanilla) as
the build configuration.

To build IDF apps using
[sdkconfig.dev_sbv2_withjtag](../../configs/sdkconfig.dev_sbv2_withjtag), use
command

```bash
# In repository directory's root
cd esp32s2
docker build -f Dockerfile.esp32s2_fuseblower -t esp32s2fb:latest --build-arg IDF_SDKCONFIG=sdkconfig.dev_sbv2_withjtag .
```

The built second-stage bootloader (unsigned), application (unsigned), and
partition table images are at the following locations, respectively.

- `/home/esp/efuse_app/build/bootloader/bootloader.bin`
- `/home/esp/efuse_app/build/efuse_app.bin`
- `/home/esp/efuse_app/build/partition_table/partition-table.bin`

Note that the build configuration [sdkconfig.defaults](./sdkconfig.defaults) is
the default, vanilla configuration without any additional security features
enabled, except that we adjusted the partition table offset from `0x8000` to
`0x10000`, so that a later signed bootloader can be flashed without a problem.

If secure boot is enabled, the bootloader and efuse_app images need to be signed
for them to execute.

# How to flash bootloader and app

On Linux host, run container

```bash
# Suppose /dev/ttyUSB0 is the device's usb port on host
docker run --rm -it --device=/dev/ttyUSB0 esp32s2fb:latest
```

Inside container

```bash
source ${HOME}/esp-idf/export.sh
cd ${HOME}/apps/efuse_app
# Flash bootloader
# Adjust device node (-p option) as needed
# ESP32-S2's bootloader shall be flashed at offset 0x1000
esptool.py --chip esp32s2 \
  -p /dev/ttyUSB0 \
  -b 460800 \
  --before default_reset \
  --after no_reset \
  write_flash \
  --flash_mode dio \
  --flash_size keep \
  --flash_freq 80m \
  0x1000 build/bootloader/bootloader.bin
# Flash partition table and app
# Adjust device node (-p option) as needed
esptool.py --chip esp32s2 \
  -p /dev/ttyUSB0 \
  -b 460800 \
  --before default_reset \
  --after hard_reset \
  write_flash \
  --flash_mode dio \
  --flash_size keep \
  --flash_freq 80m \
  0x10000 build/partition_table/partition-table.bin \
  0x20000 build/efuse_app.bin
# Monitor console output
# When secure download mode is enabled, need to specify the port (-p)
idf.py -p /dev/ttyUSB0 monitor
```