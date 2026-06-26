#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "I2C_LESSON";

// Налаштування пінів (для ESP32-S3 можна обирати майже будь-які вільні GPIO)
#define I2C_MASTER_SCL_IO           9      
#define I2C_MASTER_SDA_IO           8      
#define I2C_MASTER_NUM              0      // Номер порту I2C
#define I2C_MASTER_FREQ_HZ          100000 // Стандартна частота 100 кГц
#define RTC_ADDR                    0x68   // I2C адреса DS1307

// Функція для ініціалізації шини I2C
esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(I2C_MASTER_NUM, &conf);
    // Встановлюємо драйвер без буферів (бо ми Master)
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// Функція конвертації з BCD формату у звичайний десятковий
uint8_t bcd_to_dec(uint8_t val) {
    return (val >> 4) * 10 + (val & 0x0F);
}

// Функція для зчитування часу і дати з RTC
void read_rtc_datetime() {
    uint8_t data_rx[7]; // Буфер для 7 регістрів: сек, хв, год, день тижня, дата, місяць, рік

    // 1. Створюємо "ланцюжок" команд для I2C
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // 2. Формуємо запит на запис: вказуємо адресу пристрою і номер першого регістра (0x00)
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RTC_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Починаємо з регістра секунд

    // 3. Робимо Restart і просимо пристрій віддати дані
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RTC_ADDR << 1) | I2C_MASTER_READ, true);
    // Читаємо 6 байт з ACK, а останній (рік) з NACK. DS1307 сам інкрементує вказівник регістра.
    i2c_master_read(cmd, data_rx, 6, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &data_rx[6], I2C_MASTER_NACK); // Останній байт + NACK (кінець)
    i2c_master_stop(cmd);

    // 4. Виконуємо сформований ланцюжок команд
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd); // Очищуємо пам'ять

    if (ret == ESP_OK) {
        // Конвертуємо BCD формат у звичайні цифри
        uint8_t seconds = bcd_to_dec(data_rx[0] & 0x7F); // 0x7F відкидає старший біт CH (Clock Halt)
        uint8_t minutes = bcd_to_dec(data_rx[1] & 0x7F);
        uint8_t hours   = bcd_to_dec(data_rx[2] & 0x3F); // 0x3F: біти 0-5 (режим 24 години)
        uint8_t date    = bcd_to_dec(data_rx[4] & 0x3F); // Число місяця (1-31)
        uint8_t month   = bcd_to_dec(data_rx[5] & 0x1F); // Місяць (1-12)
        uint8_t year    = bcd_to_dec(data_rx[6]);        // Рік (00-99)

        ESP_LOGI(TAG, "Час: %02d:%02d:%02d   Дата: %02d.%02d.20%02d",
                 hours, minutes, seconds, date, month, year);
    } else {
        ESP_LOGE(TAG, "Помилка читання I2C: %s", esp_err_to_name(ret));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Ініціалізація I2C...");
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C ініціалізовано успішно!");

    while (1) {
        read_rtc_datetime();
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Чекаємо 1 секунду
    }
}