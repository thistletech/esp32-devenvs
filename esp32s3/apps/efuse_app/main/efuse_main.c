/* Read efuse values on ESP32-S3

   The code is adapted from esp-idf/examples/system/efuse

   ESP32-S3 efuse references:
   - https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/efuse.html
   - https://docs.espressif.com/projects/esptool/en/latest/esp32s3/espefuse/index.html

*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_efuse_custom_table.h"
#include "esp_secure_boot.h"
#include "esp_flash_encrypt.h"
#include "sdkconfig.h"


#define BLK0_FIELD_SIZE_BYTES_MAX (32 / 8) // max field size in bytes in BLK0

static const char* TAG = "efuse-app";

typedef struct {
    uint8_t module_version;        /*!< Module version: length 8 bits */
    uint8_t device_role;           /*!< Device role: length 3 bits */
    uint8_t setting_1;             /*!< Setting 1: length 6 bits */
    uint8_t setting_2;             /*!< Setting 2: length 5 bits */
    size_t custom_secure_version;  /*!< Custom secure version: length 16 bits */
    uint16_t reserv;               /*!< Reserv */
} device_desc_t;

typedef struct {
    const esp_efuse_desc_t **field;
    char * field_name;
} blk0_field_t;

static const blk0_field_t BLK0_FIELDS[] = {
    /* From output of `idf.py show-efuse-table`. Only show BLK0 fields.
    Sorted efuse table:                                                                                                                                       [133/1900]
    #       field_name                      efuse_block     bit_start       bit_count
    1       WR_DIS                          EFUSE_BLK0         0               32
    2       WR_DIS.RD_DIS                   EFUSE_BLK0         0               1
    3       WR_DIS.GROUP_1                  EFUSE_BLK0         2               1
    4       WR_DIS.GROUP_2                  EFUSE_BLK0         3               1
    5       WR_DIS.SPI_BOOT_CRYPT_CNT       EFUSE_BLK0         4               1
    6       WR_DIS.SECURE_BOOT_KEY_REVOKE0  EFUSE_BLK0         5               1
    7       WR_DIS.SECURE_BOOT_KEY_REVOKE1  EFUSE_BLK0         6               1
    8       WR_DIS.SECURE_BOOT_KEY_REVOKE2  EFUSE_BLK0         7               1
    9       WR_DIS.KEY0_PURPOSE             EFUSE_BLK0         8               1
    10      WR_DIS.KEY1_PURPOSE             EFUSE_BLK0         9               1
    11      WR_DIS.KEY2_PURPOSE             EFUSE_BLK0         10              1
    12      WR_DIS.KEY3_PURPOSE             EFUSE_BLK0         11              1
    13      WR_DIS.KEY4_PURPOSE             EFUSE_BLK0         12              1
    14      WR_DIS.KEY5_PURPOSE             EFUSE_BLK0         13              1
    15      WR_DIS.SECURE_BOOT_EN           EFUSE_BLK0         15              1
    16      WR_DIS.SECURE_BOOT_AGGRESSIVE_REVOKE    EFUSE_BLK0         16              1
    17      WR_DIS.GROUP_3                  EFUSE_BLK0         18              1
    18      WR_DIS.BLK1                     EFUSE_BLK0         20              1
    19      WR_DIS.SYS_DATA_PART1           EFUSE_BLK0         21              1
    20      WR_DIS.USER_DATA                EFUSE_BLK0         22              1
    21      WR_DIS.KEY0                     EFUSE_BLK0         23              1
    22      WR_DIS.KEY1                     EFUSE_BLK0         24              1
    23      WR_DIS.KEY2                     EFUSE_BLK0         25              1
    24      WR_DIS.KEY3                     EFUSE_BLK0         26              1
    25      WR_DIS.KEY4                     EFUSE_BLK0         27              1
    26      WR_DIS.KEY5                     EFUSE_BLK0         28              1
    27      WR_DIS.SYS_DATA_PART2           EFUSE_BLK0         29              1
    28      WR_DIS.USB_EXCHG_PINS           EFUSE_BLK0         30              1
    29      RD_DIS                          EFUSE_BLK0         32              7
    30      RD_DIS.KEY0                     EFUSE_BLK0         32              1
    31      RD_DIS.KEY1                     EFUSE_BLK0         33              1
    32      RD_DIS.KEY2                     EFUSE_BLK0         34              1
    33      RD_DIS.KEY3                     EFUSE_BLK0         35              1
    34      RD_DIS.KEY4                     EFUSE_BLK0         36              1
    35      RD_DIS.KEY5                     EFUSE_BLK0         37              1
    36      RD_DIS.SYS_DATA_PART2           EFUSE_BLK0         38              1
    37      DIS_ICACHE                      EFUSE_BLK0         40              1
    38      DIS_DCACHE                      EFUSE_BLK0         41              1
    39      DIS_DOWNLOAD_ICACHE             EFUSE_BLK0         42              1
    40      DIS_DOWNLOAD_DCACHE             EFUSE_BLK0         43              1
    41      DIS_FORCE_DOWNLOAD              EFUSE_BLK0         44              1
    42      DIS_USB                         EFUSE_BLK0         45              1
    43      DIS_CAN                         EFUSE_BLK0         46              1
    44      DIS_APP_CPU                     EFUSE_BLK0         47              1
    45      SOFT_DIS_JTAG                   EFUSE_BLK0         48              3
    46      HARD_DIS_JTAG                   EFUSE_BLK0         51              1
    47      DIS_DOWNLOAD_MANUAL_ENCRYPT     EFUSE_BLK0         52              1
    48      USB_EXCHG_PINS                  EFUSE_BLK0         57              1
    49      USB_EXT_PHY_ENABLE              EFUSE_BLK0         58              1
    50      BTLC_GPIO_ENABLE                EFUSE_BLK0         59              2
    51      VDD_SPI_XPD                     EFUSE_BLK0         68              1
    52      VDD_SPI_TIEH                    EFUSE_BLK0         69              1
    53      VDD_SPI_FORCE                   EFUSE_BLK0         70              1
    54      WDT_DELAY_SEL                   EFUSE_BLK0         80              2
    55      SPI_BOOT_CRYPT_CNT              EFUSE_BLK0         82              3
    56      SECURE_BOOT_KEY_REVOKE0         EFUSE_BLK0         85              1
    57      SECURE_BOOT_KEY_REVOKE1         EFUSE_BLK0         86              1
    58      SECURE_BOOT_KEY_REVOKE2         EFUSE_BLK0         87              1
    59      KEY_PURPOSE_0                   EFUSE_BLK0         88              4
    60      KEY_PURPOSE_1                   EFUSE_BLK0         92              4
    61      KEY_PURPOSE_2                   EFUSE_BLK0         96              4
    62      KEY_PURPOSE_3                   EFUSE_BLK0        100              4
    63      KEY_PURPOSE_4                   EFUSE_BLK0        104              4
    64      KEY_PURPOSE_5                   EFUSE_BLK0        108              4
    65      SECURE_BOOT_EN                  EFUSE_BLK0        116              1
    66      SECURE_BOOT_AGGRESSIVE_REVOKE   EFUSE_BLK0        117              1
    67      DIS_USB_JTAG                    EFUSE_BLK0        118              1
    68      DIS_USB_SERIAL_JTAG             EFUSE_BLK0        119              1
    69      STRAP_JTAG_SEL                  EFUSE_BLK0        120              1
    70      USB_PHY_SEL                     EFUSE_BLK0        121              1
    71      FLASH_TPUW                      EFUSE_BLK0        124              4
    72      DIS_DOWNLOAD_MODE               EFUSE_BLK0        128              1
    73      DIS_DIRECT_BOOT                 EFUSE_BLK0        129              1
    74      DIS_USB_SERIAL_JTAG_ROM_PRINT   EFUSE_BLK0        130              1
    75      FLASH_ECC_MODE                  EFUSE_BLK0        131              1
    76      DIS_USB_SERIAL_JTAG_DOWNLOAD_MODE       EFUSE_BLK0        132              1
    77      ENABLE_SECURITY_DOWNLOAD        EFUSE_BLK0        133              1
    78      UART_PRINT_CONTROL              EFUSE_BLK0        134              2
    79      PIN_POWER_SELECTION             EFUSE_BLK0        136              1
    80      FLASH_TYPE                      EFUSE_BLK0        137              1
    81      FLASH_PAGE_SIZE                 EFUSE_BLK0        138              2
    82      FLASH_ECC_EN                    EFUSE_BLK0        140              1
    83      FORCE_SEND_RESUME               EFUSE_BLK0        141              1
    84      SECURE_VERSION                  EFUSE_BLK0        142              16
    85      DIS_USB_OTG_DOWNLOAD_MODE       EFUSE_BLK0        159              1
    86      DISABLE_WAFER_VERSION_MAJOR     EFUSE_BLK0        160              1
    87      DISABLE_BLK_VERSION_MAJOR       EFUSE_BLK0        161              1
    */
    { ESP_EFUSE_WR_DIS, "WR_DIS" },
    { ESP_EFUSE_RD_DIS, "RD_DIS" },
    { ESP_EFUSE_DIS_ICACHE, "DIS_ICACHE" },
    { ESP_EFUSE_DIS_DCACHE, "DIS_DCACHE" },
    { ESP_EFUSE_DIS_DOWNLOAD_ICACHE, "DIS_DOWNLOAD_ICACHE" },
    { ESP_EFUSE_DIS_DOWNLOAD_DCACHE, "DIS_DOWNLOAD_DCACHE" },
    { ESP_EFUSE_DIS_FORCE_DOWNLOAD, "DIS_FORCE_DOWNLOAD" },
    { ESP_EFUSE_DIS_USB, "DIS_USB" },
    { ESP_EFUSE_DIS_CAN, "DIS_CAN" },
    { ESP_EFUSE_DIS_APP_CPU, "DIS_APP_CPU" },
    { ESP_EFUSE_SOFT_DIS_JTAG, "SOFT_DIS_JTAG" },
    { ESP_EFUSE_HARD_DIS_JTAG, "HARD_DIS_JTAG" },
    { ESP_EFUSE_DIS_DOWNLOAD_MANUAL_ENCRYPT, "DIS_DOWNLOAD_MANUAL_ENCRYPT" },
    { ESP_EFUSE_USB_EXCHG_PINS, "USB_EXCHG_PINS" },
    { ESP_EFUSE_USB_EXT_PHY_ENABLE, "USB_EXT_PHY_ENABLE" },
    { ESP_EFUSE_BTLC_GPIO_ENABLE, "BTLC_GPIO_ENABLE" },
    { ESP_EFUSE_VDD_SPI_XPD, "VDD_SPI_XPD" },
    { ESP_EFUSE_VDD_SPI_TIEH, "VDD_SPI_TIEH" },
    { ESP_EFUSE_VDD_SPI_FORCE, "VDD_SPI_FORCE" },
    { ESP_EFUSE_WDT_DELAY_SEL, "WDT_DELAY_SEL" },
    { ESP_EFUSE_SPI_BOOT_CRYPT_CNT, "SPI_BOOT_CRYPT_CNT" },
    { ESP_EFUSE_SECURE_BOOT_KEY_REVOKE0, "SECURE_BOOT_KEY_REVOKE0" },
    { ESP_EFUSE_SECURE_BOOT_KEY_REVOKE1, "SECURE_BOOT_KEY_REVOKE1" },
    { ESP_EFUSE_SECURE_BOOT_KEY_REVOKE2, "SECURE_BOOT_KEY_REVOKE2" },
    { ESP_EFUSE_KEY_PURPOSE_0, "KEY_PURPOSE_0" },
    { ESP_EFUSE_KEY_PURPOSE_1, "KEY_PURPOSE_1" },
    { ESP_EFUSE_KEY_PURPOSE_2, "KEY_PURPOSE_2" },
    { ESP_EFUSE_KEY_PURPOSE_3, "KEY_PURPOSE_3" },
    { ESP_EFUSE_KEY_PURPOSE_4, "KEY_PURPOSE_4" },
    { ESP_EFUSE_KEY_PURPOSE_5, "KEY_PURPOSE_5" },
    { ESP_EFUSE_SECURE_BOOT_EN, "SECURE_BOOT_EN" },
    { ESP_EFUSE_SECURE_BOOT_AGGRESSIVE_REVOKE, "SECURE_BOOT_AGGRESSIVE_REVOKE" },
    { ESP_EFUSE_DIS_USB_JTAG, "DIS_USB_JTAG" },
    { ESP_EFUSE_DIS_USB_SERIAL_JTAG, "DIS_USB_SERIAL_JTAG" },
    { ESP_EFUSE_STRAP_JTAG_SEL, "STRAP_JTAG_SEL" },
    { ESP_EFUSE_USB_PHY_SEL, "USB_PHY_SEL" },
    { ESP_EFUSE_FLASH_TPUW, "FLASH_TPUW" },
    { ESP_EFUSE_DIS_DOWNLOAD_MODE, "DIS_DOWNLOAD_MODE" },
    { ESP_EFUSE_DIS_DIRECT_BOOT, "DIS_DIRECT_BOOT" },
    { ESP_EFUSE_DIS_USB_SERIAL_JTAG_ROM_PRINT, "DIS_USB_SERIAL_JTAG_ROM_PRINT" },
    { ESP_EFUSE_FLASH_ECC_MODE, "FLASH_ECC_MODE" },
    { ESP_EFUSE_DIS_USB_SERIAL_JTAG_DOWNLOAD_MODE, "DIS_USB_SERIAL_JTAG_DOWNLOAD_MODE" },
    { ESP_EFUSE_ENABLE_SECURITY_DOWNLOAD, "ENABLE_SECURITY_DOWNLOAD" },
    { ESP_EFUSE_UART_PRINT_CONTROL, "UART_PRINT_CONTROL" },
    { ESP_EFUSE_PIN_POWER_SELECTION, "PIN_POWER_SELECTION" },
    { ESP_EFUSE_FLASH_TYPE, "FLASH_TYPE" },
    { ESP_EFUSE_FLASH_PAGE_SIZE, "FLASH_PAGE_SIZE" },
    { ESP_EFUSE_FLASH_ECC_EN, "FLASH_ECC_EN" },
    { ESP_EFUSE_FORCE_SEND_RESUME, "FORCE_SEND_RESUME" },
    { ESP_EFUSE_SECURE_VERSION, "SECURE_VERSION" },
    { ESP_EFUSE_DIS_USB_OTG_DOWNLOAD_MODE, "DIS_USB_OTG_DOWNLOAD_MODE" },
    { ESP_EFUSE_DISABLE_WAFER_VERSION_MAJOR, "DISABLE_WAFER_VERSION_MAJOR" },
    { ESP_EFUSE_DISABLE_BLK_VERSION_MAJOR, "DISABLE_BLK_VERSION_MAJOR" },
};

