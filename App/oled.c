#include "oled.h"
#include "oledfont.h"
#include "gpio.h"
#include "spi.h"

// OLED缓存区，用于存储显示内容
static uint8_t OLED_Buffer[128][8];

/**
 * @brief OLED写命令
 * @param cmd 命令字节
 */
void OLED_Write_Command(uint8_t cmd)
{
    HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_RESET); // 命令模式
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);                  // 发送命令
}

/**
 * @brief OLED写数据
 * @param data 数据字节
 */
void OLED_Write_Data(uint8_t data)
{
    HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET);   // 数据模式
    HAL_SPI_Transmit(&hspi2, &data, 1, HAL_MAX_DELAY);                 // 发送数据
}

/**
 * @brief OLED初始化
 * @note 包括硬件复位和寄存器配置
 */
void OLED_Init(void)
{
    HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, GPIO_PIN_SET);
    
    OLED_Write_Command(0xAE); // 关闭显示
    OLED_Write_Command(0xD5); // 设置显示时钟分频比/振荡器频率
    OLED_Write_Command(0x80);
    OLED_Write_Command(0xA8); // 设置多路复用率
    OLED_Write_Command(0x3F);
    OLED_Write_Command(0xD3); // 设置显示偏移
    OLED_Write_Command(0x00);
    OLED_Write_Command(0x40); // 设置显示开始行
    OLED_Write_Command(0x8D); // 充电泵设置
    OLED_Write_Command(0x14);
    OLED_Write_Command(0x20); // 设置内存地址模式
    OLED_Write_Command(0x02);
    OLED_Write_Command(0xA1); // 段重定义
    OLED_Write_Command(0xC8); // COM扫描方向
    OLED_Write_Command(0xDA); // COM硬件配置
    OLED_Write_Command(0x12);
    OLED_Write_Command(0x81); // 对比度设置
    OLED_Write_Command(0xCF);
    OLED_Write_Command(0xD9); // 设置预充电周期
    OLED_Write_Command(0xF1);
    OLED_Write_Command(0xDB); // 设置VCOMH取消选择级别
    OLED_Write_Command(0x30);
    OLED_Write_Command(0xA4); // 输出遵循RAM内容
    OLED_Write_Command(0xA6); // 设置正常显示
    OLED_Write_Command(0xAF); // 开启显示
    
    OLED_Clear();             // 清屏
}

/**
 * @brief OLED清屏
 * @note 将显存全部清零并更新显示
 */
void OLED_Clear(void)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        OLED_Write_Command(0xb0 + i); // 设置页地址
        OLED_Write_Command(0x00);     // 设置列地址低4位
        OLED_Write_Command(0x10);     // 设置列地址高4位
        
        for(uint8_t j = 0; j < 128; j++)
        {
            OLED_Write_Data(0x00);
            OLED_Buffer[j][i] = 0x00;
        }
    }
}

/**
 * @brief 设置OLED显示位置
 * @param x 列地址(0-127)
 * @param y 页地址(0-7)
 */
void OLED_SetPos(uint8_t x, uint8_t y)
{
    OLED_Write_Command(0xb0 + y);
    OLED_Write_Command(((x & 0xf0) >> 4) | 0x10);
    OLED_Write_Command(x & 0x0f);
}

/**
 * @brief 显示单个ASCII字符
 * @param x 起始列地址(0-127)
 * @param y 起始页地址(0-7)
 * @param chr 要显示的字符
 * @param size 字体大小(8/16)
 * @note size=8时使用6x8字体，size=16时使用8x16字体
 */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size)
{
    uint8_t i;
    chr = chr - ' ';  // 减去空格的ASCII码值(32)，得到字模数组的索引
    
    if(size == 16)
    {
        OLED_SetPos(x, y);
        // 写入上半部分(8字节)
        for(i = 0; i < 8; i++)
            OLED_Write_Data(F8X16[chr*16 + i]);
        
        OLED_SetPos(x, y + 1);
        // 写入下半部分(8字节)
        for(i = 8; i < 16; i++)
            OLED_Write_Data(F8X16[chr*16 + i]);
    }
    else if(size == 8)
    {
        OLED_SetPos(x, y);
        // 写入6x8字体数据
        for(i = 0; i < 6; i++)
            OLED_Write_Data(F6x8[chr][i]);
    }
}

/**
 * @brief 显示ASCII字符串
 * @param x 起始列地址(0-127)
 * @param y 起始页地址(0-7)
 * @param str 要显示的字符串
 * @param size 字体大小(8/16)
 * @note 超出屏幕范围自动换行
 */
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
    while(*str != '\0')
    {
        if(x > 128 - size/2) // 如果到达了行尾
        {
            x = 0;
            y += 2;  // 自动换行，每行占用2页
        }
        OLED_ShowChar(x, y, *str, size);
        x += (size/2);  // 移动光标位置，8x16字体占8列，6x8字体占6列
        str++;
    }
}

/**
 * @brief 显示中文字符
 * @param x 起始列地址(0-127)
 * @param y 起始页地址(0-7)
 * @param index 中文字符在字库中的索引
 * @note 使用16x16点阵中文字库
 */
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t index)
{
    uint8_t i;
    
    OLED_SetPos(x, y);
    // 写入上半部分(16字节)
    for(i = 0; i < 16; i++)
        OLED_Write_Data(Hzk[index][i]);
    
    OLED_SetPos(x, y + 1);
    // 写入下半部分(16字节)
    for(i = 16; i < 32; i++)
        OLED_Write_Data(Hzk[index][i]);
}

/**
 * @brief 刷新OLED显示
 * @note 将OLED_Buffer中的内容更新到屏幕
 */
void OLED_Refresh(void)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        OLED_SetPos(0, i);
        for(uint8_t j = 0; j < 128; j++)
        {
            OLED_Write_Data(OLED_Buffer[j][i]);
        }
    }
}