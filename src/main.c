#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ds1307.h"
#include "ssd1306.h"

static const char *TAG = "MAIN";

// Налаштування шини I2C (для ESP32-S3 можна обирати майже будь-які вільні GPIO)
#define I2C_MASTER_SCL_IO   9
#define I2C_MASTER_SDA_IO   8
#define I2C_MASTER_NUM      0      // Номер порту I2C
#define I2C_MASTER_FREQ_HZ  100000 // Стандартна частота 100 кГц

// Абревіатури днів тижня. Індекс = значення регістра DS1307 (домовляємось: 1=Sun ... 7=Sat)
static const char *WEEKDAYS[] = {"---", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Ініціалізація шини I2C (Master)
static esp_err_t i2c_master_init(void) {
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

// Скомпонувати і вивести час та дату на дисплей
static void display_clock(const rtc_time_t *t) {
    char time_str[16];
    char date_str[24];

    // Рядок часу: HH:MM:SS
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
             t->hours, t->minutes, t->seconds);

    // Рядок дати: EEE DD.MM.YYYY (день тижня + дата)
    const char *wday = (t->day_of_week >= 1 && t->day_of_week <= 7)
                           ? WEEKDAYS[t->day_of_week] : "---";
    snprintf(date_str, sizeof(date_str), "%s %02d.%02d.20%02d",
             wday, t->date, t->month, t->year);

    ssd1306_clear();
    // Час великим шрифтом (scale=2): ширина 8 символів * 12px = 96, центруємо
    ssd1306_draw_string(16, 12, time_str, 2);
    // Дата звичайним шрифтом (scale=1): ширина 14 символів * 6px = 84, центруємо
    ssd1306_draw_string(22, 44, date_str, 1);
    ssd1306_update();

    // Дублюємо в монітор порту
    ESP_LOGI(TAG, "%s   %s", time_str, date_str);
}

void app_main(void) {
    ESP_LOGI(TAG, "Ініціалізація I2C...");
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C ініціалізовано успішно!");

    // Ініціалізація дисплея
    ssd1306_init();
    ssd1306_clear();
    ssd1306_update();
    ESP_LOGI(TAG, "Дисплей SSD1306 ініціалізовано!");

    // === ВСТАНОВЛЕННЯ ЧАСУ (виконати ОДИН РАЗ) ===
    // Параметри: год, хв, сек, день тижня (1=Sun..7=Sat), число, місяць, рік(20XX).
    // Дата: понеділок, 29.06.2026. ЗАМІНИ час (год, хв, сек) на свій поточний!
    // ПІСЛЯ першої вдалої прошивки ЗАКОМЕНТУЙ цей рядок, інакше час
    // буде скидатися при кожному перезавантаженні плати.
    // ds1307_set(12, 0, 0, /*Mon*/2, 29, 6, 26);

    rtc_time_t now;
    while (1) {
        if (ds1307_read(&now) == ESP_OK) {
            display_clock(&now);
        } else {
            ESP_LOGE(TAG, "Помилка читання RTC");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Оновлення раз на секунду
    }
}
