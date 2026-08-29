/**
 * @file    bsp_control.c
 * @brief   小车指令控制模块
 * @details 集中管理小车"动作指令"：命令表 g_uartCmds[]（字符串命令）
 *          以及各指令对应的电机动作回调（前进/后退/平移/原地转向/停止）。
 * @note    依赖 bsp_motor_dirver.h 提供的 Motors_* 电机驱动接口。
 */

#include "bsp_control.h"
#include "STC8G_H_UART.h" // RX2_Buffer / COM2（UART2 接收缓冲区与计数）

#include "bsp_motor_dirver.h" // Motors_Forward / Motors_Around / Motors_Stop / Motors_move
#include "bsp_bluetooth.h"    // UART2_RxFlag（蓝牙接收完成标志）
#include "bsp_light.h"        // Light_SetState / LIGHT_RUN / LIGHT_LEFT / LIGHT_RIGHT
#include "bsp_horn.h"         // Horn_Beep

// 小车移动速度（0~100，值越大越快），供各指令回调使用
#define CAR_SPEED 70

// 手机遥控：原地旋转速度（低速，便于控制）
#define REMOTE_TURN_SPEED 30

// 遥控空闲超时计数（主循环约10ms调用一次，30次≈300ms）：
// 超过该时间未收到新控制帧则自动停止电机，防止松开按键/断连后一直运动
#define REMOTE_IDLE_TIMEOUT 30

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
    {"FORWARD", "FW", Cmd_Forward, "FORWARD OK\r\n"},
    {"BACKWARD", "BW", Cmd_Backward, "BACKWARD OK\r\n"},
    {"LEFT", "L", Cmd_Left, "LEFT OK\r\n"},
    {"RIGHT", "R", Cmd_Right, "RIGHT OK\r\n"},
    {"TURN-LEFT", "TL", Cmd_TurnLeft, "TURN-LEFT OK\r\n"},
    {"TURN-RIGHT", "TR", Cmd_TurnRight, "TURN-RIGHT OK\r\n"},
    {"STOP", "S", Cmd_Stop, "STOP OK\r\n"},
};

// 命令表条目数
const u8 g_uartCmdCount = sizeof(g_uartCmds) / sizeof(g_uartCmds[0]);

/**
 * @brief 手机小程序（蓝牙）遥控处理函数
 * @note  需周期调用（约10ms，配合主循环）。从 UART2(RX2_Buffer) 固定协议帧解析：
 *        - buf[0..1] 帧头，buf[2] = 摇杆 x，buf[3] = 摇杆 y
 *        - buf[4] = A 按键，buf[5] = B 按键，buf[6] = C 按键，buf[7] = D 按键
 *        - A 键（边缘触发）：鸣笛 + 车灯全亮/全灭切换
 *        - B 键（巡线）：暂留空占位
 *        - C/D 键（电平触发）：原地左/右旋转，松开停止
 *        - 摇杆：未旋转时通过 Motors_move() 全向移动
 *        采用轮询方式读取 RX2_Buffer 最新一帧（尾部8字节）；若超过约300ms未收到新帧
 *        （REMOTE_IDLE_TIMEOUT），自动停止电机并复位边缘触发状态。
 */
