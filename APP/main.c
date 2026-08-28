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

/**
 * @brief 串口1命令处理：解析接收到的单字符命令并执行对应动作，处理后回传执行结果
 * @note 需周期性调用（与 UART1RxProcess 配合），当 UART1_RxFlag 置位表示一帧数据接收完成：
 *         - 发送 '1' → BeepOnce    (喇叭叫一声)
 *         - 发送 '2' → BeepTwice   (喇叭叫两声)
 *         - 发送 '3' → RunForward5s(向前跑5秒)
 *         - 发送 '4' → RunLeft     (向左平移跑)
 * @note 取首个有效字节作为命令，执行命令后通过 UART1_SendString 回传状态（如 "BEEP1 OK\r\n"），
 *       处理完成后复位 UART1_RxFlag 与 COM1.RX_Cnt。
 */
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
void UART1_Command(void)
{
    if (UART1_RxFlag == 0)
        return; // 未收到完整数据帧

    UART1_RxFlag = 0; // 清除接收完成标志

    if (COM1.RX_Cnt == 0)
        return; // 无有效数据

    // 取第一个字节作为命令
    switch (RX1_Buffer[0])
    {
    case '1': // 喇叭叫一声
        BeepOnce();
        UART1_SendString("BEEP1 OK\r\n");
        break;

    case '2': // 喇叭叫两声
        BeepTwice();
        UART1_SendString("BEEP2 OK\r\n");
        break;

    case '3': // 向前跑5秒
        RunForward5s();
        UART1_SendString("FORWARD5S OK\r\n");
        break;

    case '4': // 向左平移跑
        RunAround();
        UART1_SendString("Around OK\r\n");
        break;

    default:
        UART1_SendString("UNKNOWN CMD\r\n");
        break; // 忽略未知命令
    }

    COM1.RX_Cnt = 0; // 复位接收计数，准备下一次接收
}

void main(void)
{
    KeyEvent evC;
    KeyEvent evK;

    EAXSFR(); // 使用扩展SFR PWM需要
    LED_C_Init();
    LED_Init();

    Horn_Init();     // 初始化喇叭
    // Motors_Init();   // 初始化电机PWM硬件（bsp_motor_driver，必须调用否则电机无输出）
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
