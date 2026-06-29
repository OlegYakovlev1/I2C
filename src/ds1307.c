#include "ds1307.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#define I2C_PORT   I2C_NUM_0 // Порт I2C (той самий, що ініціалізується в main)
#define RTC_ADDR   0x68      // I2C адреса DS1307

static const char *TAG = "DS1307";

// Конвертація з BCD формату у звичайний десятковий
static uint8_t bcd_to_dec(uint8_t val) {
    return (val >> 4) * 10 + (val & 0x0F);
}

// Конвертація зі звичайного десяткового у BCD формат
static uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

void ds1307_set(uint8_t hours, uint8_t minutes, uint8_t seconds,
                uint8_t day_of_week, uint8_t date, uint8_t month, uint8_t year) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RTC_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Починаємо з регістра секунд

    // Біт CH = 0 у регістрі секунд -> запускає осцилятор
    i2c_master_write_byte(cmd, dec_to_bcd(seconds) & 0x7F, true); // 0x00
    i2c_master_write_byte(cmd, dec_to_bcd(minutes), true);        // 0x01
    i2c_master_write_byte(cmd, dec_to_bcd(hours) & 0x3F, true);   // 0x02 (режим 24h)
    i2c_master_write_byte(cmd, dec_to_bcd(day_of_week), true);    // 0x03
    i2c_master_write_byte(cmd, dec_to_bcd(date), true);           // 0x04
    i2c_master_write_byte(cmd, dec_to_bcd(month), true);          // 0x05
    i2c_master_write_byte(cmd, dec_to_bcd(year), true);           // 0x06
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Час успішно встановлено!");
    } else {
        ESP_LOGE(TAG, "Помилка запису часу: %s", esp_err_to_name(ret));
    }
}

esp_err_t ds1307_read(rtc_time_t *t) {
    uint8_t data_rx[7];

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RTC_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Починаємо з регістра секунд

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RTC_ADDR << 1) | I2C_MASTER_READ, true);
    // 6 байт з ACK, останній (рік) з NACK. DS1307 сам інкрементує вказівник регістра.
    i2c_master_read(cmd, data_rx, 6, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &data_rx[6], I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        t->seconds     = bcd_to_dec(data_rx[0] & 0x7F); // 0x7F відкидає біт CH
        t->minutes     = bcd_to_dec(data_rx[1] & 0x7F);
        t->hours       = bcd_to_dec(data_rx[2] & 0x3F); // біти 0-5 (режим 24h)
        t->day_of_week = bcd_to_dec(data_rx[3] & 0x07);
        t->date        = bcd_to_dec(data_rx[4] & 0x3F);
        t->month       = bcd_to_dec(data_rx[5] & 0x1F);
        t->year        = bcd_to_dec(data_rx[6]);
    }
    return ret;
}