static void print_device_desc(device_desc_t *desc)
{
    ESP_LOGI(TAG, "module_version = %d", desc->module_version);
    if (desc->device_role == 0) {
        ESP_LOGI(TAG, "device_role = None");
    } else if (desc->device_role == 1) {
        ESP_LOGI(TAG, "device_role = Master");
    } else if (desc->device_role == 2) {
        ESP_LOGI(TAG, "device_role = Slave");
    } else {
        ESP_LOGI(TAG, "device_role = Not supported");
    }
    ESP_LOGI(TAG, "setting_1 = %d", desc->setting_1);
    ESP_LOGI(TAG, "setting_2 = %d", desc->setting_2);
    ESP_LOGI(TAG, "custom_secure_version = %d", desc->custom_secure_version);
}


static void read_device_desc_efuse_fields(device_desc_t *desc)
{
    ESP_ERROR_CHECK(esp_efuse_read_field_blob(ESP_EFUSE_MODULE_VERSION, &desc->module_version, 8));
    ESP_ERROR_CHECK(esp_efuse_read_field_blob(ESP_EFUSE_DEVICE_ROLE, &desc->device_role, 3));
    ESP_ERROR_CHECK(esp_efuse_read_field_blob(ESP_EFUSE_SETTING_1, &desc->setting_1, 6));
    ESP_ERROR_CHECK(esp_efuse_read_field_blob(ESP_EFUSE_SETTING_2, &desc->setting_2, 5));
    ESP_ERROR_CHECK(esp_efuse_read_field_cnt(ESP_EFUSE_CUSTOM_SECURE_VERSION, &desc->custom_secure_version));
    print_device_desc(desc);
}


