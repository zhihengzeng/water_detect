#include "water.h"
#include "4G.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>

// 定义ADC采样缓冲区
static uint16_t adc_buffer[ADC_BUFFER_SIZE];
static volatile uint8_t data_ready_flag = 0;
static uint16_t adc_filtered_value = 0;
static uint8_t water_level = 0;
static uint8_t previous_water_level = 0;

// 页面管理相关变量
static DisplayPage_TypeDef current_page = PAGE_WATER;
static uint32_t page_timer = 0;
static uint8_t page_lock = 0;  // 锁定标志，1表示锁定在水位页面
static uint8_t water_stable_counter = 0; // 水位稳定计数器

// 外部变量引用
extern volatile uint32_t system_ms; // 系统毫秒计数，假设由定时器中断维护
extern uint8_t g4_connected;  // 全局4G连接状态标志
extern Weather_TypeDef g4_weather;  // 全局4G天气数据结构体

/**
  * @brief  初始化水位检测模块
  * @retval None
  */
void WATER_Init(void)
{
    // 清空ADC缓冲区
    memset(adc_buffer, 0, sizeof(adc_buffer));
    
    // 启动ADC和DMA传输，使用循环模式
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_BUFFER_SIZE);
    
    // 清屏
    OLED_Clear();
    
    // 初始化变量
    current_page = PAGE_WATER;
    page_timer = TIMER_GetTick();
    page_lock = 0;
    water_stable_counter = 0;
    
    // 显示水位页面
    WATER_DisplayWaterPage();
}

/**
  * @brief  水位检测处理函数，在主循环中调用
  * @retval None
  */
void WATER_Process(void)
{
    // 判断是否有新数据可以处理
    if (data_ready_flag)
    {
        data_ready_flag = 0;
        
        // 保存上一次水位值用于比较
        previous_water_level = water_level;
        
        // 计算中位数 (中位数滤波)
        uint16_t temp_buffer[ADC_BUFFER_SIZE];
        // 复制数据到临时缓冲区
        for (int i = 0; i < ADC_BUFFER_SIZE; i++)
        {
            temp_buffer[i] = adc_buffer[i];
        }

        // 冒泡排序
        for (int i = 0; i < ADC_BUFFER_SIZE - 1; i++)
        {
            for (int j = 0; j < ADC_BUFFER_SIZE - i - 1; j++)
            {
                if (temp_buffer[j] > temp_buffer[j + 1])
                {
                    uint16_t temp = temp_buffer[j];
                    temp_buffer[j] = temp_buffer[j + 1];
                    temp_buffer[j + 1] = temp;
                }
            }
        }

        // 取中位数
        adc_filtered_value = temp_buffer[ADC_BUFFER_SIZE / 2];
        
        // 根据ADC值计算水位
        water_level = WATER_GetLevel(adc_filtered_value);

        if (print_adc_value_flag)
        {
            // SEGGER_RTT_printf(0, "%d\n", adc_filtered_value);
            print_adc_value_flag = 0;
        }
        
        // 检查水位是否变化
        if (water_level != previous_water_level)
        {
            // 水位变化，锁定在水位页面
            page_lock = 1;
            water_stable_counter = 0;
            
            // 如果当前不是水位页面，则切换到水位页面
            if (current_page != PAGE_WATER)
            {
                WATER_SwitchPage(PAGE_WATER);
                WATER_DisplayWaterPage(); // 切换后显示完整水位页面
            }
            else if (current_page == PAGE_WATER)
            {
                // 更新整个水位页面而不只是数值
                WATER_DisplayWaterPage();
            }
        }
        else
        {
            // 水位未变化，增加稳定计数
            if (page_lock && water_stable_counter < WATER_STABLE_COUNT)
            {
                water_stable_counter++;
                
                // 达到稳定阈值，解除锁定
                if (water_stable_counter >= WATER_STABLE_COUNT)
                {
                    page_lock = 0;
                    page_timer = TIMER_GetTick(); // 重置页面计时器
                }
            }
        }
    }
    
    // 处理页面切换
    WATER_PageManager();
}

/**
  * @brief  根据ADC值计算水位百分比
  * @param  adc_value: ADC采样值
  * @retval 水位百分比 (0-100)
  */
uint8_t WATER_GetLevel(uint16_t adc_value)
{
    uint8_t ans;
    
    if (adc_value >= 2450) ans = 100;
    else if (adc_value >= 1800) ans = 90;
    else if (adc_value >= 1150) ans = 80;
    else if (adc_value >= 830) ans = 70;
    else if (adc_value >= 600) ans = 60;
    else if (adc_value >= 500) ans = 50;
    else if (adc_value >= 430) ans = 40;
    else if (adc_value >= 380) ans = 30;
    else if (adc_value >= 320) ans = 20;
    else if (adc_value >= 270) ans = 10;
    else ans = 0;
    
    return ans;

}

/**
  * @brief  显示时间页面
  * @param  time: RTC时间结构体指针
  * @retval None
  */
