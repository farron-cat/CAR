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
 *        对整帧做字符串匹配（cmd/alias），命中则调用 handler 并回传 ack；
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

    // 遍历命令表，对整帧做字符串匹配（cmd 或 alias）
    for (i = 0; i < count; i++)
    {
        if (UART_CmdFrameMatch(RX2_Buffer, COM2.RX_Cnt, cmdTable[i].cmd) ||
            (cmdTable[i].alias != 0 && UART_CmdFrameMatch(RX2_Buffer, COM2.RX_Cnt, cmdTable[i].alias)))
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

/**
 * @brief 将UART1收到的一帧数据原样转发到蓝牙(UART2)
 * @note  需在 UART1_ProcessCommands 之前周期性调用。检测到 UART1_RxFlag 置位时，
 *        将当前帧的 RX1_Buffer 逐字节转发到蓝牙(UART2)，但不修改接收标志/计数
 *        （标志与计数的复位仍由 UART1_ProcessCommands 负责，避免冲突）。
 */
void UART1_BT_Forward(void)
{
    u8 i;

    if (UART1_RxFlag == 0)
        return; // 未收到完整数据帧

    if (COM1.RX_Cnt == 0)
        return; // 无有效数据

    for (i = 0; i < COM1.RX_Cnt; i++)
    {
        TX2_write2buff(RX1_Buffer[i]); // 将UART1收到的原样转发到UART2（蓝牙）
    }
}

/**
 * @brief 将蓝牙(UART2)收到的一帧数据原样转发到UART1
 * @note  需在 BT_ProcessCommands 之前周期性调用。检测到 UART2_RxFlag 置位时，
 *        将当前帧的 RX2_Buffer 逐字节转发到UART1，但不修改接收标志/计数
 *        （标志与计数的复位仍由 BT_ProcessCommands 负责，避免冲突）。
 */
void BT_UART1_Forward(void)
{
    u8 i;

    if (UART2_RxFlag == 0)
        return; // 未收到完整数据帧

    if (COM2.RX_Cnt == 0)
        return; // 无有效数据

    for (i = 0; i < COM2.RX_Cnt; i++)
    {
        TX1_write2buff(RX2_Buffer[i]); // 将蓝牙(UART2)收到的原样转发到UART1
    }
}

/**
 * @brief 通过UART1打印蓝牙状态引脚(BT_STATE)电平，用于诊断模块是否处于可发现/连接状态
 * @note  周期调用即可。定义打印间隔 RPT_PERIOD_MS（如500ms）：
 *        - BT_STATE=1 → 模块可能处于已连接或AT模式，手机通常搜不到
 *        - BT_STATE=0 → 模块处于空闲/可发现状态（具体含义随模组而定）
 */
void BT_StatusReport(void)
{
    static unsigned int report_cnt = 0; // 计数器，配合外部周期调用实现定时打印

    if (++report_cnt >= 50) // 外部每10ms调用一次，则50次=500ms打印一次
    {
        report_cnt = 0;

        if (BT_STATE)
            UART1_SendString("BT_STATE=1\r\n"); // 1: 已连接 / AT模式
        else
            UART1_SendString("BT_STATE=0\r\n"); // 0: 空闲 / 可发现
    }
}
