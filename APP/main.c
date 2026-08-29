#include "STC8H.H"
#include "STC8G_H_Delay.h"
#include "STC8G_H_UART.h"

#include "bsp_light.h"
#include "bsp_horn.h"
#include "bsp_motor_io.h"
#include "bsp_key.h"
#include "bsp_timer.h"
#include "bsp_motor_dirver.h"
#include "bsp_uart.h"
#include "bsp_bluetooth.h"
#include "bsp_control.h" // 字符串指令命令表与动作回调

// 鸣笛一次
void BeepOnce(void)
{
    Horn_PlayTone(5); // 播放指定音调
    delay_ms(200);    // 持续200ms
    Horn_stop();      // 停止
}

// 鸣笛两次（两段之间间隔300ms）
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
    Motors_Forward(70); // 全速前进，速度70）
    delay_ms(5000);     // 持续5秒
    Motors_Stop();      // 停止
}

// 原地打转
void RunAround(void)
{
    Motors_Around(70, 0); // 原地打转（逆时针），速度70）
    delay_ms(3000);       // 持续3秒
    Motors_Stop();        // 停止
}

/**
 * @brief 按键扫描 KEY 与 KEY_C 并响应对应动作
 * @note 建议周期调用（与主循环一致，约10ms），保证按键状态判断正确
 */
void Key_Task(void)
{
    KeyEvent evC = KeyC_Scan(); // 扫描核心板按键 KEY_C
    KeyEvent evK = Key_Scan();  // 扫描扩展板按键 KEY

    if (evC == KEY_SHORT_PRESS)
        RunForward5s(); // 短按 KEY_C，向前跑5秒
    else if (evC == KEY_LONG_PRESS)
        BeepOnce(); // 长按 KEY_C，鸣笛一声

    if (evK == KEY_SHORT_PRESS)
        RunAround(); // 短按 KEY，原地打转
    else if (evK == KEY_LONG_PRESS)
        BeepTwice(); // 长按 KEY，鸣笛两声
}

void main(void)
{
    EAXSFR(); // 使能扩展SFR（PWM需要）

    Light_Init(); // 初始化小车灯光
    Light_SetState(LIGHT_RUN, LIGHT_OFF);
    Light_SetState(LIGHT_LEFT, LIGHT_OFF);
    Light_SetState(LIGHT_RIGHT, LIGHT_OFF);
    Light_SetState(LIGHT_TRACK, LIGHT_OFF);
    Light_SetState(LIGHT_RANGE, LIGHT_OFF);

    Motor_Init();    // 初始化电机
    Horn_Init();     // 初始化喇叭
    KeyInit();       // 初始化按键 KEY / KEY_C
    Timer0Init1ms(); // 配置Timer0 1ms中断，用于 tickMs 计时等
    UART1Init();     // 初始化串口1（波特率115200，中断使能）
    BT_Init();       // 初始化蓝牙模块（UART2，波特率115200，中断使能）

    EA = 1; // 使能全局中断
    while (1)
    {
        // UART1 串口调试（字符串命令，如 FORWARD/FW/STOP 等）：
        // UART1RxProcess();   // 串口接收超时判断：检测到一帧数据接收是否完成
        // UART1_ProcessCommands(g_uartCmds, g_uartCmdCount);

        BT_RxProcess();     // 蓝牙(UART2)接收超时判断：检测到一帧数据接收是否完成
        BT_UART1_Forward(); // 将蓝牙(UART2)收到的数据原样转发至UART1
        // 手机小程序（蓝牙）遥控：解析协议帧（摇杆 + A/B/C/D 按键）并驱动小车
        BT_Remote_Control();

        // Key_Task(); // 周期扫描并响应 KEY / KEY_C

        // BT_StatusReport(); // 周期打印蓝牙状态，每10ms调用一次，约500ms打印一次（可注释）

        delay_ms(10); // 简单延时节拍，10ms左右，保证时序正确
    }
}
