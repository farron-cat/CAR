/**
 * @file    bsp_control.c
 * @brief   小车指令控制模块
 * @details 集中管理小车"动作指令"：命令表 g_uartCmds[]（字符串命令）
 *          以及各指令对应的电机动作回调（前进/后退/平移/原地转向/停止）。
 * @note    依赖 bsp_motor_dirver.h 提供的 Motors_* 电机驱动接口。
 */

#include "bsp_control.h"
#include "bsp_motor_dirver.h" // Motors_Forward / Motors_Backward / ... / Motors_Stop

// 小车移动速度（0~100，值越大越快），供各指令回调使用
#define CAR_SPEED 70

// ---------- 指令回调（动作） ----------

// 前进（保持当前速度运行，直到收到新的移动/停止指令）
void Cmd_Forward(void)
{
    Motors_Forward(CAR_SPEED);
}

// 后退
void Cmd_Backward(void)
{
    Motors_Backward(CAR_SPEED);
}

// 左平移
void Cmd_Left(void)
{
    Motors_Left(CAR_SPEED, 0);
}

// 右平移
void Cmd_Right(void)
{
    Motors_Right(CAR_SPEED, 0);
}

// 原地左转（逆时针旋转）
void Cmd_TurnLeft(void)
{
    Motors_Around(CAR_SPEED, 0);
}

// 原地右转（顺时针旋转）
void Cmd_TurnRight(void)
{
    Motors_Around(CAR_SPEED, 1);
}

// 停止所有电机
void Cmd_Stop(void)
{
    Motors_Stop();
}

/**
 * @brief 小车指令命令表（字符串命令）
 * @note  结构：{ 命令全称 cmd, 别名 alias, 回调 handler, 回传 ack }
 *        串口驱动对整帧做字符串匹配（含 alias），命中后回调执行并回传 ack。
 */
const UART1_CmdItem g_uartCmds[] = {
    {"FORWARD",  "FW",  Cmd_Forward,   "FORWARD OK\r\n"},
    {"BACKWARD", "BW",  Cmd_Backward,  "BACKWARD OK\r\n"},
    {"LEFT",     "L",   Cmd_Left,      "LEFT OK\r\n"},
    {"RIGHT",    "R",   Cmd_Right,     "RIGHT OK\r\n"},
    {"TURN-LEFT",  "TL", Cmd_TurnLeft,  "TURN-LEFT OK\r\n"},
    {"TURN-RIGHT", "TR", Cmd_TurnRight, "TURN-RIGHT OK\r\n"},
    {"STOP",     "S",   Cmd_Stop,      "STOP OK\r\n"},
};

// 命令表条目数
const u8 g_uartCmdCount = sizeof(g_uartCmds) / sizeof(g_uartCmds[0]);
