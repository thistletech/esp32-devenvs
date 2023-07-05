# ESP32-S3 Key Generation

## Secure Boot V2 Signing Key

- Generate development signing key

  ESP32 secure boot V2 uses 3072-bit RSA key pairs. One can configure up to 3
  keys (Keys #0, #1, #2) in the hardware root-of-trust in efuses. But we are
  going to support only 1 key (Key #0).

  ```bash
  # Generate private key
  openssl genrsa -out secure_boot_signing_key_private-dev.pem 3072
  # Extract public key
  openssl rsa -in secure_boot_signing_key_private-dev.pem -pubout > secure_boot_signing_key_public-dev.pem
  # Decode public key for human inspection
  openssl rsa -pubin -in secure_boot_signing_key_public-dev.pem -noout -text
  ```

  An example key pair is as follows

  - [secure_boot_signing_key_private-dev.pem](./secure_boot_signing_key_private-dev.pem)
  - [secure_boot_signing_key_public-dev.pem](./secure_boot_signing_key_public-dev.pem)
