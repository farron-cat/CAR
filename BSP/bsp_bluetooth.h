#ifndef __BSP_BLUETOOTH_H__
#define __BSP_BLUETOOTH_H__

#include "bsp_uart.h" // 复用 UART1_CmdItem 命令表结构体

#define BT_EN P46    // 蓝牙使能引脚（P4.6），输出
#define BT_STATE P06 // 蓝牙状态引脚（P0.6），输入
#define BT_RXD P10   // UART2 RxD（P1.0）
#define BT_TXD P11   // UART2 TxD（P1.1）

extern unsigned char UART2_RxFlag; // 蓝牙(UART2)接收完成标志位

void BT_Init(void);                // 初始化蓝牙模块（GPIO + UART2）
void BT_UART_SENsD2BT(void);       // 发送数据到蓝牙模块
u8 BT_UART_RESFBT(void);           // 接收数据从蓝牙模块

void BT_RxProcess(void);           // 判断UART2一帧数据是否接收完成（置位 UART2_RxFlag）
void BT_ProcessCommands(const UART1_CmdItem *cmdTable, u8 count); // 蓝牙(UART2)命令分发

void UART1_BT_Forward(void);       // 将UART1收到的一帧数据原样转发到蓝牙(UART2)
void BT_UART1_Forward(void);       // 将蓝牙(UART2)收到的一帧数据原样转发到UART1

void BT_StatusReport(void);        // 通过UART1打印蓝牙状态引脚(BT_STATE)电平，用于诊断

#endif // __BSP_BLUETOOTH_H__