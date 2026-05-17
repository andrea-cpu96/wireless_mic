#include "veeprom.h"

#include <zephyr/fs/nvs.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(veeprom, LOG_LEVEL_INF);

#define VEEPROM_SECTOR_SIZE 4096
#define VEEPROM_SECTOR_COUNT (CONFIG_PM_PARTITION_SIZE_NVS_STORAGE / VEEPROM_SECTOR_SIZE)

static const struct device *flash_dev;
static uint32_t flash_offset;
static uint32_t flash_size;

/**
 * @brief veeprom_init
 */
void veeprom_init(void)
{
    flash_dev = FLASH_AREA_DEVICE(storage);
    if (!flash_dev)
    {
        LOG_ERR("Flash device not found");
        return;
    }

    flash_offset = FLASH_AREA_OFFSET(storage);
    flash_size = FLASH_AREA_SIZE(storage);

    LOG_INF("Flash RAW storage at 0x%08x, size %u bytes",
            flash_offset, flash_size);
    
    /*
     * Clear flash sectors dedicated to VEEPROM
     *
     * NOTE: This is necessary to avoid data corruption when writing to flash,
     * as flash_write can only change bits from 1 to 0, and not from 0 to 1.
     */
    for (int i = 0; i < VEEPROM_SECTOR_COUNT; ++i)
    {
        flash_erase(flash_dev, flash_offset + (i * VEEPROM_SECTOR_SIZE), VEEPROM_SECTOR_SIZE);
    }
}

/**
 * @brief veeprom_write
 *
 * @param data
 * @param size
 * @return int
 */
int veeprom_write(const void *data, int size)
{
    static int new_start_address = 0;
    int address = new_start_address;

    flash_write(flash_dev, flash_offset + new_start_address,
                data, size);

    new_start_address += size;

    return  address;
}

/**
 * @brief veeprom_read
 *
 * @param address
 * @param data
 * @param size
 */
void veeprom_read(uint32_t address, void *data, int size)
{
    flash_read(flash_dev, flash_offset + address, data, size);
}
