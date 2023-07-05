# eFuse Configurations

Reference:
[espefuse.py](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/espefuse/index.html)

## Development fusing

### eFuse values of unfused device

For reference, an example output of the `espefuse.py summary` command executed
on a laptop connected to an ESP32-S3 dev board via UART is in
[esp32s3-efuse-unfused.txt](data/esp32s3-efuse-unfused.txt).

### Secure boot fusing

There are two ways to fuse the dev board, as follows.

#### (Recommended) Approach 1: flash signed bootload and app images

There are three development-fusing flavors, whose associated `sdkconfig` are

- [sdkconfig.dev-sbv2-fe_withjtag](../configs/sdkconfig.dev-sbv2-fe_withjtag)
  - Both flash encryption (with AES-256-XTS) and secure boot are enabled; JTAG
    is not locked out
- [sdkconfig.dev-sbv2_withjtag](../configs/sdkconfig.dev-sbv2_withjtag)
  - Secure boot is enabled. Flash encryption is not enabled. JTAG is not locked
    out.
- [sdkconfig.dev-sbv2_nojtag](../configs/sdkconfig.dev-sbv2_nojtag)
  - secure boot is enabled. Flash encryption is not enabled. JTAG is locked
    out.

Note that in all the above flavors:

- We enable secure ROM download mode, which only allows image flashing, but
  disable the use of `espefuse.py` tool, after secure boot is enabled.
- We adjusted the partition table offset to `0x10000` from the default `0x8000`,
  to make sure that the signed bootloader image fit.

To see the security related configuration tweaks, one may compare these
`sdkconfig` files with the original configuration file
[sdkconfig.orig](../configs/sdkconfig.orig).


1. Build the void app according to [these
instructions](https://github.com/thistletech/esp32-devenvs/blob/main/esp32s3/void_app/README.md).
Before running `idf.py build`, symlink the `sdkconfig.*` file for the desired
fusing flavor to `void_app/sdkconfig`.


2. Connect laptop where `esp-idf` is set up to the ESP32-S3 development board
via UART (as shown below). Flash the signed bootloader and app images, and the
partition table.

   ![Connect via UART](../img/esp32s3-uart.png "UART connection")


   ```bash
   # Flash bootloader
   esptool.py --chip esp32s3 \
     --port=/dev/ttyUSB0 # adjust device node as needed \
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
    esptool.py -c esp32s3 \
      -p /dev/ttyUSB0 # adjust device node as needed \
      -b 460800 \
      --before=default_reset \
      --after=no_reset \
      --no-stub \
      write_flash \
      --flash_mode dio \
      --flash_freq 80m \
      --flash_size keep \
      0x20000 /path/to/void_app.bin \
      0x10000 /path/to/partition-table.bin
    ```

    Once all images are flashed, upon the next successful reboot, secure boot
    (and flash encryption, if the flavor is selected) will be enabled, and the
    relevant efuses will be blown by the ROM first-stage bootloader.


#### Approach 2: use espefuse.py (untested)

Fuse public key digest from public key PEM file

```bash
$ espefuse.py burn_key_digest BLOCK_KEY0 /path/to/secure_boot_signing_key_public.pem SECURE_BOOT_DIGEST0
espefuse.py v4.5.dev2
Connecting....
Detecting chip type... ESP32-S3

=== Run "burn_key_digest" command ===
Burn keys to blocks:
 - BLOCK_KEY0 -> [a9 9b c1 9c b7 e5 fd 37 99 56 8a 2d ae 37 83 c3 c8 3c 85 95 a3 00 19 79 76 1a a7 d5 de 43 71 da]
        'KEY_PURPOSE_0': 'USER' -> 'SECURE_BOOT_DIGEST0'.
        Disabling write to 'KEY_PURPOSE_0'.
        Disabling write to key block


Check all blocks for burn...
idx, BLOCK_NAME,          Conclusion
[00] BLOCK0               is not empty
        (written ): 0x0000000080000100000000000000d1f50000000000000000
        (to write): 0x000000000000000000000000090000000000000000800100
        (coding scheme = NONE)
[04] BLOCK_KEY0           is empty, will burn the new value
.
This is an irreversible operation!
Type 'BURN' (all capitals) to continue.
```

Burn the `SECURE_BOOT_EN` fuse

```bash
$ espefuse.py burn_efuse SECURE_BOOT_EN
```

TODO: burn fuse to revoke unused key slots
