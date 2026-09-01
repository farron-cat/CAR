/**
 * @file    bsp_timer.c
 * @brief   定时器驱动模块
 * @details 本模块负责 STC8H 定时器的初始化与中断服务处理：
 *          - Timer0：1ms 定时中断（1T 时钟源，16 位自动重载），驱动毫秒计数器 tickMs 递增，
 *            供主循环各功能做非阻塞时间调度；其 ISR 中保留了数码管与温度检测的定时扩展接口。
 *          - Timer1：1ms 定时中断（12T 时钟源），其 ISR 执行 8 位数码管动态扫描刷新。
 * @note    Timer3 已改由超声波模块（bsp_ultrasonic）用作回波计时（12T 自由计数），
 *          不在本模块中初始化。
 * @note    依赖 STC8G_H_Timer.h（定时器初始化）、STC8G_H_NVIC.h（中断配置）。
 * @note    Timer 中断服务函数使用 C51 关键字 interrupt 声明（TMR0_VECTOR / TMR1_VECTOR）。
 * @note    Timer1 的 ISR 引用了 bsp_digital_led 的显示缓冲与段码表，需先初始化数码管。
 */

#include "STC8G_H_Timer.h"
#include "STC8G_H_NVIC.h"
#include "bsp_digital_led.h"

volatile unsigned char dutyUpdateFlag = 0; // 占空比更新标志
volatile unsigned char tempDetectFlag = 0; // 温度检测标志
volatile unsigned int tickMs = 0;          // 毫秒计数器

/**
 * @brief Timer0初始化（1ms定时中断）
 * @note 配置Timer0为16位自动重载模式，时钟源1T
 * @note 定时初值 = 65536 - (MAIN_Fosc / 1000)，即1ms中断一次
 * @note 使能Timer0中断，优先级0
 */
void Timer0Init1ms(void)
{
    TIM_InitTypeDef TIM_INIT_STRUCT;

    TIM_INIT_STRUCT.TIM_Mode = TIM_16BitAutoReload;             // 指定工作模式 16位自动重载
    TIM_INIT_STRUCT.TIM_ClkSource = TIM_CLOCK_1T;               // 指定时钟源 1T
    TIM_INIT_STRUCT.TIM_ClkOut = DISABLE;                       // 指定是否输出时钟 否
    TIM_INIT_STRUCT.TIM_Value = 65536UL - (MAIN_Fosc / 1000UL); // 指定初值 1000Hz 1ms  65536UL - (MAIN_Fosc / 中断频率)
    TIM_INIT_STRUCT.TIM_Run = ENABLE;                           // 指定是否启动定时器 是

    Timer_Inilize(Timer0, &TIM_INIT_STRUCT); // 初始化Timer0

    NVIC_Timer0_Init(ENABLE, Priority_0); // 使能Timer0中断，优先级0
}

/**
 * @brief Timer1初始化（1ms定时中断）
 * @note 配置Timer1为16位自动重载模式，时钟源12T
 * @note 定时初值 = 65536 - (MAIN_Fosc / 12 / 1000)，即1ms中断一次
 * @note 使能Timer1中断，优先级0
 */
void Timer1Init1ms(void)
{
    TIM_InitTypeDef TIM_INIT_STRUCT;

    TIM_INIT_STRUCT.TIM_Mode = TIM_16BitAutoReload;                    // 指定工作模式 16位自动重载
    TIM_INIT_STRUCT.TIM_ClkSource = TIM_CLOCK_12T;                     // 指定时钟源 12T
    TIM_INIT_STRUCT.TIM_ClkOut = DISABLE;                              // 指定是否输出时钟 否
    TIM_INIT_STRUCT.TIM_Value = 65536UL - (MAIN_Fosc / 12UL / 1000UL); // 指定初值 1000Hz 1ms  65536UL - (MAIN_Fosc / 12 / 中断频率)
    TIM_INIT_STRUCT.TIM_Run = ENABLE;                                  // 指定是否启动定时器 是

    Timer_Inilize(Timer1, &TIM_INIT_STRUCT); // 初始化Timer1

    NVIC_Timer1_Init(ENABLE, Priority_0); // 使能Timer1中断，优先级0
}

// TODO: 临时放这里的函数
/**
 * @brief Timer0中断服务函数（1ms定时中断处理）
 * @note 进中断时硬件已自动清除标志位
 * @note 每1ms触发一次，递增毫秒计数器 tickMs
 */

void Timer0_ISR_Handler(void) interrupt TMR0_VECTOR // 进中断时已经清除标志
{
    // static unsigned char cnt5ms = 0;
    // static unsigned int cnt1000ms = 0;

    tickMs++; // 毫秒计数器递增

    // cnt5ms++;
    // if (cnt5ms == 5) // 5ms
    // {
    //     cnt5ms = 0;
    //     dutyUpdateFlag = 1;
    // }

    // cnt1000ms++;
    // if (cnt1000ms == 200) // 1000ms
    // {
    //     cnt1000ms = 0;
    //     tempDetectFlag = 1;
    // }
}

/**
 * @brief Timer1中断服务函数（1ms定时中断处理）
 * @note 进中断时硬件已自动清除标志位
 * @note 每1ms触发一次，执行数码管动态扫描：按 position 依次发送
 *       显示缓冲与段码，并在发送移入数据后切换下一位扫描位置。
 */
void Timer1_ISR_Handler(void) interrupt TMR1_VECTOR // 进中断时已经清除标志
{
    // 数码管扫描
    static unsigned char position = 0;

    SendByte(displayBuffer[position]);
    SendByte(digCodeTable[position]);

    // 移入数据
    RCLK = 1;
    RCLK = 0;

    position = (position + 1) % 8;
}
