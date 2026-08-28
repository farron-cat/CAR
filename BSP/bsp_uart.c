#include "STC8G_H_GPIO.h"
#include "STC8G_H_UART.h"
#include "STC8G_H_NVIC.h"
#include "STC8G_H_Switch.h"

unsigned char UART1_RxFlag = 0; // 串口1接收完成标志位

void UART1Init(void)
{
    // 配置结构体定义
    GPIO_InitTypeDef GPIO_UART1_P3_PU;
    COMx_InitDefine UART1_8BIT_BRTX;

    // 配置串口引脚 P3.0 P3.1 为准双向模式
    GPIO_UART1_P3_PU.Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_UART1_P3_PU.Mode = GPIO_PullUp;
    GPIO_Inilize(GPIO_P3, &GPIO_UART1_P3_PU);

    // 配置串口1为8位数据，波特率115600，
    UART1_8BIT_BRTX.UART_Mode = UART_8bit_BRTx;  // 模式：8位数据，波特率可变
    UART1_8BIT_BRTX.UART_BRT_Use = BRT_Timer2;   // 波特率发生器：定时器2（Timer1用于数码管扫描）
    UART1_8BIT_BRTX.UART_BaudRate = 115200;      // 波特率：115200
    UART1_8BIT_BRTX.UART_RxEnable = ENABLE;      // 接收允许：允许
    UART1_8BIT_BRTX.BaudRateDouble = DISABLE;    // 波特率加倍：不使用
    UART_Configuration(UART1, &UART1_8BIT_BRTX); // 初始化UART1

    NVIC_UART1_Init(ENABLE, Priority_1); // 中断使能

    UART1_SW(UART1_SW_P30_P31); // 使用P3.0 P3.1作为串口1的引脚 (默认)
}

// 判断一组数据是否接收完成
void UART1RxProcess(void)
{
    if (COM1.RX_TimeOut > 0)
    {
        if (--COM1.RX_TimeOut == 0)
        {
            UART1_RxFlag = 1;
        }
    }
}

// 将指定缓冲区中的数据依次发送出去
void UART1SendBuffer(u8 *buf, u8 len)
{
    u8 i;

    for (i = 0; i < len; i++)
    {
        TX1_write2buff(buf[i]);
    }
}