void WATER_DisplayTimePage(RTC_TimeTypeDef *time)
{
    char buffer[32];
    uint8_t x_pos;
    
    // 第一行：日期（居中）
    sprintf(buffer, "20%02d-%02d-%02d", time->year, time->month, time->day);
    x_pos = (128 - strlen(buffer) * 8) / 2;
    OLED_ShowString(x_pos, 0, buffer, 16);
    
    // 第二行：时间 星期缩写（居中）
    const char* week_days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    sprintf(buffer, "%02d:%02d:%02d %s", time->hour, time->minute, time->second, week_days[time->week % 7]);
    x_pos = (128 - strlen(buffer) * 8) / 2;
    OLED_ShowString(x_pos, 2, buffer, 16);
    
    // 第三行：城市（居中）
    if (g4_weather.updated) {
        // 已获取天气数据
        sprintf(buffer, "%s", g4_weather.city);
        x_pos = (128 - strlen(buffer) * 8) / 2;
        OLED_ShowString(x_pos, 4, buffer, 16);
        
        // 第四行：天气状况 气温（居中）
        sprintf(buffer, "%s %sC", g4_weather.text, g4_weather.temperature);
        x_pos = (128 - strlen(buffer) * 8) / 2;
        OLED_ShowString(x_pos, 6, buffer, 16);
    } else {
        // 未获取天气数据
        strcpy(buffer, "Weather Loading...");
        x_pos = (128 - strlen(buffer) * 8) / 2;
        OLED_ShowString(x_pos, 4, buffer, 16);
        
        OLED_ShowString(0, 6, "                ", 16); // 清空第四行
    }
}

/**
  * @brief  显示水位页面
  * @retval None
  */
void WATER_DisplayWaterPage(void)
{
    char buffer[32];
    uint16_t threshold = FLASH_GetWaterThreshold();
    uint8_t x_pos;
    
    // 显示水位页面标题（包含4G状态，居中显示）
    if (g4_connected)
    {
        strcpy(buffer, "Water Level 4G+");
    }
    else
    {
        strcpy(buffer, "Water Level 4G-");
    }
    // 计算居中位置 (128 - 字符数*8) / 2
    // 16像素字体下ASCII字符宽度为8像素
    x_pos = (128 - strlen(buffer) * 8) / 2;
    OLED_ShowString(x_pos, 0, buffer, 16);
    
    // 显示当前水位百分比和与阈值的比较关系（居中）
    if (water_level >= threshold)
    {
        sprintf(buffer, "%3d%% >= %3d%%", water_level, threshold);
    }
    else
    {
        sprintf(buffer, "%3d%% < %3d%% ", water_level, threshold);
    }
    x_pos = (128 - strlen(buffer) * 8) / 2;
    OLED_ShowString(x_pos, 2, buffer, 16);
    
    // 显示水位条形图边框（居中）
    strcpy(buffer, "[          ]");
    x_pos = (128 - strlen(buffer) * 8) / 2;
    OLED_ShowString(x_pos, 4, buffer, 16);
    
    // 水位条形图填充
    uint8_t bar_length = water_level / 10;
    memset(buffer, 0, sizeof(buffer));
    buffer[0] = '[';
    for (int i = 0; i < 10; i++)
    {
        if (i < bar_length)
            buffer[i+1] = '=';
        else
            buffer[i+1] = ' ';
    }
    buffer[11] = ']';
    buffer[12] = '\0';
    
    OLED_ShowString(x_pos, 4, buffer, 16);
    
    // 如果水位超过阈值，显示报警信息（居中）
    if (water_level >= threshold)
    {
        strcpy(buffer, "!!! ALARM !!!");
        x_pos = (128 - strlen(buffer) * 8) / 2;
        OLED_ShowString(x_pos, 6, buffer, 16);
    }
    else
    {
        // 清除报警信息行
        OLED_ShowString(0, 6, "               ", 16);
    }
}

/**
  * @brief  页面管理器，处理页面切换逻辑
  * @retval None
  */
void WATER_PageManager(void)
{
    // 如果当前页面被锁定，则不进行切换
    if (page_lock)
    {
        return;
    }
    
    // 检查是否达到页面切换时间
    if ((TIMER_GetTick() - page_timer) >= PAGE_SWITCH_TIME)
    {
        // 先切换页面状态（会清屏）
        if (current_page == PAGE_WATER)
        {
            // 切换到时间页面
            WATER_SwitchPage(PAGE_TIME);
            
            // 显示时间内容
            RTC_TimeTypeDef time;
            if (PCF8563_GetTime(&time) == HAL_OK)
            {
                // 修改DisplayTimePage函数删除内部的OLED_Clear调用
                WATER_DisplayTimePage(&time);
            }
        }
        else
        {
            // 切换到水位页面
            WATER_SwitchPage(PAGE_WATER);
            
            // 修改DisplayWaterPage函数删除内部的OLED_Clear调用
            WATER_DisplayWaterPage();
        }
    }
}

/**
  * @brief  切换页面
  * @param  page: 目标页面
  * @retval None
  */
void WATER_SwitchPage(DisplayPage_TypeDef page)
{
    // 清屏，防止页面内容重叠
    OLED_Clear();
    
    // 更新页面状态
    current_page = page;
    page_timer = TIMER_GetTick(); // 重置页面计时器
}

/**
  * @brief  ADC DMA转换完成回调函数
  * @param  hadc: ADC句柄
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
        // 设置数据就绪标志
        data_ready_flag = 1;
    }
}

DisplayPage_TypeDef WATER_GetCurrentPage(void)
{
    return current_page;
}

uint8_t WATER_IsPageLocked(void)
{
    return page_lock;
}

uint8_t WATER_GetCurrentLevel(void)
{
    return water_level;
}
