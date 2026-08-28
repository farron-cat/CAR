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

void main(void)
{
    KeyEvent evC;
    KeyEvent evK;

    EAXSFR(); // 使用扩展SFR PWM需要
    LED_C_Init();
    LED_Init();

    Horn_Init();     // 初始化喇叭
    KeyInit();       // 初始化按键 KEY / KEY_C
    Timer0Init1ms(); // 启动Timer0 1ms中断，递增 tickMs（按键扫描依赖）
    UART1Init();     // 初始化串口1（波特率115200，接收使能）

    EA = 1; // 使能全局中断
    while (1)
    {
        UART1RxProcess(); // 串口接收超时计数（判断一帧数据是否接收完成）
        UART1_Command();  // 串口命令处理（1/2/3/4 对应不同动作）

        evC = KeyC_Scan(); // 扫描核心板按键 KEY_C
        evK = Key_Scan();  // 扫描独立按键 KEY

        if (evC == KEY_SHORT_PRESS)
            RunForward5s(); // 短按 KEY_C：向前跑5秒
        else if (evC == KEY_LONG_PRESS)
            BeepOnce(); // 长按 KEY_C：喇叭叫一声

        if (evK == KEY_SHORT_PRESS)
            RunAround(); // 短按 KEY：向左跑
        else if (evK == KEY_LONG_PRESS)
            BeepTwice(); // 长按 KEY：喇叭叫两声

        delay_ms(10); // 周期性调用按键扫描（10ms），保证消抖正确
    }
}
