#include "4G.h"
#include <string.h>
#include <stdio.h>

// 定义接收缓冲区
static uint8_t g4_rx_buffer[UART_RX_BUFFER_SIZE];
static uint16_t g4_rx_len = 0;
static UART_ReceiveState g4_rx_state = UART_IDLE;

// 假设4G模块连接到USART2，如果不是，请修改以下宏定义
#define G4_UART &huart2
#define G4_UART_HANDLE huart2

// 添加全局状态变量
uint8_t g4_connected = 0;

// 在4G.c顶部添加全局变量
Weather_TypeDef g4_weather = {0};

/**
  * @brief  初始化4G模块通信
  * @retval None
  */
void G4_Init(void)
{
    // 清空接收缓冲区
    memset(g4_rx_buffer, 0, UART_RX_BUFFER_SIZE);
    g4_rx_len = 0;
    g4_rx_state = UART_IDLE;
    
    // 使能串口接收空闲中断
    __HAL_UART_ENABLE_IT(G4_UART, UART_IT_IDLE);
    
    // 启动DMA接收
    HAL_UART_Receive_DMA(G4_UART, g4_rx_buffer, UART_RX_BUFFER_SIZE);
    
    // 标记天气数据未更新
    g4_weather.updated = 0;
    
    // 发送初始化命令
    G4_SendCmd("AT+DTUTASK=\"1\",\"10\"\r\n");
    HAL_Delay(1000); // 等待响应
    
    G4_SendCmd("AT+REST\r\n");
    HAL_Delay(2000); // 等待模块复位
    
    SEGGER_RTT_printf(0, "等待4G模块连接...\n");
}

/**
  * @brief  发送AT命令到4G模块
  * @param  cmd: 要发送的AT命令字符串
  * @retval None
  */
void G4_SendCmd(const char* cmd)
{
    // 清空接收缓冲区，准备接收新数据
    G4_ClearBuffer();
    
    // 发送命令
    HAL_UART_Transmit(G4_UART, (uint8_t*)cmd, strlen(cmd), 100);
    
    // 设置接收状态为接收中
    g4_rx_state = UART_RECEIVING;
}

/**
  * @brief  检查是否收到数据
  * @retval 1: 已收到数据，0: 未收到数据
  */
uint8_t G4_IsDataReceived(void)
{
    return (g4_rx_state == UART_RECEIVED) ? 1 : 0;
}

/**
  * @brief  获取接收数据长度
  * @retval 接收数据长度
  */
uint16_t G4_GetDataLength(void)
{
    return g4_rx_len;
}

/**
  * @brief  获取接收缓冲区指针
  * @retval 接收缓冲区指针
  */
uint8_t* G4_GetRxBuffer(void)
{
    return g4_rx_buffer;
}

/**
  * @brief  处理接收到的数据
  * @retval None
  */
void G4_ProcessData(void)
{
    // 处理4G模块接收到的数据
    if (G4_IsDataReceived())
    {
        // 获取接收缓冲区和长度
        uint8_t* rx_data = G4_GetRxBuffer();
        uint16_t rx_len = G4_GetDataLength();
        
        // 使用SEGGER_RTT打印接收数据
        SEGGER_RTT_printf(0, "receive(%d bytes): %s\n", rx_len, rx_data);
        
        // 尝试解析天气数据
        if (strstr((char*)rx_data, "\"results\":")) {
            G4_ParseWeatherJson((char*)rx_data);
        }
        
        // 清除缓冲区，准备下一次接收
        G4_ClearBuffer();
    }
}

/**
  * @brief  清空接收缓冲区
  * @retval None
  */
void G4_ClearBuffer(void)
{
    memset(g4_rx_buffer, 0, UART_RX_BUFFER_SIZE);
    g4_rx_len = 0;
    g4_rx_state = UART_IDLE;
    
    // 重启DMA接收
    HAL_UART_AbortReceive(G4_UART);
    HAL_UART_Receive_DMA(G4_UART, g4_rx_buffer, UART_RX_BUFFER_SIZE);
}

/**
  * @brief  串口中断回调函数(需要放在stm32f1xx_it.c或main.c中)
  * @param  huart: 串口句柄
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // DMA接收完成回调，通常不会触发，因为设置的接收长度比较大
    if (huart->Instance == G4_UART_HANDLE.Instance)
    {
        g4_rx_state = UART_RECEIVED;
        g4_rx_len = UART_RX_BUFFER_SIZE;
    }
}

/**
  * @brief  串口空闲中断处理
  * @note   需要在stm32f1xx_it.c的USARTx_IRQHandler中调用此函数
  * @retval None
  */
