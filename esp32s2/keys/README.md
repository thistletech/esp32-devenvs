# ESP32 Secure Boot V2 and MCUboot Application Verificaiton Keys

## Keygen: Development Keys

- MCUboot image verification key pair (ECDSA-P256)

  ```bash
  export MCUBOOT_SK="mcuboot-ecdsa-p256_private_dev.pem"
  export MCUBOOT_PK="mcuboot-ecdsa-p256_public_dev.pem"
  # Generate private key with openssl
  # Alternatively, with imgtool:
  #   mcuboot/scripts/imgtool.py keygen -t ecdsa-p256 -k mcuboot-ecdsa-p256_private.pem
  openssl ecparam -name prime256v1 -genkey -noout -out "${MCUBOOT_SK}"
  # Extract public key from private key
  openssl ec -in "${MCUBOOT_SK}" -pubout -out "${MCUBOOT_PK}"
  # Decode public key for human inspection
  openssl ec -pubin -in ${MCUBOOT_PK} -text -noout
  ```

- Secure Boot V2 signing/verification key pair (RSA-3072)

  - Generate development signing key

    ESP32 secure boot V2 uses 3072-bit RSA key pairs. One can configure up to 3
    keys (Keys #0, #1, #2) in the hardware root-of-trust in efuses. But we are
    going to support only 1 key (Key #0).

    ```bash
    # Generate private key
    openssl genrsa -out sbv2_private_dev.pem 3072
    # Extract public key
    openssl rsa -in sbv2_private_dev.pem -pubout > sbv2_public_dev.pem
    # Decode public key for human inspection
    openssl rsa -pubin -in sbv2_public_dev.pem -noout -text
    ```

  An example key pair is as follows

  - [sbv2_private_dev.pem](./sbv2_private_dev.pem)
  - [sbv2_public_dev.pem](./sbv2_public_dev.pem)


## Dummy Keys

You will also find "dummy" keys in this directory. These are SBV2 and MCUboot
signing keys generated following the above steps, but only used for testing
image patching (a feature to be available on <https://app.thistle.tech/> soon).
Dummy keys [sbv2_private_dummy.pem](./sbv2_private_dummy.pem) and
[mcuboot_private_dummy.pem](./mcuboot_private_dummy.pem) are manually passed as
`--build-arg` options when building the Zephyr container using
[Dockerfile.esp32s2_zephyr](../Dockerfile.esp32s2_zephyr), to get test images
signed with the dummy keys.