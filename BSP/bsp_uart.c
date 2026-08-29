#include "STC8G_H_GPIO.h"
#include "STC8G_H_UART.h"
#include "STC8G_H_NVIC.h"
#include "STC8G_H_Switch.h"

#include "bsp_uart.h"

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

/**
 * @brief 串口1发送字符串（以 '\0' 结尾）
 * @param str 待发送的以空字符结尾的字符串
 */
void UART1_SendString(u8 *str)
{
    while (*str)
    {
        TX1_write2buff(*str++); // 逐字节写入发送缓冲
    }
}

/**
 * @brief 在接收帧中匹配是否命中一个字符串命令
 * @param buf 接收缓冲区（帧数据）
 * @param len 接收到的有效字节数
 * @param cmd 待匹配的命令字符串（以 '\0' 结尾）
 * @return 1 表示命中；0 表示未命中
 * @note  逐字符比较命令串，帧内命令之后剩余部分仅允许空白字符
 *        （'\r'/'\n'/空格/'\t'），可兼容串口终端发送时自带的行尾换行。
 */
u8 UART_CmdFrameMatch(const u8 *buf, u8 len, const char *cmd)
{
    u8 i = 0;

    // 空命令串：不匹配
    if (cmd == 0 || cmd[0] == '\0')
        return 0;

    // 逐字符比较命令串与帧开头
    while (cmd[i] != '\0')
    {
        if (i >= len)
            return 0;     // 帧过早结束
        if (buf[i] != cmd[i])
            return 0;     // 出现不一致
        i++;
    }

    // 命令串匹配完毕，帧内剩余部分只能是空白字符（结尾换行/空格）
    for (; i < len; i++)
    {
        if (buf[i] != '\r' && buf[i] != '\n' && buf[i] != ' ' && buf[i] != '\t')
            return 0; // 命令后有非空白内容，如 "LEFTX" 不命中 "LEFT"
    }

    return 1; // 命中
}

/**
 * @brief 串口1命令分发：接收一帧指令并在命令表中匹配执行
 * @param cmdTable 命令结构体数组（const，建议放代码段）
 * @param count    命令表项数
 * @note  需周期性调用（与 UART1RxProcess 配合）。当 UART1_RxFlag 置位表示一帧数据接收完成：
 *        在接收帧中做字符串匹配（cmd/alias），命中则调用 handler 并回传 ack；
 *        未命中回传 "UNKNOWN CMD\r\n"。处理完成后复位 UART1_RxFlag 与 COM1.RX_Cnt。
 */
void UART1_ProcessCommands(const UART1_CmdItem *cmdTable, u8 count)
{
    u8 i;

    if (UART1_RxFlag == 0)
        return; // 未收到完整数据帧

    UART1_RxFlag = 0; // 清除接收完成标志

    if (COM1.RX_Cnt == 0)
        return; // 无有效数据

    // 遍历命令表，对整帧做字符串匹配（cmd 或 alias）
    for (i = 0; i < count; i++)
    {
        if (UART_CmdFrameMatch(RX1_Buffer, COM1.RX_Cnt, cmdTable[i].cmd) ||
            (cmdTable[i].alias != 0 && UART_CmdFrameMatch(RX1_Buffer, COM1.RX_Cnt, cmdTable[i].alias)))
        {
            if (cmdTable[i].handler != 0)
                cmdTable[i].handler(); // 执行动作
            if (cmdTable[i].ack != 0)
                UART1_SendString((u8 *)cmdTable[i].ack); // 回传执行结果
            COM1.RX_Cnt = 0;                             // 复位接收计数
            return;
        }
    }

    // 未匹配到任何命令
    UART1_SendString("UNKNOWN CMD\r\n");
    COM1.RX_Cnt = 0; // 复位接收计数，准备下一次接收
}