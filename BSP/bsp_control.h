/**
 * @file    bsp_control.h
 * @brief   小车指令控制模块接口
 * @details 定义小车"动作指令"命令表及其回调函数：
 *          - 命令表 g_uartCmds[]：每条指令为字符串（cmd/alias），
 *            由串口驱动（UART1_ProcessCommands / BT_ProcessCommands）匹配并回调。
 *          - 动作函数：前进、后退、左移、右移、原地左转/右转、停止。
 * @note    命令表与动作函数从应用层(APP/main.c)抽出，集中在本模块管理，便于扩展。
 */
#ifndef BSP_CONTROL_H
#define BSP_CONTROL_H

#include "bsp_uart.h" // 复用 UART1_CmdItem 命令表结构体（提供字符串匹配框架）

/**
 * @brief 小车指令命令表（字符串命令）
 * @note  每条命令由 (cmd全称, alias别名, handler回调, ack回传) 组成，
 *        串口驱动通过 UART_CmdFrameMatch() 对整帧做字符串匹配后回调执行。
 */
extern const UART1_CmdItem g_uartCmds[];

/**
 * @brief 命令表条目数
 * @note  extern 数组无法对引用侧取 sizeof，故单独暴露条目数，供分发函数使用。
 */
extern const u8 g_uartCmdCount;

// ---------- 指令回调（动作） ----------

void Cmd_Forward(void);   // 前进
void Cmd_Backward(void);  // 后退
void Cmd_Left(void);      // 左移
void Cmd_Right(void);     // 右移
void Cmd_TurnLeft(void);  // 原地左转（逆时针）
void Cmd_TurnRight(void); // 原地右转（顺时针）
void Cmd_Stop(void);      // 停止

/**
 * @brief 手机小程序（蓝牙）遥控处理函数
 * @note  需周期调用。从 UART2(RX2_Buffer) 固定协议帧中解析摇杆与按键：
 *        - buf[0..1] 帧头，buf[2] = 摇杆 x，buf[3] = 摇杆 y
 *        - buf[4] = A 按键，buf[5] = B 按键，buf[6] = C 按键，buf[7] = D 按键
 *        实现：A 键鸣笛+车灯切换、B/C 键原地左右转、摇杆全向移动；
 *        D 键（巡线）暂留空占位。处理完成后复位 UART2_RxFlag 与 COM2.RX_Cnt。
 */
void BT_Remote_Control(void);

#endif // BSP_CONTROL_H
