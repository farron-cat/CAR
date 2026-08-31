/**
 * @file    bsp_ultrasonic.h
 * @brief   超声波测距模块（HC-SR04）接口定义
 * @details 本模块提供阻塞式和非阻塞式两种测距 API：
 *          - 阻塞式：Ultrasonic_GetDistance()，通过延时等待回波。
 *          - 非阻塞式：Ultrasonic_GetDistance_NB()，配合定时器中断状态机。
 *          引脚连接：
 *          - TRIG（P4.7）：推挽输出，产生触发信号（≥10us 高电平）。
 *          - ECHO（P3.3）：高阻输入（或准双向），接收回波信号，高电平宽度对应距离。
 * @note    系统时钟 24MHz，非阻塞模式需 Timer3 产生 10us 中断，
 *          并将 Ultrasonic_NB_Isr() 放在该中断服务函数中调用。
 */

#ifndef BSP_ULTRASONIC_H
#define BSP_ULTRASONIC_H

#include "STC8H.H" // 包含 sbit 引脚定义（P33 / P47）

// 引脚定义
#define TRIG P47 // 触发引脚（P4.7）
#define ECHO P33 // 回波引脚（P3.3）

//  返回值状态码
#define ULTRASONIC_OK 0           // 测距成功
#define ULTRASONIC_BUSY 1         // 非阻塞测距进行中
#define ULTRASONIC_ERR_NO_ECHO -1 // 未收到回波（超时 5ms）
#define ULTRASONIC_ERR_TOO_FAR -2 // 回波宽度超时（>30ms，距离过远）
#define ULTRASONIC_ERR_RANGE -3   // 结果超出有效范围（2~400cm）

// 函数声明

/**
 * @brief 初始化超声波模块引脚
 * @note  配置 TRIG（P4.7）为推挽输出，ECHO（P3.3）为高阻输入。
 *        定时器3的 10us 中断需外部开启，本函数不负责定时器初始化。
 */
void Ultrasonic_Init(void);

/**
 * @brief 阻塞式单次测距
 * @param distance 输出参数，存放测量得到的距离值（单位 cm）
 * @return char 执行状态码：
 *         - ULTRASONIC_OK (0)         成功
 *         - ULTRASONIC_ERR_NO_ECHO (-1) 未收到回波（超时 5ms）
 *         - ULTRASONIC_ERR_TOO_FAR (-2) 回波超时（距离过远）
 *         - ULTRASONIC_ERR_RANGE (-3)  结果超出 2~400cm 范围
 * @note  阻塞等待，期间 CPU 被占用，适用于简单场景。
 */
char Ultrasonic_GetDistance(float *distance);

/**
 * @brief 非阻塞式获取距离（状态机驱动）
 * @param distance 输出参数，成功时存放距离值（单位 cm）
 * @return char 状态码：
 *         - ULTRASONIC_OK (0)         测量完成，distance 有效
 *         - ULTRASONIC_BUSY (1)       测量进行中，需继续调用
 *         - ULTRASONIC_ERR_NO_ECHO (-1) 等错误码，表示测量失败
 * @note  1. 首次调用本函数会启动一次测量（状态从 IDLE 转为 TRIG_HOLD）。
 *        2. 之后应频繁调用（如主循环），直到返回非 BUSY。
 *        3. 测量完成后自动回到 IDLE 状态，可再次触发。
 *        4. 本函数需要 Timer3 的 10us 中断调用 Ultrasonic_NB_Isr() 作为状态机驱动。
 */
char Ultrasonic_GetDistance_NB(float *distance);

/**
 * @brief 非阻塞状态机服务函数，需在 10us 定时器中断中调用
 * @note  每 10us 调用一次，驱动状态机完成测距过程。
 *        该函数由 Timer3 中断服务函数调用，用户无需直接调用。
 */
void Ultrasonic_NB_Isr(void);

/**
 * @brief 雷达鸣叫任务（非阻塞）
 * @note  需在主循环中约每 10ms 调用一次。
 *        测距完成后，若距离在 2~20cm 范围内，则根据距离动态调整鸣叫间隔：
 *        - 2cm 时间隔约 100ms（急促）
 *        - 20cm 时间隔约 3000ms（缓慢）
 *        鸣叫持续 200ms 后自动停止，超出范围立即停止。
 */
void Ultrasonic_Radar_Task(void);

#endif // BSP_ULTRASONIC_H