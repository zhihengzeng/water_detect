#ifndef __TIMER_H
#define __TIMER_H

#include "main.h"
#include "tim.h"

// 声明1s定时标志
extern volatile uint8_t timer_1s_flag;
// 声明启动定时器函数
void TIMER_Start(void);

#endif /* __TIMER_H */
