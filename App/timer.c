#include "timer.h"

// 1ms中断计数器
static volatile uint16_t timer_ms_counter = 0;
// 1s定时标志
volatile uint8_t timer_1s_flag = 0;
/**
 * @brief 启动定时器
 */
void TIMER_Start(void)
{
    // 启动定时器中断
    HAL_TIM_Base_Start_IT(&htim2); // 假设使用TIM2，请根据实际使用的定时器修改
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
        // 1ms中断计数
        timer_ms_counter++;
        
        // 当计数达到1000ms时，设置1s标志并重置计数器
        if(timer_ms_counter >= 1000)
        {
            timer_1s_flag = 1;
            timer_ms_counter = 0;
        }
    }
}
