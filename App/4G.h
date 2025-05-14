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

// MQTT相关状态
typedef enum {
    MQTT_DISCONNECTED,   // 未连接
    MQTT_CONNECTING,     // 正在连接
    MQTT_CONNECTED       // 已连接
} MQTT_State;

extern Weather_TypeDef g4_weather;
extern MQTT_State g4_mqtt_state;

// 函数声明
void G4_Init(void);
void G4_SendCmd(const char* cmd);
uint8_t G4_IsDataReceived(void);
void G4_ProcessData(void);
void G4_ClearBuffer(void);
uint16_t G4_GetDataLength(void);
uint8_t* G4_GetRxBuffer(void);

// 添加天气相关函数声明
HAL_StatusTypeDef G4_GetWeather(void);
uint8_t G4_ParseWeatherJson(const char* json_data);

// 添加MQTT相关函数声明
HAL_StatusTypeDef G4_InitMQTT(uint8_t force_wait);
HAL_StatusTypeDef G4_UploadData(void);
void G4_ProcessMQTTData(const char* data, uint16_t len);

#endif /* __4G_H */
