#include "rtc.h"
#include "oled.h"

/**
 * @brief 写一个字节到PCF8563
 * @param reg_addr 寄存器地址
 * @param data 要写入的数据
 * @return HAL状态
 */
static HAL_StatusTypeDef PCF8563_Write(uint8_t reg_addr, uint8_t data)
{
    uint8_t buf[2];
    buf[0] = reg_addr;
    buf[1] = data;
    return HAL_I2C_Master_Transmit(&hi2c1, PCF8563_ADDR, buf, 2, HAL_MAX_DELAY);
}

/**
 * @brief 从PCF8563读取一个字节
 * @param reg_addr 寄存器地址
 * @param data 读取数据的存储地址
 * @return HAL状态
 */
static HAL_StatusTypeDef PCF8563_Read(uint8_t reg_addr, uint8_t *data)
{
    HAL_StatusTypeDef status;
    
    status = HAL_I2C_Master_Transmit(&hi2c1, PCF8563_ADDR, &reg_addr, 1, HAL_MAX_DELAY);
    if(status != HAL_OK)
        return status;
    
    return HAL_I2C_Master_Receive(&hi2c1, PCF8563_ADDR | 0x01, data, 1, HAL_MAX_DELAY);
}

/**
 * @brief 初始化PCF8563
 * @return HAL状态
 */
HAL_StatusTypeDef PCF8563_Init(void)
{
    HAL_StatusTypeDef status;
    
    // 设置控制/状态寄存器1
    status = PCF8563_Write(PCF8563_CONTROL_STATUS1, 0x00);  // 正常模式
    if(status != HAL_OK)
        return status;
    
    // 设置控制/状态寄存器2
    status = PCF8563_Write(PCF8563_CONTROL_STATUS2, 0x00);  // 禁用闹钟和定时器
    
    return status;
}

/**
 * @brief 获取PCF8563当前时间
 * @param time 时间结构体指针
 * @return HAL状态
 */
HAL_StatusTypeDef PCF8563_GetTime(RTC_TimeTypeDef *time)
{
    uint8_t buf[7];
    HAL_StatusTypeDef status;
    
    // 连续读取7个时间日期寄存器
    for(uint8_t i = 0; i < 7; i++)
    {
        status = PCF8563_Read(PCF8563_SECONDS + i, &buf[i]);
        if(status != HAL_OK)
            return status;
    }
    
    // 转换BCD码到十进制
    time->second = ((buf[0] & 0x70) >> 4) * 10 + (buf[0] & 0x0F);
    time->minute = ((buf[1] & 0x70) >> 4) * 10 + (buf[1] & 0x0F);
    time->hour = ((buf[2] & 0x30) >> 4) * 10 + (buf[2] & 0x0F);
    time->day = ((buf[3] & 0x30) >> 4) * 10 + (buf[3] & 0x0F);
    time->week = buf[4] & 0x07;
    time->month = ((buf[5] & 0x10) >> 4) * 10 + (buf[5] & 0x0F);
    time->year = ((buf[6] & 0xF0) >> 4) * 10 + (buf[6] & 0x0F);
    
    return HAL_OK;
}

/**
 * @brief 设置PCF8563时间
 * @param time 时间结构体指针
 * @return HAL状态
 */
HAL_StatusTypeDef PCF8563_SetTime(RTC_TimeTypeDef *time)
{
    HAL_StatusTypeDef status;
    
    // 转换十进制到BCD码并写入
    status = PCF8563_Write(PCF8563_SECONDS, ((time->second / 10) << 4) | (time->second % 10));
    if(status != HAL_OK) return status;
    
    status = PCF8563_Write(PCF8563_MINUTES, ((time->minute / 10) << 4) | (time->minute % 10));
    if(status != HAL_OK) return status;
    
    status = PCF8563_Write(PCF8563_HOURS, ((time->hour / 10) << 4) | (time->hour % 10));
    if(status != HAL_OK) return status;
    
    status = PCF8563_Write(PCF8563_DAYS, ((time->day / 10) << 4) | (time->day % 10));
    if(status != HAL_OK) return status;
    
    status = PCF8563_Write(PCF8563_WEEKDAYS, time->week);
    if(status != HAL_OK) return status;
    
    status = PCF8563_Write(PCF8563_MONTHS, ((time->month / 10) << 4) | (time->month % 10));
    if(status != HAL_OK) return status;
    
    status = PCF8563_Write(PCF8563_YEARS, ((time->year / 10) << 4) | (time->year % 10));
    
    return status;
}

/**
 * @brief 在OLED上显示时间
 * @param time 时间结构体指针
 * @param x 起始x坐标
 * @param y 起始y坐标
 */
void RTC_DisplayTime(RTC_TimeTypeDef *time, uint8_t x, uint8_t y)
{
    char timeStr[20];
    char dateStr[20];
    
    // 格式化时间字符串 "HH:MM:SS"
    sprintf(timeStr, "%02d:%02d:%02d", time->hour, time->minute, time->second);
    // 格式化日期字符串 "20YY-MM-DD"
    sprintf(dateStr, "20%02d-%02d-%02d", time->year, time->month, time->day);
    
    // 在OLED上显示时间和日期
    OLED_ShowString(x, y, dateStr, 16);         // 显示日期
    OLED_ShowString(x, y + 2, timeStr, 16);     // 显示时间
    
    // 显示星期
    const char *weekDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    OLED_ShowString(x + 72, y + 2, weekDays[time->week], 16);
}