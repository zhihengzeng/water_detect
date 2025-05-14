#ifndef __4G_H
#define __4G_H

#include "main.h"
#include "usart.h"

// 接收缓冲区大小
#define UART_RX_BUFFER_SIZE 512

// 接收状态
typedef enum {
    UART_IDLE,       // 空闲状态
    UART_RECEIVING,  // 正在接收数据
    UART_RECEIVED    // 数据接收完成
} UART_ReceiveState;

// 天气信息结构体
typedef struct {
    char city[32];      // 城市名称
    char text[32];      // 天气描述
    char temperature[8]; // 温度
    uint8_t updated;    // 数据是否已更新
} Weather_TypeDef;

extern Weather_TypeDef g4_weather;

// 函数声明
void G4_Init(void);
void G4_SendCmd(const char* cmd);
uint8_t G4_IsDataReceived(void);
void G4_ProcessData(void);
void G4_ClearBuffer(void);
uint16_t G4_GetDataLength(void);
uint8_t* G4_GetRxBuffer(void);

extern uint8_t g4_connected;  // 全局4G连接状态标志
void G4_CheckConnectionStatus(void);

// 添加天气相关函数声明
void G4_GetWeather(void);
uint8_t G4_ParseWeatherJson(const char* json_data);

#endif /* __4G_H */
