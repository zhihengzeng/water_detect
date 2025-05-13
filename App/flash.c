#include "flash.h"

/**
  * @brief  初始化Flash参数存储
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_Init(void)
{
    uint16_t threshold;
    HAL_StatusTypeDef status;
    
    // 读取水位阈值
    status = FLASH_ReadParam(FLASH_PARAM_WATER_THRESHOLD, &threshold);
    
    // 如果读取失败或值无效(0xFFFF表示擦除状态)，则写入默认值
    if (status != HAL_OK || threshold == 0xFFFF)
    {
        status = FLASH_SetWaterThreshold(DEFAULT_WATER_THRESHOLD);
    }
    
    return status;
}

/**
  * @brief  从Flash读取参数
  * @param  address: 参数存储地址
  * @param  data: 数据指针
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_ReadParam(uint32_t address, uint16_t *data)
{
    *data = *(__IO uint16_t*)address;
    return HAL_OK;
}

/**
  * @brief  向Flash写入参数
  * @param  address: 参数存储地址
  * @param  data: 要写入的数据
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_WriteParam(uint32_t address, uint16_t data)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError = 0;
    uint16_t currentData;
    
    // 首先读取当前值，如果相同则不需要写入
    FLASH_ReadParam(address, &currentData);
    if (currentData == data)
    {
        return HAL_OK;
    }
    
    // 解锁Flash
    HAL_FLASH_Unlock();
    
    // 擦除一页
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.PageAddress = FLASH_PARAM_ADDR;
    eraseInit.NbPages = 1;
    
    status = HAL_FLASHEx_Erase(&eraseInit, &pageError);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }
    
    // 写入数据
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address, data);
    
    // 锁定Flash
    HAL_FLASH_Lock();
    
    return status;
}

/**
  * @brief  获取水位阈值
  * @retval 水位阈值(百分比0-100)
  */
uint16_t FLASH_GetWaterThreshold(void)
{
    uint16_t threshold;
    
    if (FLASH_ReadParam(FLASH_PARAM_WATER_THRESHOLD, &threshold) != HAL_OK)
    {
        // 读取失败，返回默认值
        return DEFAULT_WATER_THRESHOLD;
    }
    
    // 确保阈值在有效范围内
    if (threshold > 100 || threshold == 0xFFFF)
    {
        return DEFAULT_WATER_THRESHOLD;
    }
    
    return threshold;
}

/**
  * @brief  设置水位阈值
  * @param  threshold: 水位阈值(百分比0-100)
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_SetWaterThreshold(uint16_t threshold)
{
    // 验证阈值范围
    if (threshold > 100)
    {
        threshold = 100;
    }
    
    return FLASH_WriteParam(FLASH_PARAM_WATER_THRESHOLD, threshold);
}