static void read_efuse_fields(device_desc_t *desc)
{
    ESP_LOGI(TAG, "read efuse fields");

    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_efuse_read_field_blob(ESP_EFUSE_MAC_FACTORY, &mac, sizeof(mac) * 8));
    ESP_LOGI(TAG, "1. read MAC address: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    size_t secure_version = 0;
    ESP_ERROR_CHECK(esp_efuse_read_field_cnt(ESP_EFUSE_SECURE_VERSION, &secure_version));
    ESP_LOGI(TAG, "2. read secure_version: %d", secure_version);

    ESP_LOGI(TAG, "3. read custom fields");
    read_device_desc_efuse_fields(desc);
}

static void read_efuse_blk0_fields(void) {

    int field_size = 0;
    char * field_name = NULL;
    uint8_t dst_buf[BLK0_FIELD_SIZE_BYTES_MAX] = {0};
    for (int i = 0; i < sizeof(BLK0_FIELDS) / sizeof(BLK0_FIELDS[0]); ++i) {
        memset(dst_buf, 0, sizeof(dst_buf)/sizeof(dst_buf[0]));
        field_size = esp_efuse_get_field_size(BLK0_FIELDS[i].field);
        field_name = BLK0_FIELDS[i].field_name;
        //int dstout_size_bytes = (field_size + 7) / 8;
        esp_err_t err = ESP_FAIL;
        err = esp_efuse_read_field_blob(BLK0_FIELDS[i].field, &dst_buf, field_size);
        if ( err != ESP_OK ) {
            ESP_LOGW(TAG, "can't read BLK0, field %s; err %i (%s))",
                    BLK0_FIELDS[i].field_name,
                    err,
                    esp_err_to_name(err)
                    );
        } else {
            ESP_LOGI(TAG, "read %s (%d-bit): %02x %02x %02x %02x",
                field_name, field_size, dst_buf[0], dst_buf[1], dst_buf[2], dst_buf[3]);
        }
    }
}