void G4_UART_IDLECallback(void)
{
    if (__HAL_UART_GET_FLAG(G4_UART, UART_FLAG_IDLE) != RESET)
    {
        // 清除空闲中断标志
        __HAL_UART_CLEAR_IDLEFLAG(G4_UART);
        
        // 停止DMA传输
        HAL_UART_DMAStop(G4_UART);
        
        // 计算接收到的数据长度
        g4_rx_len = UART_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(G4_UART_HANDLE.hdmarx);
        
        // 标记接收完成
        g4_rx_state = UART_RECEIVED;
        
        // 重启DMA接收
        HAL_UART_Receive_DMA(G4_UART, g4_rx_buffer, UART_RX_BUFFER_SIZE);
    }
}

/**
  * @brief  检查4G模块连接状态
  * @retval None
  */
void G4_CheckConnectionStatus(void)
{
  static uint32_t last_check_time = 0;
  static uint8_t high_count = 0;
  uint32_t current_time = TIMER_GetTick();
  
  // 每100ms检测一次
  if (current_time - last_check_time >= 100)
  {
    last_check_time = current_time;
    
    // 检测COMM4_SAT_GPIO状态
    if (HAL_GPIO_ReadPin(COMM4_SAT_GPIO_Port, COMM4_SAT_Pin) == GPIO_PIN_SET)
    {
      high_count++;
      if (high_count >= 5)  // 连续5次检测到高电平
      {
        if (g4_connected == 0)
        {
          SEGGER_RTT_printf(0, "4G connected\n");
        }
        g4_connected = 1;
        high_count = 5;  // 限制计数器最大值
      }
    }
    else
    {
      if (g4_connected == 1)
      {
        SEGGER_RTT_printf(0, "4G disconnected\n");
      }
      high_count = 0;
      g4_connected = 0;
    }
  }
}

// 修改为阻塞式获取天气函数
HAL_StatusTypeDef G4_GetWeather(void)
{  
    // 构建HTTP GET请求
    const char* request = "GET https://api.seniverse.com/v3/weather/now.json?key=SQNlQOMv_LBPmUraM&location=Guilin&language=en&unit=c\r\n";
    
    // 发送请求前清空状态
    g4_weather.updated = 0;
    
    // 发送请求
    G4_SendCmd(request);
    SEGGER_RTT_printf(0, "send weather request\n");
    
    // 等待接收和解析，设置10秒超时
    uint32_t start_time = TIMER_GetTick();
    uint32_t timeout = 10000; // 10秒超时
    
    while (!g4_weather.updated)
    {
        // 处理接收到的数据
        if (G4_IsDataReceived())
        {
            // 获取接收缓冲区和长度
            uint8_t* rx_data = G4_GetRxBuffer();
            uint16_t rx_len = G4_GetDataLength();
            
            SEGGER_RTT_printf(0, "receive(%d bytes)\n", rx_len);
            
            // 尝试解析JSON数据
            if (strstr((char*)rx_data, "\"results\":")) {
                if (G4_ParseWeatherJson((char*)rx_data)) {
                    // 解析成功
                    G4_ClearBuffer();
                    return HAL_OK;
                }
            }
            
            // 清除缓冲区，准备继续接收
            G4_ClearBuffer();
        }
        
        // 检查是否超时
        if (TIMER_GetTick() - start_time > timeout) {
            SEGGER_RTT_printf(0, "get weather timeout\n");
            return HAL_TIMEOUT;
        }
        
        // 小延时，避免过度占用CPU
        HAL_Delay(10);
    }
    
    return HAL_OK;
}

// 添加JSON解析函数
uint8_t G4_ParseWeatherJson(const char* json_data)
{
    // 简单的JSON解析逻辑，提取需要的字段
    char* name_pos = strstr(json_data, "\"name\":\"");
    char* text_pos = strstr(json_data, "\"text\":\"");
    char* temp_pos = strstr(json_data, "\"temperature\":\"");
    
    if (name_pos && text_pos && temp_pos) {
        // 提取城市名称
        name_pos += 8; // 跳过 "name":"
        int i = 0;
        while (*name_pos != '"' && i < sizeof(g4_weather.city)-1) {
            g4_weather.city[i++] = *name_pos++;
        }
        g4_weather.city[i] = '\0';
        
        // 提取天气描述
        text_pos += 8; // 跳过 "text":"
        i = 0;
        while (*text_pos != '"' && i < sizeof(g4_weather.text)-1) {
            g4_weather.text[i++] = *text_pos++;
        }
        g4_weather.text[i] = '\0';
        
        // 提取温度
        temp_pos += 15; // 跳过 "temperature":"
        i = 0;
        while (*temp_pos != '"' && i < sizeof(g4_weather.temperature)-1) {
            g4_weather.temperature[i++] = *temp_pos++;
        }
        g4_weather.temperature[i] = '\0';
        
        // 标记更新完成
        g4_weather.updated = 1;
        
        SEGGER_RTT_printf(0, "weather update success: %s, %s, %s°C\n", 
                         g4_weather.city, g4_weather.text, g4_weather.temperature);
        
        return 1;
    }
    
    SEGGER_RTT_printf(0, "parse weather data failed\n");
    return 0;
}
