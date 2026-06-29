#include "ssd1306.h"
#include "font5x7.h"
#include <string.h>
#include <stdbool.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

#define I2C_PORT    I2C_NUM_0 // Порт I2C (той самий, що ініціалізується в main)
#define OLED_ADDR   0x3C      // I2C адреса дисплея SSD1306

// Кадровий буфер у пам'яті (1 байт = вертикальні 8 пікселів)
static uint8_t oled_buf[OLED_WIDTH * OLED_PAGES];

// Надсилання однієї команди (control byte 0x00 = потік команд)
static esp_err_t ssd1306_send_cmd(uint8_t command) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Co=0, D/C#=0 -> команда
    i2c_master_write_byte(cmd, command, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

// Надсилання масиву даних у відеопам'ять (control byte 0x40 = потік даних)
static esp_err_t ssd1306_send_data(const uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x40, true); // Co=0, D/C#=1 -> дані в GDDRAM
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void ssd1306_init(void) {
    ssd1306_send_cmd(0xAE);             // Вимкнути дисплей
    ssd1306_send_cmd(0x20);             // Режим адресації пам'яті...
    ssd1306_send_cmd(0x00);             // ...горизонтальний
    ssd1306_send_cmd(0x40);             // Стартова лінія = 0
    ssd1306_send_cmd(0xA1);             // Дзеркалювання сегментів (зліва направо)
    ssd1306_send_cmd(0xC8);             // Напрямок сканування COM (зверху вниз)
    ssd1306_send_cmd(0x81);             // Контраст...
    ssd1306_send_cmd(0x7F);             // ...середнє значення
    ssd1306_send_cmd(0xA6);             // Нормальне відображення (не інверсія)
    ssd1306_send_cmd(0xA8);             // Multiplex ratio...
    ssd1306_send_cmd(0x3F);             // ...63 (для 64 рядків)
    ssd1306_send_cmd(0xA4);             // Виводити вміст RAM
    ssd1306_send_cmd(0xD3);             // Зсув дисплея...
    ssd1306_send_cmd(0x00);             // ...немає
    ssd1306_send_cmd(0xD5);             // Частота тактування...
    ssd1306_send_cmd(0x80);             // ...за замовчуванням
    ssd1306_send_cmd(0xD9);             // Період перезаряду...
    ssd1306_send_cmd(0xF1);
    ssd1306_send_cmd(0xDA);             // Конфігурація COM-пінів...
    ssd1306_send_cmd(0x12);             // ...для 128x64
    ssd1306_send_cmd(0xDB);             // Рівень VCOMH...
    ssd1306_send_cmd(0x40);
    ssd1306_send_cmd(0x8D);             // Charge pump...
    ssd1306_send_cmd(0x14);             // ...увімкнути
    ssd1306_send_cmd(0xAF);             // Увімкнути дисплей
}

void ssd1306_clear(void) {
    memset(oled_buf, 0x00, sizeof(oled_buf));
}

void ssd1306_update(void) {
    ssd1306_send_cmd(0x21); // Діапазон стовпців...
    ssd1306_send_cmd(0);
    ssd1306_send_cmd(OLED_WIDTH - 1);
    ssd1306_send_cmd(0x22); // Діапазон сторінок...
    ssd1306_send_cmd(0);
    ssd1306_send_cmd(OLED_PAGES - 1);
    ssd1306_send_data(oled_buf, sizeof(oled_buf));
}

// Встановити один піксель у буфері
static void oled_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    uint16_t idx = x + (y / 8) * OLED_WIDTH;
    uint8_t bit = 1 << (y & 7);
    if (on) {
        oled_buf[idx] |= bit;
    } else {
        oled_buf[idx] &= ~bit;
    }
}

void ssd1306_draw_char(int x, int y, char c, int scale) {
    if (c < 0x20 || c > 0x7A) c = '?'; // Поза межами шрифту -> знак питання
    const uint8_t *glyph = font5x7[c - 0x20];

    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                // Малюємо квадрат scale x scale для збільшення
                for (int dx = 0; dx < scale; dx++) {
                    for (int dy = 0; dy < scale; dy++) {
                        oled_set_pixel(x + col * scale + dx, y + row * scale + dy, true);
                    }
                }
            }
        }
    }
}

void ssd1306_draw_string(int x, int y, const char *str, int scale) {
    while (*str) {
        ssd1306_draw_char(x, y, *str, scale);
        x += 6 * scale; // 5 пікселів символ + 1 проміжок
        str++;
    }
}