void BT_Remote_Control(void)
{
    // static变量，函数调用完毕不释放
    static u8 led_flag = 0;    // 1:灯亮, 0:灯灭
    static u8 is_turning = 0;  // 1:正在原地旋转, 0:未旋转
    static u8 s_lastA = 0;     // 1:A键上一次状态（边缘触发用）
    static u8 s_lastB = 0;     // 1:B键上一次状态（边缘触发用）
    static u8 s_lastRxCnt = 0; // 上一次调用时的接收计数（用于判断是否有新帧）
    static u8 s_idleCnt = 0;   // 空闲计数：连续未收到新帧的次数

    char x; // 摇杆横向分量（有符号，负=左，正=右）
    char y; // 摇杆纵向分量（有符号，负=后，正=前）
    u8 cur_A, cur_B, cur_C, cur_D;

    // 判断是否有新数据到达（RX_Cnt 发生变化即有新帧）
    if (COM2.RX_Cnt != s_lastRxCnt)
    {
        s_lastRxCnt = COM2.RX_Cnt;
        s_idleCnt = 0; // 有新数据，刷新空闲计数
    }
    else if (s_idleCnt < REMOTE_IDLE_TIMEOUT)
    {
        s_idleCnt++; // 无新数据，空闲计数累加
    }

    // 遥控空闲超时：手机停止发送（松开按键未发帧 / 蓝牙断开），安全停止电机
    if (s_idleCnt >= REMOTE_IDLE_TIMEOUT)
    {
        if (is_turning == 1)
        {
            Motors_Stop(); // 电机停止
            is_turning = 0;
        }
        s_lastA = 0; // 复位边缘触发状态，再次按下可重新触发
        s_lastB = 0;
        return; // 无新数据且已超时，不再基于旧状态执行
    }

    // 帧长度不足 8 字节，等待更多数据
    if (COM2.RX_Cnt < 8)
        return;

    // 提取最新一帧状态（RX2_Buffer 尾部 8 字节，对应协议 buf[0..7]）
    x = (char)RX2_Buffer[COM2.RX_Cnt - 6]; // buf[2] 摇杆x
    y = (char)RX2_Buffer[COM2.RX_Cnt - 5]; // buf[3] 摇杆y
    cur_A = RX2_Buffer[COM2.RX_Cnt - 4];   // buf[4] A键
    cur_B = RX2_Buffer[COM2.RX_Cnt - 3];   // buf[5] B键
    cur_C = RX2_Buffer[COM2.RX_Cnt - 2];   // buf[6] C键
    cur_D = RX2_Buffer[COM2.RX_Cnt - 1];   // buf[7] D键

    // 1. A键: 蜂鸣器/车灯（边缘触发，只在按下瞬间执行一次）
    if (cur_A == 1 && s_lastA == 0)
    {
        Horn_Beep(7, 100);
        Horn_Beep(8, 100);
        Horn_Beep(9, 100);
        if (led_flag == 0)
        { // 原来是灭，需要开灯
            Light_SetState(LIGHT_RUN, LIGHT_ON);
            Light_SetState(LIGHT_LEFT, LIGHT_ON);
            Light_SetState(LIGHT_RIGHT, LIGHT_ON);
        }
        else
        { // 原来是亮的，需关灯
            Light_SetState(LIGHT_RUN, LIGHT_OFF);
            Light_SetState(LIGHT_LEFT, LIGHT_OFF);
            Light_SetState(LIGHT_RIGHT, LIGHT_OFF);
        }
        led_flag = !led_flag; // 标志位反转
    }
    s_lastA = cur_A; // 记录A键本次状态

    // 2. B键: 巡线开关（边缘触发，先留空占位，暂不实现）
    if (cur_B == 1 && s_lastB == 0)
    {
        // TODO: 开启/关闭巡线功能（巡线任务后续实现）
    }
    s_lastB = cur_B; // 记录B键本次状态

    // 3. 运动控制
    //    C/D键: 原地旋转（电平触发：按下持续转，松开停止）
    if (cur_C == 1)
    { // 按下C: 左旋转
        if (is_turning == 0)
        {
            Motors_Around(REMOTE_TURN_SPEED, 1);
            is_turning = 1;
        }
    }
    else if (cur_D == 1)
    { // 按下D: 右旋转
        if (is_turning == 0)
        {
            Motors_Around(REMOTE_TURN_SPEED, 0);
            is_turning = 1;
        }
    }
    else
    { // C和D抬起
        if (is_turning == 1)
        {
            Motors_Stop(); // 电机停止
            is_turning = 0;
        }
    }

    // 4. 摇杆控制: 只有在没有按下旋转按键时，摇杆才生效
    if (is_turning == 0)
    {
        Motors_move(x, y); // 麦克纳姆全向移动
    }
}