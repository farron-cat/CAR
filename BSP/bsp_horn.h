/**
 * @file    bsp_horn.h
 * @brief   蜂鸣器/喇叭驱动模块接口
 * @details 基于 STC8H PWMB（PWM8 通道）的喇叭驱动接口：
 *          - BUZZER：喇叭引脚，接 P3.4（PWM8_2）。
 *          - 接口：初始化、PWM 配置、按频率/音调播放、停止、开关、鸣笛。
 * @note    播放前需先调用 Horn_Init() 完成初始化。
 */
#ifndef BSP_HORN_H
#define BSP_HORN_H

#define BUZZER P34 // PWM8_2

/**
 * @brief 初始化蜂鸣器/喇叭（使能扩展SFR、初始化GPIO、默认关闭）
 */
void Horn_Init(void);

/**
 * @brief 初始化喇叭 GPIO（配置 P3.4 为推挽输出）
 */
void Horn_GPIO_Init(void);

/**
 * @brief 按指定频率配置喇叭 PWM 输出
 * @param freq 期望发声频率（Hz）
 */
void Horn_PWM_Config(u16 freq);

/**
 * @brief 按指定频率播放
 * @param freq 发声频率（Hz）
 */
void Horn_PlayFreq(u16 freq);

/**
 * @brief 按指定音调编号播放
 * @param tone 音调编号（1~28 对应 FREQS 表）；0 表示不发音
 */
void Horn_PlayTone(u16 tone);

/**
 * @brief 停止播放（关闭 PWM8 输出通道）
 */
void Horn_stop();

/**
 * @brief 开启喇叭（使能 PWM8 输出）
 */
void Horn_On(void);

/**
 * @brief 关闭喇叭（禁用 PWM8 输出）
 */
void Horn_Off(void);

/**
 * @brief 鸣笛指定时间（播放指定音调并阻塞延时）
 * @param tone 音调编号（1~28）
 * @param ms   鸣笛持续时长（毫秒）
 */
void Horn_Beep(u16 tone, unsigned int ms);

#endif // BSP_HORN_H