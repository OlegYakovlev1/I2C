#pragma once

#include <stdint.h>
#include "esp_err.h"

// Структура для зберігання часу/дати, прочитаних з DS1307
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day_of_week; // 1-7 (визначається користувачем)
    uint8_t date;
    uint8_t month;
    uint8_t year;        // 0-99 (напр. 26 = 2026)
} rtc_time_t;

// Зчитати поточний час і дату з RTC (читає 7 регістрів одним пакетом)
esp_err_t ds1307_read(rtc_time_t *t);

// Встановити час і дату в RTC. Викликати один раз; запускає осцилятор (CH=0).
// day_of_week: 1-7 (домовленість користувача), year: 0-99 (напр. 26 = 2026)
void ds1307_set(uint8_t hours, uint8_t minutes, uint8_t seconds,
                uint8_t day_of_week, uint8_t date, uint8_t month, uint8_t year);
