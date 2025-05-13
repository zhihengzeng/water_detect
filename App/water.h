#ifndef __WATER_H
#define __WATER_H

#include "main.h"
#include "adc.h"
#include "oled.h"
#include "rtc.h"

// 定义ADC采样缓冲区大小
#define ADC_BUFFER_SIZE 100

// 定义页面切换时间(ms)
#define PAGE_SWITCH_TIME 5000

// 定义水位稳定计数阈值
#define WATER_STABLE_COUNT 5

// 页面枚举类型
typedef enum {
    PAGE_WATER,     // 水位信息页面
    PAGE_TIME       // 时间信息页面
} DisplayPage_TypeDef;

// 水位检测状态
typedef enum {
    WATER_INIT,             // 初始化状态
    WATER_SAMPLING,         // 采样中
    WATER_DATA_READY        // 数据准备就绪，可以处理
} WaterStatus_TypeDef;

// 函数声明
void WATER_Init(void);
void WATER_Process(void);
uint8_t WATER_GetLevel(uint16_t adc_value);
void WATER_DisplayLevel(uint8_t level, uint8_t force_update);
void WATER_DisplayWaterPage(void);
void WATER_DisplayTimePage(RTC_TimeTypeDef *time);
void WATER_PageManager(void);
void WATER_SwitchPage(DisplayPage_TypeDef page);

// 回调函数声明
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);

// 添加函数声明
DisplayPage_TypeDef WATER_GetCurrentPage(void);
uint8_t WATER_IsPageLocked(void);

#endif /* __WATER_H */
