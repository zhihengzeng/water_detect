#include "timer.h"

// 1ms中断计数器
static volatile uint16_t timer_ms_counter = 0;
// 1s定时标志
volatile uint8_t timer_1s_flag = 0;
// 系统毫秒计数器
volatile uint32_t system_ms = 0;

/**
 * @brief 启动定时器
 */
void TIMER_Start(void)
{
    // 启动定时器中断
    HAL_TIM_Base_Start_IT(&htim2); // 假设使用TIM2，请根据实际使用的定时器修改
}

/**
 * @brief 获取系统毫秒计数
 * @retval 系统毫秒计数
 */
uint32_t TIMER_GetTick(void)
{
    return system_ms;
}

/**
 * @brief 定时器中断回调函数
 * @param htim 定时器句柄
 * @note 该函数会被HAL库在定时器中断发生时自动调用
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // 检查是否是我们期望的定时器
    if(htim->Instance == TIM2) // 假设使用TIM2，请根据实际使用的定时器修改
    {
        // 更新系统毫秒计数
        system_ms++;
        
        // 1s定时器标志
        static uint16_t ms_counter = 0;
        ms_counter++;
        if (ms_counter >= 1000)
        {
            ms_counter = 0;
            timer_1s_flag = 1;
        }
    }
}