static void dump_efuse_blocks(void) {
    uint8_t block[32];
    esp_efuse_block_t bids[] = {
        EFUSE_BLK0, EFUSE_BLK1, EFUSE_BLK2, EFUSE_BLK3,
        EFUSE_BLK4, EFUSE_BLK5, EFUSE_BLK6, EFUSE_BLK7,
        EFUSE_BLK8, EFUSE_BLK9, EFUSE_BLK10 };

    // Blocks for system purposes
    esp_efuse_block_t sysblk[] = {
        EFUSE_BLK0, EFUSE_BLK1
    };

    for (int i = 0; i < sizeof(bids) / sizeof(bids[0]); ++i ) {
        int is_sysblk = 0; // 0: not system block; 1: is system block
        // Skip system efuse blocks
        for (int j = 0; j < sizeof(sysblk) / sizeof(sysblk[0]); ++j ) {
            if (bids[i] == sysblk[j]) {
                is_sysblk = 1;
                break;
            }
        }

        if (is_sysblk != 0) {
            ESP_LOGI(TAG, "skip BLK%d: used for system purposes", i);
            continue;
        }

        memset(block, 0, sizeof(block) / sizeof(block[0]));
        esp_err_t err = ESP_FAIL;
        err = esp_efuse_read_block(bids[i], &block, 0, sizeof(block) * 8);
        if ( err != ESP_OK ) {
            ESP_LOGW(TAG, "can't read BLK%d; err %i (%s))",
                    i,
                    err,
                    esp_err_to_name(err)
                    );

        } else {
            ESP_LOGI(TAG, "read BLK%d: %02x %02x %02x %02x %02x %02x %02x %02x"
                " %02x %02x %02x %02x %02x %02x %02x %02x"
                " %02x %02x %02x %02x %02x %02x %02x %02x"
                " %02x %02x %02x %02x %02x %02x %02x %02x",
                i,
                block[0], block[1], block[2], block[3], block[4], block[5], block[6], block[7],
                block[8], block[9], block[10], block[11], block[12], block[13], block[14], block[15],
                block[16], block[17], block[18], block[19], block[20], block[21], block[22], block[23],
                block[24], block[25], block[26], block[27], block[28], block[29], block[30], block[31]
                );
        }
    }
}

