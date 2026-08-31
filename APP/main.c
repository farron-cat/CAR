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
#include "bsp_control.h"
#include "bsp_ultrasonic.h"
#include "bsp_adc.h"

#include <stdio.h>

void main(void)
{

    EAXSFR(); // 使能扩展SFR（PWM需要）

    Light_Init(); // 初始化小车灯光
    Light_SetState(LIGHT_RUN, LIGHT_OFF);
    Light_SetState(LIGHT_LEFT, LIGHT_OFF);
    Light_SetState(LIGHT_RIGHT, LIGHT_OFF);
    Light_SetState(LIGHT_TRACK, LIGHT_OFF);
    Light_SetState(LIGHT_RANGE, LIGHT_OFF);

    Motor_Init();      // 初始化电机
    Horn_Init();       // 初始化喇叭
    KeyInit();         // 初始化按键 KEY / KEY_C
    UART1Init();       // 初始化串口1（波特率115200，中断使能）
    BT_Init();         // 初始化蓝牙模块（UART2，波特率115200，中断使能）
    Ultrasonic_Init(); // 初始化超声波传感器
    ADC_Init();        // 初始化ADC（用于测量电池电压）

    Timer0Init1ms();  // 配置Timer0 1ms中断，用于 tickMs 计时等
    Timer3Init10us(); // 配置Timer3 10us中断，驱动非阻塞超声波测距

    EA = 1; // 使能全局中断

    printf("UART1 OK\r\n");

    while (1)
    {
        v
            printf("battary: %.2fv\r\n", ADC_Battary_Voltage());

        // UART1 串口调试（字符串命令，如 FORWARD/FW/STOP 等）：
        // UART1RxProcess();   // 串口接收超时判断：检测到一帧数据接收是否完成
        // UART1_ProcessCommands(g_uartCmds, g_uartCmdCount);

        BT_RxProcess(); // 蓝牙(UART2)接收超时判断：检测到一帧数据接收是否完成
        // BT_UART1_Forward(); // 将蓝牙(UART2)收到的数据原样转发至UART1
        // 手机小程序（蓝牙）遥控：解析协议帧（摇杆 + A/B/C/D 按键）并驱动小车
        BT_Remote_Control();
        Ultrasonic_Radar_Task(); // 雷达鸣叫（非阻塞，2cm→0.5s、20cm→3s）

        // Key_Task(); // 周期扫描并响应 KEY / KEY_C

        // BT_StatusReport(); // 周期打印蓝牙状态，每10ms调用一次，约500ms打印一次（可注释）

        delay_ms(10); // 简单延时节拍，10ms左右，保证时序正确
    }
}
