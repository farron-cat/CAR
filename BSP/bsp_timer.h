/**
 * @file    bsp_timer.h
 * @brief   定时器驱动模块接口
 * @details 提供 STC8H 定时器初始化接口与共享毫秒计数器：
 *          - Timer0Init1ms()：Timer0 1ms 定时中断初始化。
 *          - Timer1Init1ms()：Timer1 1ms 定时中断初始化。
 *          - tickMs：由 Timer0 中断每 1ms 递增的毫秒计数器，供主循环非阻塞时间调度。
 * @note    Timer0 与 Timer1 的中断服务函数定义在 bsp_timer.c 中，本模块不对外暴露。
 */
#ifndef BSP_TIMER_H
#define BSP_TIMER_H

// `dutyUpdateFlag` 在中断中修改，应声明为 `volatile`，防止 Keil C51 优化器缓存该变量导致主循环读取不到最新值。
// extern volatile unsigned char dutyUpdateFlag;
// extern volatile unsigned char tempDetectFlag;
// 毫秒计数器：由Timer0每1ms递增，供主循环各功能做非阻塞时间调度
extern volatile unsigned int tickMs;

/**
 * @brief Timer0初始化（1ms定时中断）
 * @note 配置Timer0为16位自动重载模式，时钟源1T，定时初值对应1ms中断一次。
 * @note 使能Timer0中断，优先级0。调用后需开启全局中断（EA = 1）。
 */
void Timer0Init1ms(void);

/**
 * @brief Timer1初始化（1ms定时中断）
 * @note 配置Timer1为16位自动重载模式，时钟源12T，定时初值对应1ms中断一次。
 * @note 使能Timer1中断，优先级0。调用后需开启全局中断（EA = 1）。
 */
void Timer1Init1ms(void);

#endif // BSP_TIMER_H