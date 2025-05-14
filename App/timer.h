#ifndef __TIMER_H
#define __TIMER_H

#include "main.h"
#include "tim.h"

// 添加全局状态变量
extern uint8_t g4_connected;
extern uint8_t g4_upload_flag;
extern uint8_t print_adc_value_flag;
// 声明1s定时标志
extern volatile uint8_t timer_1s_flag;
// 声明系统毫秒计数器
extern volatile uint32_t system_ms;

// 启动定时器函数
void TIMER_Start(void);
// 获取系统毫秒计数函数
uint32_t TIMER_GetTick(void);

#endif /* __TIMER_H */
