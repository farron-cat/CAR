#include "STC8G_H_GPIO.h"
#include "STC8G_H_UART.h"
#include "STC8G_H_NVIC.h"
#include "STC8G_H_Switch.h"

#include "bsp_bluetooth.h"
#include "bsp_uart.h" // 复用 UART1_CmdItem 命令表结构体

// 蓝牙(UART2)接收完成标志位
unsigned char UART2_RxFlag = 0;

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
            { // 通过UART1 printf 将蓝牙模块发给UART2信息打印出来
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

// 通过UART2（蓝牙）发送字符串（以 '\0' 结尾）
void BT_SendString(u8 *str)
{
    while (*str)
    {
        TX2_write2buff(*str++); // 逐字节写入UART2发送缓冲
    }
}

// 判断UART2（蓝牙）一组数据是否接收完成
void BT_RxProcess(void)
{
    if (COM2.RX_TimeOut > 0)
    {
        if (--COM2.RX_TimeOut == 0)
        {
            UART2_RxFlag = 1;
        }
    }
}

/**
 * @brief 蓝牙(UART2)命令分发：接收一帧指令并在命令表中匹配执行
 * @param cmdTable 命令结构体数组（与 UART1_ProcessCommands 共用同一命令表）
 * @param count    命令表项数
 * @note  需周期性调用（与 BT_RxProcess 配合），当 UART2_RxFlag 置位表示一帧数据接收完成：
 *        取首个有效字节作为命令，遍历命令表匹配 cmd/alias，命中则调用 handler 并回传 ack；
 *        未命中回传 "UNKNOWN BT CMD\r\n"。处理完成后复位 UART2_RxFlag 与 COM2.RX_Cnt。
 */
void BT_ProcessCommands(const UART1_CmdItem *cmdTable, u8 count)
{
    u8 i;

    if (UART2_RxFlag == 0)
        return; // 未收到完整数据帧

    UART2_RxFlag = 0; // 清除接收完成标志

    if (COM2.RX_Cnt == 0)
        return; // 无有效数据

    // 取第一个字节作为命令，在命令表中匹配执行
    for (i = 0; i < count; i++)
    {
        if (RX2_Buffer[0] == cmdTable[i].cmd || RX2_Buffer[0] == cmdTable[i].alias)
        {
            if (cmdTable[i].handler != 0)
                cmdTable[i].handler(); // 执行动作
            if (cmdTable[i].ack != 0)
                BT_SendString((u8 *)cmdTable[i].ack); // 回传执行结果
            COM2.RX_Cnt = 0;                          // 复位接收计数
            return;
        }
    }

    // 未匹配到任何命令
    BT_SendString("UNKNOWN BT CMD\r\n");
    COM2.RX_Cnt = 0; // 复位接收计数，准备下一次接收
}
