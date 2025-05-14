#include "timer.h"
#include "gpio.h"

// 添加全局状态变量
uint8_t g4_connected = 0;
uint8_t g4_upload_flag = 0;
uint8_t print_adc_value_flag = 0;

// 1ms中断计数器
static volatile uint16_t timer_counter_1s = 0;
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
        static uint16_t counter_1s = 0;
        counter_1s++;
        if (counter_1s >= 1000)
        {
            counter_1s = 0;
            timer_1s_flag = 1;
        }

        // 100ms定时器标志
        static uint16_t counter_100ms = 0;
        static uint8_t high_count = 0;
        counter_100ms++;
        if (counter_100ms >= 100)
        {
            counter_100ms = 0;
            // 检测COMM4_SAT_GPIO状态
            if (HAL_GPIO_ReadPin(COMM4_SAT_GPIO_Port, COMM4_SAT_Pin) == GPIO_PIN_SET) {
                high_count++;
                if (high_count >= 5)  // 连续5次检测到高电平
                {
                    if (g4_connected == 0) SEGGER_RTT_printf(0, "4G connected\n");
                    g4_connected = 1;
                    high_count = 5;  // 限制计数器最大值
                }
            }
            else {
                if (g4_connected == 1) SEGGER_RTT_printf(0, "4G disconnected\n");
                high_count = 0;
                g4_connected = 0;
            }

            print_adc_value_flag = 1;
        }

        // 20s定时器标志
        static uint16_t counter_20s = 0;
        counter_20s++;
        if (counter_20s >= 20000)
        {
            counter_20s = 0;
            g4_upload_flag = 1;
        }
    }
}
