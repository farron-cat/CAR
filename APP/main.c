#include "STC8H.H"
#include "bsp_led.h"
#include "STC8G_H_Delay.h"
#include "bsp_motor_dirver.h"
#include "STC8G_H_UART.h"

#include "bsp_horn.h"
#include "bsp_motor_io.h"
#include "bsp_key.h"
#include "bsp_timer.h"
#include "bsp_uart.h"
#include "bsp_bluetooth.h"

// 小车移动速度（0~100，越大越快），供串口单字母指令使用
#define CAR_SPEED 70

// 喇叭鸣叫一声
void BeepOnce(void)
{
    Horn_PlayTone(5); // 播放指定音调
    delay_ms(200);    // 持续200ms
    Horn_stop();      // 停止
}

// 喇叭鸣叫两声（间隔300ms）
void BeepTwice(void)
{
    Horn_PlayTone(5);
    delay_ms(200);
    Horn_stop();
    delay_ms(300);
    Horn_PlayTone(5);
    delay_ms(200);
    Horn_stop();
}

// 向前跑5秒
void RunForward5s(void)
{
    Motors_Forward(70); // 全轮正转（速度70）
    delay_ms(5000);     // 持续5秒
    Motors_Stop();      // 停止
}

// 向左平移跑
void RunAround(void)
{
    Motors_Around(70, 0); // 左平移（速度70）
    delay_ms(3000);       // 持续3秒
    Motors_Stop();        // 停止
}

// ---------- 串口单字母指令的落地动作（无参包装，经命令表回调调用） ----------

// 前进（持续，直到收到新的移动/停止指令，下同）
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

// 左转（原地逆时针旋转）
void Cmd_TurnLeft(void)
{
    Motors_Around(CAR_SPEED, 0);
}

// 右转（原地顺时针旋转）
void Cmd_TurnRight(void)
{
    Motors_Around(CAR_SPEED, 1);
}

// 停止所有电机
void Cmd_Stop(void)
{
    Motors_Stop();
}

// 串口1单字符命令表：{ 触发字符, 别名, 处理函数, 回传内容 }
// 命令由应用层定义并以参数传给 UART1_ProcessCommands()，串口驱动不关心具体业务。
const UART1_CmdItem g_uartCmds[] = {
    {'W', 'w', Cmd_Forward,   "FORWARD OK\r\n"},
    {'S', 's', Cmd_Backward,  "BACKWARD OK\r\n"},
    {'A', 'a', Cmd_Left,      "LEFT OK\r\n"},
    {'D', 'd', Cmd_Right,     "RIGHT OK\r\n"},
    {'Q', 'q', Cmd_TurnLeft,  "TURN-LEFT OK\r\n"},
    {'E', 'e', Cmd_TurnRight, "TURN-RIGHT OK\r\n"},
    {'X', 'x', Cmd_Stop,      "STOP OK\r\n"},
};

/**
 * @brief 按键任务：扫描 KEY 与 KEY_C，并响应各自动作
 * @note 需周期性调用（与主循环周期一致，约10ms），保证消抖状态机正确
 */
void Key_Task(void)
{
    KeyEvent evC = KeyC_Scan(); // 扫描核心板按键 KEY_C
    KeyEvent evK = Key_Scan();  // 扫描独立按键 KEY

    if (evC == KEY_SHORT_PRESS)
        RunForward5s(); // 短按 KEY_C：向前跑5秒
    else if (evC == KEY_LONG_PRESS)
        BeepOnce(); // 长按 KEY_C：喇叭叫一声

    if (evK == KEY_SHORT_PRESS)
        RunAround(); // 短按 KEY：向左跑
    else if (evK == KEY_LONG_PRESS)
        BeepTwice(); // 长按 KEY：喇叭叫两声
}

void main(void)
{
    EAXSFR(); // 使用扩展SFR PWM需要
    LED_C_Init();
    LED_Init();

    Horn_Init();     // 初始化喇叭
    KeyInit();       // 初始化按键 KEY / KEY_C
    Timer0Init1ms(); // 启动Timer0 1ms中断，递增 tickMs（按键扫描依赖）
    UART1Init();     // 初始化串口1（波特率115200，接收使能）
    BT_Init();       // 初始化蓝牙模块（UART2，波特率115200，接收使能）

    EA = 1; // 使能全局中断
    while (1)
    {
        UART1RxProcess(); // 串口接收超时计数（判断一帧数据是否接收完成）
        // 串口命令分发：根据命令表执行移动等动作
        UART1_ProcessCommands(g_uartCmds, sizeof(g_uartCmds) / sizeof(g_uartCmds[0]));

        BT_RxProcess(); // 蓝牙(UART2)接收超时计数（判断一帧数据是否接收完成）
        // 蓝牙命令分发：根据命令表执行移动等动作
        BT_ProcessCommands(g_uartCmds, sizeof(g_uartCmds) / sizeof(g_uartCmds[0]));

        Key_Task(); // 按键任务（扫描并响应 KEY / KEY_C）

        delay_ms(10); // 周期性调用（10ms），保证消抖正确
    }
}
