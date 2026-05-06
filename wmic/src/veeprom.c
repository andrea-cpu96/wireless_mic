#include "veeprom.h"

#include <zephyr/fs/nvs.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

LOG_MODULE_REGISTER(veeprom, LOG_LEVEL_INF);

#define VEEPROM_SECTOR_SIZE 4096

volatile static struct nvs_fs fs;
volatile static bool nvs_initialized = false;

void storage_init(void)
{
    /* Get the flash device for the storage partition */
    const struct device *flash_dev = FLASH_AREA_DEVICE(storage);
    if (flash_dev == NULL) {
        LOG_ERR("Flash device not found");
        return;
    }

    fs.flash_device = flash_dev;
    fs.offset = FLASH_AREA_OFFSET(storage);
    fs.sector_size = VEEPROM_SECTOR_SIZE;
    fs.sector_count = (int)(PM_NVS_STORAGE_SIZE / VEEPROM_SECTOR_SIZE); 

    volatile int rc = nvs_mount(&fs);
    if (rc) {
        LOG_ERR("NVS mount failed: %d", rc);
        nvs_initialized = false;
    } else {
        LOG_INF("NVS mounted at offset 0x%lx", fs.offset);
        nvs_initialized = true;
    }
}

void storage_write(uint8_t id, const int32_t *data, int size)
{
    if (!nvs_initialized) {
        LOG_ERR("NVS not initialized");
        return;
    }
    int rc = nvs_write(&fs, id, data, size * sizeof(int32_t));
    if (rc < 0) {
        LOG_ERR("NVS write failed: %d", rc);
    }
}

void storage_read(uint8_t id, int32_t *data, int size)
{
    if (!nvs_initialized) {
        LOG_ERR("NVS not initialized");
        return;
    }
    int rc = nvs_read(&fs, id, data, size * sizeof(int32_t));
    if (rc < 0) {
        LOG_ERR("NVS read failed: %d", rc);
    }
}