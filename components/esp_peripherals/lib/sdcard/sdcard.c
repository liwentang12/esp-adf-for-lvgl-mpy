/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
//#include "esp_vfs_fat.h"
#include "soc/soc_caps.h"

#if SOC_SDMMC_HOST_SUPPORTED
#include "driver/sdmmc_host.h"
#endif
#include "driver/sdmmc_defs.h"
#include "driver/gpio.h"

#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

#include "sdcard.h"
#include "board.h"
#include "esp_idf_version.h"

static const char *TAG = "SDCARD";
static int g_gpio = -1;
static sdmmc_card_t *card = NULL;

static void sdmmc_card_print_info(const sdmmc_card_t *card)
{
    ESP_LOGD(TAG, "Name: %s\n", card->cid.name);
    ESP_LOGD(TAG, "Type: %s\n", (card->ocr & SD_OCR_SDHC_CAP) ? "SDHC/SDXC" : "SDSC");
    ESP_LOGD(TAG, "Speed: %s\n", (card->csd.tr_speed > 25000000) ? "high speed" : "default speed");
    ESP_LOGD(TAG, "Size: %lluMB\n", ((uint64_t) card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
    ESP_LOGD(TAG, "CSD: ver=%d, sector_size=%d, capacity=%d read_bl_len=%d\n",
             card->csd.csd_ver,
             card->csd.sector_size, card->csd.capacity, card->csd.read_block_len);
    ESP_LOGD(TAG, "SCR: sd_spec=%d, bus_width=%d\n", card->scr.sd_spec, card->scr.bus_width);
}

esp_err_t sdcard_mount(const char *base_path, periph_sdcard_mode_t mode)
{
    ESP_LOGE(TAG, "LWT sdcard_mount, base_path: %s\n", base_path);
    return ESP_FAIL;
}

esp_err_t sdcard_unmount(const char *base_path, periph_sdcard_mode_t mode)
{
    ESP_LOGE(TAG, "LWT sdcard_unmount, base_path: %s\n", base_path);
    return ESP_OK;
}

bool sdcard_is_exist()
{
    if (g_gpio >= 0) {
        return (gpio_get_level(g_gpio) == 0x00);
    } else {
        return true;
    }
    return false;
}

int IRAM_ATTR sdcard_read_detect_pin(void)
{
    if (g_gpio >= 0) {
        return gpio_get_level(g_gpio);
    } else {
        return -1;
    }
    return 0;
}

esp_err_t sdcard_destroy()
{
    if (g_gpio >= 0) {
        return gpio_isr_handler_remove(g_gpio);
    }
    return ESP_OK;
}

esp_err_t sdcard_init(int card_detect_pin, void (*detect_intr_handler)(void *), void *isr_context)
{
    esp_err_t ret = ESP_OK;
    if (card_detect_pin >= 0) {
        gpio_set_direction(card_detect_pin, GPIO_MODE_INPUT);
        if (detect_intr_handler) {
            gpio_set_intr_type(card_detect_pin, GPIO_INTR_ANYEDGE);
            gpio_isr_handler_add(card_detect_pin, detect_intr_handler, isr_context);
            gpio_intr_enable(card_detect_pin);
        }
        gpio_pullup_en(card_detect_pin);
    }
    g_gpio = card_detect_pin;
    return ret;
}
