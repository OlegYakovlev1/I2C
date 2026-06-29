#pragma once

// Геометрія дисплея SSD1306
#define OLED_WIDTH   128
#define OLED_HEIGHT  64
#define OLED_PAGES   (OLED_HEIGHT / 8) // 8 сторінок по 8 пікселів

// Ініціалізація дисплея (стандартна послідовність для 128x64)
void ssd1306_init(void);

// Очистити кадровий буфер (у пам'яті, без виводу на екран)
void ssd1306_clear(void);

// Вивести вміст кадрового буфера на дисплей
void ssd1306_update(void);

// Намалювати символ у буфері (scale = масштаб, напр. 1 або 2)
void ssd1306_draw_char(int x, int y, char c, int scale);

// Намалювати рядок тексту в буфері
void ssd1306_draw_string(int x, int y, const char *str, int scale);
