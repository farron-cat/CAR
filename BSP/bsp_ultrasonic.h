/**
 * @file    bsp_ultrasonic.h
 * @brief   超声波测距模块（HC-SR04）接口定义
 * @details 基于 INT1 外部中断 + Timer3 硬件计时实现测距：
 *          - TRIG（P4.7）：推挽输出，产生触发信号（≥10us 高电平）。
 *          - ECHO（P3.3）：高阻输入，同时作为 INT1 外部中断输入，
 *            上升沿触发中断并启动 Timer3 计时，下降沿中断读取回波宽度。
 * @note    系统时钟 24MHz；Timer3 配置为 12T 自由计数（0.5us/计数）。
 *          调用 Ultrasonic_Init() 后即完成 GPIO / INT1 / Timer3 初始化，
 *          无需额外开启定时器。
 */

#ifndef BSP_ULTRASONIC_H
#define BSP_ULTRASONIC_H

#include "STC8H.H" // 包含 sbit 引脚定义（P33 / P47）

// 引脚定义
#define TRIG P47 // 触发引脚（P4.7）
#define ECHO P33 // 回波引脚（P3.3，INT1）

// 返回值状态码
#define ULTRASONIC_OK 0           // 测距成功
#define ULTRASONIC_BUSY 1         // 非阻塞测距进行中
#define ULTRASONIC_ERR_NO_ECHO -1 // 未收到回波（超时）
#define ULTRASONIC_ERR_TOO_FAR -2 // 回波宽度超时（距离过远）
#define ULTRASONIC_ERR_RANGE -3   // 结果超出有效范围（2~400cm）

// 函数声明

/**
 * @brief 初始化超声波模块（GPIO + INT1 + Timer3）
 * @note  配置 TRIG（P4.7）为推挽输出，ECHO（P3.3）为高阻输入并作为 INT1。
 *        Timer3 配置为 12T 自由计数（0.5us/计数）用于回波计时。
 *        INT1 使能中断（上升沿/下降沿），优先级 Priority_1。
 */
void Ultrasonic_Init(void);

/**
 * @brief 阻塞式单次测距
 * @param distance 输出参数，存放测量得到的距离值（单位 cm）
 * @return char 执行状态码：
 *         - ULTRASONIC_OK (0)         成功
 *         - ULTRASONIC_ERR_NO_ECHO (-1) 未收到回波（超时）
 *         - ULTRASONIC_ERR_TOO_FAR (-2) 回波超时（距离过远）
 *         - ULTRASONIC_ERR_RANGE (-3)  结果超出 2~400cm 范围
 * @note  基于非阻塞接口忙等实现，内部有 60ms 超时保护，不会无限阻塞。
 */
char Ultrasonic_GetDistance(float *distance);

/**
 * @brief 非阻塞式获取距离（INT1 中断 + Timer3 计时）
 * @param distance 输出参数，成功时存放距离值（单位 cm）
 * @return char 状态码：
 *         - ULTRASONIC_OK (0)         测量完成，distance 有效
 *         - ULTRASONIC_BUSY (1)       测量进行中，需继续调用
 *         - ULTRASONIC_ERR_NO_ECHO (-1) 等错误码，表示测量失败
 * @note  1. 首次调用本函数会启动一次测量（空闲时产生 TRIG 触发脉冲）。
 *        2. 之后应频繁调用（如主循环），直到返回非 BUSY。
 *        3. 测量完成后自动回到 IDLE 状态，可再次触发。
 *        4. ECHO 上升沿/下降沿由 INT1 中断处理，Timer3 硬件计时，
 *           测量精度不依赖主循环调用频率。
 */
char Ultrasonic_GetDistance_NB(float *distance);

/**
 * @brief 雷达鸣叫任务（非阻塞）
 * @note  需在主循环中约每 10ms 调用一次。
 *        测距完成后，若距离在 2~20cm 范围内，则根据距离动态调整鸣叫间隔：
 *        - 2cm 时间隔约 100ms（急促）
 *        - 20cm 时间隔约 3000ms（缓慢）
 *        鸣叫持续 100ms 后自动停止，超出范围立即停止。
 */
void Ultrasonic_Radar_Task(void);

#endif // BSP_ULTRASONIC_H