// Secure boot public key hash is blown to BLK4. This function prints it out.
static void print_pk_hash(void) {
    uint8_t block[32] = {0};
    esp_err_t err = ESP_FAIL;
    err = esp_efuse_read_block(EFUSE_BLK4, &block, 0, sizeof(block) * 8);
    if ( err != ESP_OK ) {
        ESP_LOGW(TAG, "can't read BLK4 (PK_HASH); err %i (%s))",
                err,
                esp_err_to_name(err)
                );

    } else {
        ESP_LOGI(TAG, "PK_HASH: %02x%02x%02x%02x%02x%02x%02x%02x"
            "%02x%02x%02x%02x%02x%02x%02x%02x"
            "%02x%02x%02x%02x%02x%02x%02x%02x"
            "%02x%02x%02x%02x%02x%02x%02x%02x",
            block[0], block[1], block[2], block[3], block[4], block[5], block[6], block[7],
            block[8], block[9], block[10], block[11], block[12], block[13], block[14], block[15],
            block[16], block[17], block[18], block[19], block[20], block[21], block[22], block[23],
            block[24], block[25], block[26], block[27], block[28], block[29], block[30], block[31]
            );
    }

}

#ifdef CONFIG_EFUSE_VIRTUAL
static void write_efuse_fields(device_desc_t *desc, esp_efuse_coding_scheme_t coding_scheme)
{
#if CONFIG_IDF_TARGET_ESP32
    const esp_efuse_coding_scheme_t coding_scheme_for_batch_mode = EFUSE_CODING_SCHEME_3_4;
#else
    const esp_efuse_coding_scheme_t coding_scheme_for_batch_mode = EFUSE_CODING_SCHEME_RS;
#endif

    ESP_LOGI(TAG, "write custom efuse fields");
    if (coding_scheme == coding_scheme_for_batch_mode) {
        ESP_LOGI(TAG, "In the case of 3/4 or RS coding scheme, you cannot write efuse fields separately");
        ESP_LOGI(TAG, "You should use the batch mode of writing fields for this");
        ESP_ERROR_CHECK(esp_efuse_batch_write_begin());
    }

    ESP_ERROR_CHECK(esp_efuse_write_field_blob(ESP_EFUSE_MODULE_VERSION, &desc->module_version, 8));
    ESP_ERROR_CHECK(esp_efuse_write_field_blob(ESP_EFUSE_DEVICE_ROLE, &desc->device_role, 3));
    ESP_ERROR_CHECK(esp_efuse_write_field_blob(ESP_EFUSE_SETTING_1, &desc->setting_1, 6));
    ESP_ERROR_CHECK(esp_efuse_write_field_blob(ESP_EFUSE_SETTING_2, &desc->setting_2, 5));
    ESP_ERROR_CHECK(esp_efuse_write_field_cnt(ESP_EFUSE_CUSTOM_SECURE_VERSION, desc->custom_secure_version));

    if (coding_scheme == coding_scheme_for_batch_mode) {
        ESP_ERROR_CHECK(esp_efuse_batch_write_commit());
    }
}
#endif // CONFIG_EFUSE_VIRTUAL


