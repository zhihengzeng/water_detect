#ifndef __RTC_H
#define __RTC_H

#include "main.h"
#include "i2c.h"

// PCF8563设备地址
#define PCF8563_ADDR              0xA2    // 器件地址 (写地址)

// PCF8563寄存器地址
#define PCF8563_CONTROL_STATUS1   0x00    // 控制/状态寄存器1
#define PCF8563_CONTROL_STATUS2   0x01    // 控制/状态寄存器2
#define PCF8563_SECONDS          0x02    // 秒寄存器
#define PCF8563_MINUTES          0x03    // 分寄存器
#define PCF8563_HOURS            0x04    // 时寄存器
#define PCF8563_DAYS             0x05    // 日期寄存器
#define PCF8563_WEEKDAYS         0x06    // 星期寄存器
#define PCF8563_MONTHS           0x07    // 月寄存器
#define PCF8563_YEARS            0x08    // 年寄存器

// 时间日期结构体
typedef struct {
    uint8_t second;     // 秒 (0-59)
    uint8_t minute;     // 分 (0-59)
    uint8_t hour;       // 时 (0-23)
    uint8_t day;        // 日 (1-31)
    uint8_t week;       // 周 (0-6)
    uint8_t month;      // 月 (1-12)
    uint8_t year;       // 年 (0-99)
} RTC_TimeTypeDef;

// 函数声明
HAL_StatusTypeDef PCF8563_Init(void);
HAL_StatusTypeDef PCF8563_GetTime(RTC_TimeTypeDef *time);
HAL_StatusTypeDef PCF8563_SetTime(RTC_TimeTypeDef *time);
void RTC_DisplayTime(RTC_TimeTypeDef *time, uint8_t x, uint8_t y);

#endif