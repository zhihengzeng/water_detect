#include "4G.h"
#include "water.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>

// 定义接收缓冲区
static uint8_t g4_rx_buffer[UART_RX_BUFFER_SIZE];
static uint16_t g4_rx_len = 0;
static UART_ReceiveState g4_rx_state = UART_IDLE;

// 假设4G模块连接到USART2，如果不是，请修改以下宏定义
#define G4_UART &huart2
#define G4_UART_HANDLE huart2

// 在4G.c顶部添加全局变量
Weather_TypeDef g4_weather = {0};

MQTT_State g4_mqtt_state = MQTT_DISCONNECTED;

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
    
    // 标记MQTT未连接
    g4_mqtt_state = MQTT_DISCONNECTED;
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
        
        SEGGER_RTT_printf(0, "receive(%d bytes): %s\n", rx_len, (char*)rx_data);
        
        // 检查是否是MQTT消息
        if (g4_mqtt_state == MQTT_CONNECTED) {
            G4_ProcessMQTTData((const char *)rx_data, rx_len);
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

// 修改为阻塞式获取天气函数
HAL_StatusTypeDef G4_GetWeather(void)
{  
    // 构建HTTP GET请求
    const char* request = "GET https://api.seniverse.com/v3/weather/now.json?key=SQNlQOMv_LBPmUraM&location=Guilin&language=en&unit=c\r\n";
    
    // 发送请求前清空状态
    g4_weather.updated = 0;

    // 显示初始化中 
    OLED_Clear();
    char title[] = "City Weather";
    uint8_t title_x = (128 - strlen(title) * 8) / 2;
    OLED_ShowString(title_x, 2, title, 16);
    OLED_ShowString(24, 4, "Getting", 16);

    // 发送初始化命令
    G4_SendCmd("AT+DTUTASK=\"1\",\"10\"\r\n");
    HAL_Delay(200); // 等待响应
    
    G4_SendCmd("AT+REST\r\n");
    HAL_Delay(200); // 等待模块复位

    
    uint32_t retry_count = 0;
    uint8_t first_time = 1;
    // 等待接收数据时添加动态效果
    uint8_t dots = 0, cnt = 0;
    char dot_text[4] = {0};
    
    while (!g4_weather.updated)
    {
        // 超时重发
        if (g4_connected == 1 && (first_time || ++retry_count % 20 == 0)) {
            retry_count = 0;
            first_time = 0;
            // 发送请求
            G4_SendCmd(request);
            SEGGER_RTT_printf(0, "send weather request\n");
        }

        // 处理接收到的数据
        if (G4_IsDataReceived())
        {
            // 获取接收缓冲区和长度
            uint8_t* rx_data = G4_GetRxBuffer();
            uint16_t rx_len = G4_GetDataLength();
            
            SEGGER_RTT_printf(0, "receive(%d bytes): %s\n", rx_len, (char*)rx_data);
            
            // 尝试解析JSON数据
            if (strstr((char*)rx_data, "\"results\":")) {
                if (G4_ParseWeatherJson((char*)rx_data)) {
                    // 解析成功
                    G4_ClearBuffer();
                    return HAL_OK;
                }
            }
            
            // 清除缓冲区，准备继续接收
            // G4_ClearBuffer();
            g4_rx_state = UART_IDLE;
        }
        
        // 小延时，避免过度占用CPU
        HAL_Delay(100);

        // 每200ms更新一次动画
        if (++cnt >= 2) {
            cnt = 0;
            
            // 更新动态点数量 (0-3)
            dots = (dots + 1) % 4;
            
            // 生成点的文本
            memset(dot_text, 0, sizeof(dot_text));
            for (int i = 0; i < dots; i++) {
                dot_text[i] = '.';
            }
            
            // 更新显示点
            OLED_ShowString(80, 4, "   ", 16); // 清除之前的点
            OLED_ShowString(80, 4, dot_text, 16);
        }
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

/**
  * @brief  初始化MQTT连接
  * @param  force_wait: 是否强制等待连接建立
  * @retval HAL状态
  */
HAL_StatusTypeDef G4_InitMQTT(uint8_t force_wait)
{   
    // 设置MQTT模式
    g4_mqtt_state = MQTT_CONNECTING;

    // 显示初始化中 
    OLED_Clear();
    char title[] = "aliyun MQTT";
    uint8_t title_x = (128 - strlen(title) * 8) / 2;
    OLED_ShowString(title_x, 2, title, 16);
    OLED_ShowString(16, 4, "Connecting", 16);
    
    // 发送切换到MQTT模式命令
    G4_SendCmd("AT+DTUTASK=\"1\",\"20\"\r\n");
    HAL_Delay(200); // 等待响应
    
    G4_SendCmd("AT+REST\r\n");
    HAL_Delay(200); // 等待模块复位
    
    SEGGER_RTT_printf(0, "send MQTT initialization command\n");
    
    // 等待连接建立，设置30秒超时
    uint32_t start_time = TIMER_GetTick();
    uint32_t timeout = 30000; // 30秒超时
    // 等待接收数据时添加动态效果
    uint8_t dots = 0, cnt = 0;
    char dot_text[4] = {0};
    
    while (g4_mqtt_state != MQTT_CONNECTED)
    {
        // 如果设备已连接，认为MQTT也连接上了
        if (g4_connected) {
            g4_mqtt_state = MQTT_CONNECTED;
            SEGGER_RTT_printf(0, "MQTT connected\n");
            return HAL_OK;
        }
        
        // 检查是否超时
        if (!force_wait && TIMER_GetTick() - start_time > timeout) {
            SEGGER_RTT_printf(0, "MQTT connection timeout\n");
            g4_mqtt_state = MQTT_DISCONNECTED;
            return HAL_TIMEOUT;
        }

        G4_ProcessData();
        
        // 小延时，避免过度占用CPU
        HAL_Delay(100);

        // 每200ms更新一次动画
        if (++cnt >= 2) {
            cnt = 0;
            
            // 更新动态点数量 (0-3)
            dots = (dots + 1) % 4;
            
            // 生成点的文本
            memset(dot_text, 0, sizeof(dot_text));
            for (int i = 0; i < dots; i++) {
                dot_text[i] = '.';
            }
            
            // 更新显示点
            OLED_ShowString(94, 4, "   ", 16); // 清除之前的点
            OLED_ShowString(94, 4, dot_text, 16);
        }
    }
    
    return HAL_OK;
}

/**
  * @brief  上传数据到阿里云
  * @retval HAL状态
  */
HAL_StatusTypeDef G4_UploadData(void)
{
    if (g4_mqtt_state != MQTT_CONNECTED) {
        return HAL_ERROR;
    }
    
    // 构建JSON数据，符合阿里云格式
    char json_data[256];
    if (g4_weather.updated) {
        sprintf(json_data, "{\"params\":{\"water_ratio\":%d,\"water_threshold\":%d,\"city\":\"%s\",\"weather\":\"%s\",\"temperature\":\"%s\"}}",
                WATER_GetCurrentLevel(),
                FLASH_GetWaterThreshold(),
                g4_weather.city,
                g4_weather.text,
                g4_weather.temperature);
    } else {
            sprintf(json_data, "{\"params\":{\"water_ratio\":%d,\"water_threshold\":%d}}",
                WATER_GetCurrentLevel(),
                FLASH_GetWaterThreshold());
    }

    // 发送JSON数据
    G4_SendCmd(json_data);
    SEGGER_RTT_printf(0, "upload data\n");
    // SEGGER_RTT_printf(0, "upload data: %s\n", json_data);
}

/**
  * @brief  处理MQTT接收到的数据
  * @param  data: 接收到的数据
  * @param  len: 数据长度
  * @retval None
  */
void G4_ProcessMQTTData(const char* data, uint16_t len)
{
    // 检查是否是阿里云的属性设置指令
    if (strstr(data, "\"method\":\"thing.service.property.set\"")) {
        // 查找水位阈值设置命令
        char* threshold_pos = strstr(data, "\"water_threshold\":");
        if (threshold_pos) {
            // 提取阈值值
            threshold_pos += 18; // 跳过 "water_threshold":
            int new_threshold = atoi(threshold_pos);
            
            // 验证阈值范围
            if (new_threshold > 0 && new_threshold <= 100) {
                // 更新Flash中的阈值
                if (FLASH_SetWaterThreshold(new_threshold) == HAL_OK) {
                    SEGGER_RTT_printf(0, "update water threshold: %d\n", new_threshold);
                } else {
                    SEGGER_RTT_printf(0, "update water threshold failed\n");
                }
            } else {
                SEGGER_RTT_printf(0, "invalid water threshold: %d\n", new_threshold);
            }
        }
    }
}
