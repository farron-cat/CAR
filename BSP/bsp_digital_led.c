#include "STC8H.H"
#include "STC8G_H_GPIO.h"
#include "bsp_digital_led.h"
#include "bsp_delay.h"

// 段码表
unsigned char code segCodeTable[16] = {SEG_0, SEG_1, SEG_2, SEG_3, SEG_4, SEG_5, SEG_6, SEG_7, SEG_8, SEG_9, SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F};
// 位码表
unsigned char code digCodeTable[8] = {DIG_1, DIG_2, DIG_3, DIG_4, DIG_5, DIG_6, DIG_7, DIG_8};

// 数码管显示缓冲（8位），存放各位置的段码值，由Timer1中断扫描刷新
// 默认全部熄灭（SEG_CLR = 0xFF，共阳数码管灭段）
unsigned char displayBuffer[8] = {SEG_CLR, SEG_CLR, SEG_CLR, SEG_CLR, SEG_CLR, SEG_CLR, SEG_CLR, SEG_CLR};

void DigitalLEDInit(void)
{
    // 配置P4.2、P4.3、P4.4为推挽输出
    GPIO_InitTypeDef GPIO_DIGLED_P4_PP;
    GPIO_DIGLED_P4_PP.Mode = GPIO_OUT_PP;
    GPIO_DIGLED_P4_PP.Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_Inilize(GPIO_P4, &GPIO_DIGLED_P4_PP);

    // 初始状态
    SER = 0;
    RCLK = 0;
    SRCLK = 0;
}

void SendByte(unsigned char dat)
{
    int i;
    // 段码
    for (i = 0; i < 8; i++)
    {
        // 从左到右取位：先取 bit7，再 bit6 ... 最后 bit0
        SER = (dat >> (7 - i)) & 0x01;

        // 产生移位时钟上升沿，数据被移入
        SRCLK = 0;
        SRCLK = 1;
    }
}

void SendCode(unsigned char segCode, unsigned char digCode)
{
    // 段码
    SendByte(segCode);

    // 位码
    SendByte(digCode);

    // 所有位移完后，产生锁存脉冲
    RCLK = 0;
    RCLK = 1;
}

/**
 * @brief 在8位数码管上显示一个 unsigned long int 数值（消去前导零）
 * @param num 待显示的数值（0 ~ 4,294,967,295）
 * @note 依赖宏定义: SEG_0~SEG_9, DIG_1~DIG_8, 以及函数 SendCode(), delay_ms()
 */
void DisplayNumber(unsigned long int num)
{
    unsigned char i;
    unsigned char showFlag = 0; // 是否已开始显示有效数字（非零或后续位）
    unsigned char digits[8];    // 存储每位数字（0~9）
    unsigned long temp = num;   // 提升为32位，避免除法截断（STC8H中int为16位）

    // 段码表（引用已有宏）
    unsigned char code segTable[10] = {
        SEG_0, SEG_1, SEG_2, SEG_3, SEG_4,
        SEG_5, SEG_6, SEG_7, SEG_8, SEG_9};
    // 位码表（从最高位到最低位）
    unsigned char code digTable[8] = {
        DIG_1, DIG_2, DIG_3, DIG_4,
        DIG_5, DIG_6, DIG_7, DIG_8};

    // 1. 提取每一位数字，存入 digits[0]（最高位）~ digits[7]（最低位）
    for (i = 0; i < 8; i++)
    {
        digits[7 - i] = temp % 10; // 从低位取余，逆序放入高位
        temp /= 10;
    }

    // 2. 逐位扫描显示，同时消隐前导零
    for (i = 0; i < 8; i++)
    {
        // 若当前位不为零，或已经遇到过非零位，则标记开始显示
        if (digits[i] != 0 || showFlag)
        {
            showFlag = 1; // 从此位开始，后续均显示
            SendCode(segTable[digits[i]], digTable[i]);
        }
        else
        {
            // 当前位为零且尚未显示任何非零位（即前导零）
            if (i != 7)
            {
                SendCode(0xFF, digTable[i]); // 灭掉该位（全暗）
            }
            else
            {
                // 所有位均为零，最低位显示 0
                SendCode(segTable[0], digTable[i]);
            }
        }

        // 使用更小的延时，修复了闪烁的问题
        // 可能会导致亮度变低
        delay_us(100); // 扫描延时，稳定显示
    }
}
