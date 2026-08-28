#include "STC8G_H_GPIO.h"
#include "STC8G_H_UART.h"
#include "STC8G_H_NVIC.h"
#include "STC8G_H_Switch.h"

#include "bsp_bluetooth.h"

// 配置UART2
static void UART2_Config(void)
{
    COMx_InitDefine uart2_cfg;

    // 1. 配置核心通信参数（结构体只包含这些成员）
    uart2_cfg.UART_Mode = UART_8bit_BRTx; // 8位数据，波特率可变
    uart2_cfg.UART_BRT_Use = BRT_Timer2;  // 波特率发生器：定时器2
    uart2_cfg.UART_BaudRate = 115200ul;   // 波特率 115200
    uart2_cfg.UART_RxEnable = ENABLE;     // 使能接收
    uart2_cfg.BaudRateDouble = DISABLE;   // 波特率不加倍

    // 调用库函数初始化UART2
    UART_Configuration(UART2, &uart2_cfg);

    // 2. 使能UART2中断并设置优先级（单独函数配置）
    NVIC_UART2_Init(ENABLE, Priority_0); // 使能中断，优先级0

    // 3. 切换UART2引脚到 P1.0/P1.1（单独函数配置）
    UART2_SW(UART2_SW_P10_P11); // 使用P1.0/P1.1
}

// 配置GPIO
static void GPIO_Config(void)
{
    GPIO_InitTypeDef gpio_cfg;

    // ---- 配置 P4.6 (BT_EN) 为准双向 ----
    gpio_cfg.Pin = GPIO_Pin_6;
    gpio_cfg.Mode = GPIO_PullUp;
    GPIO_Inilize(GPIO_P4, &gpio_cfg);

    // ---- 配置 P0.6 (BT_STATE) 为为准双向
    gpio_cfg.Pin = GPIO_Pin_6;
    gpio_cfg.Mode = GPIO_PullUp;
    GPIO_Inilize(GPIO_P0, &gpio_cfg);

    // ---- 配置 P1.0 (RXD2) 和 P1.1 (TXD2) 为准双向 ----
    gpio_cfg.Pin = GPIO_Pin_0 | GPIO_Pin_1;
    gpio_cfg.Mode = GPIO_PullUp;
    GPIO_Inilize(GPIO_P1, &gpio_cfg);
}

// 初始化蓝牙模块
void BT_Init(void)
{
    GPIO_Config();  // 配置所有相关GPIO
    UART2_Config(); // 配置UART2
}

// uart1与uart2通信

void BT_UART_SENsD2BT(void)
{
    u8 index_i;

    if (COM1.RX_TimeOut > 0) // UART1 的接收到信息
    {
        // 超时计数
        if (--COM1.RX_TimeOut == 0)
        {
            if (COM1.RX_Cnt > 0)
            {
                for (index_i = 0; index_i < COM1.RX_Cnt; index_i++)
                {

                    TX2_write2buff(RX1_Buffer[index_i]); // 将UART1 接收到的送到UART2
                }
            }
            COM1.RX_Cnt = 0;
        }
    }
}

//
u8 BT_UART_RESFBT(void)
{
    u8 index_i = 0;

    if (COM2.RX_TimeOut > 0) // UART2 接收到蓝牙发送的信息
    {
        if (--COM2.RX_TimeOut == 0)
        {
            if (COM2.RX_Cnt > 0)
            { // 通过UART1 printf     将蓝牙模块发给UART2信息打印出来
                for (index_i = 0; index_i < COM2.RX_Cnt; index_i++)
                {
                    TX1_write2buff(RX2_Buffer[index_i]);
                }

                COM2.RX_Cnt = 0;
                return 1; // 接收成功 返回1
            }
        }
    }
    return 0;
}
