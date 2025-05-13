#ifndef __FLASH_H
#define __FLASH_H

#include "main.h"

// 定义Flash参数存储地址(使用最后一页)
#define FLASH_PARAM_ADDR      0x0801FC00  // STM32F103C8的Flash最后一页起始地址
#define FLASH_PARAM_WATER_THRESHOLD  (FLASH_PARAM_ADDR)  // 水位阈值存储地址

// 参数默认值
#define DEFAULT_WATER_THRESHOLD  70  // 默认水位阈值为70%

// 函数声明
HAL_StatusTypeDef FLASH_Init(void);
HAL_StatusTypeDef FLASH_ReadParam(uint32_t address, uint16_t *data);
HAL_StatusTypeDef FLASH_WriteParam(uint32_t address, uint16_t data);
uint16_t FLASH_GetWaterThreshold(void);
HAL_StatusTypeDef FLASH_SetWaterThreshold(uint16_t threshold);

#endif /* __FLASH_H */