static esp_efuse_coding_scheme_t get_coding_scheme(void)
{
    // The coding scheme is used for EFUSE_BLK1, EFUSE_BLK2 and EFUSE_BLK3.
    // We use EFUSE_BLK3 (custom block) to verify it.
    esp_efuse_coding_scheme_t coding_scheme = esp_efuse_get_coding_scheme(EFUSE_BLK3);
    if (coding_scheme == EFUSE_CODING_SCHEME_NONE) {
        ESP_LOGI(TAG, "Coding Scheme NONE");
#if CONFIG_IDF_TARGET_ESP32
    } else if (coding_scheme == EFUSE_CODING_SCHEME_3_4) {
        ESP_LOGI(TAG, "Coding Scheme 3/4");
    } else {
        ESP_LOGI(TAG, "Coding Scheme REPEAT");
    }
#else
    } else if (coding_scheme == EFUSE_CODING_SCHEME_RS) {
        ESP_LOGI(TAG, "Coding Scheme RS (Reed-Solomon coding)");
    }
#endif
    return coding_scheme;
}


void app_main(void)
{
    ESP_LOGI(TAG, "Start eFuse example");

#ifdef CONFIG_SECURE_FLASH_ENC_ENABLED
    if (esp_flash_encryption_cfg_verify_release_mode()) {
        ESP_LOGI(TAG, "Flash Encryption is in RELEASE mode");
    } else {
        ESP_LOGW(TAG, "Flash Encryption is NOT in RELEASE mode");
    }
#endif
#ifdef CONFIG_SECURE_BOOT
    if (esp_secure_boot_cfg_verify_release_mode()) {
        ESP_LOGI(TAG, "Secure Boot is in RELEASE mode");
    } else {
        ESP_LOGW(TAG, "Secure Boot is NOT in RELEASE mode");
    }
#endif

    esp_efuse_coding_scheme_t coding_scheme = get_coding_scheme();
    (void) coding_scheme;

    device_desc_t device_desc = { 0 };
    read_efuse_fields(&device_desc);

    ESP_LOGI(TAG, "reading eFuse blocks");
    dump_efuse_blocks();
    ESP_LOGI(TAG, "reading BLK0 fields...");
    read_efuse_blk0_fields();
    print_pk_hash();

    ESP_LOGW(TAG, "This example does not burn any efuse in reality only virtually");

#if CONFIG_IDF_TARGET_ESP32C2
    if (esp_secure_boot_enabled() || esp_flash_encryption_enabled()) {
        ESP_LOGW(TAG, "BLOCK3 is used for secure boot or/and flash encryption");
        ESP_LOGW(TAG, "eFuses from the custom eFuse table can not be used as they are placed in BLOCK3");
        ESP_LOGI(TAG, "Done");
        return;
    }
#endif

#ifdef CONFIG_EFUSE_VIRTUAL
    ESP_LOGW(TAG, "Write operations in efuse fields are performed virtually");
    if (device_desc.device_role == 0) {
        device_desc.module_version = 1;
        device_desc.device_role = 2;
        device_desc.setting_1 = 3;
        device_desc.setting_2 = 4;
        device_desc.custom_secure_version = 5;
        write_efuse_fields(&device_desc, coding_scheme);
        read_device_desc_efuse_fields(&device_desc);
    }
#else
    ESP_LOGW(TAG, "The part of the code that writes efuse fields is disabled");
#endif

    ESP_LOGI(TAG, "Done");
}
