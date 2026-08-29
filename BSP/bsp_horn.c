/**
 * @file    bsp_horn.c
 * @brief   蜂鸣器/喇叭驱动模块
 * @details 本模块基于 STC8H 的 PWMB（PWM8 通道）驱动蜂鸣器发声：
 *          - 使用 P3.4（PWM8_2，推挽输出）作为喇叭引脚，输出 PWM 驱动发声。
 *          - 内置音调频率表 FREQS，可按键位/音调编号播放指定频率。
 *          提供初始化、PWM 配置、按频率/音调播放、停止、开关、鸣笛（播放延时）等接口。
 * @note    依赖 STC8H_PWM.h（PWM8/PWMB 配置）、STC8G_H_Switch.h（通道切换）、
 *          STC8G_H_NVIC.h、STC8G_H_Delay.h 与 bsp_delay.h（延时）。
 * @note    播放前需先调用 Horn_Init() 完成 GPIO 与引脚切换初始化。
 * @note    FREQS 共 28 个音，对应 3.5 个八度的 C 大调音阶。
 */

#include "STC8G_H_GPIO.h"
#include "STC8H_PWM.h"
#include "STC8G_H_Switch.h"
#include "STC8G_H_NVIC.h"
#include "STC8G_H_Delay.h"

#include "bsp_horn.h"
#include "bsp_delay.h"

/**
 * @brief 内置音调频率表（C 大调音阶）
 * @note 共 28 个音，按 1、2、4、8 倍频分组（低音至高音共 3.5 个八度），
 *       每组依次为 C、D、E、F、G、A、B。供 Horn_PlayTone() 按键位查表使用。
 */
u16 code FREQS[] = {
    523 * 1, // C5
    587 * 1, // D5
    659 * 1, // E5
    698 * 1, // F5
    784 * 1, // G5
    880 * 1, // A5
    988 * 1, // B5
    523 * 2,
    587 * 2,
    659 * 2,
    698 * 2,
    784 * 2,
    880 * 2,
    988 * 2,
    523 * 4,
    587 * 4,
    659 * 4,
    698 * 4,
    784 * 4,
    880 * 4,
    988 * 4,
    523 * 8,
    587 * 8,
    659 * 8,
    698 * 8,
    784 * 8,
    880 * 8,
    988 * 8,
};

/**
 * @brief 初始化蜂鸣器/喇叭
 * @note 使能访问扩展 SFR（EAXSFR）、初始化喇叭 GPIO（Horn_GPIO_Init），
 *       并将 BUZZER 引脚置低（默认不发声）。
 */
void Horn_Init(void)
{
    EAXSFR();
    Horn_GPIO_Init();
    BUZZER = 0;
}

/**
 * @brief 初始化喇叭 GPIO
 * @note 配置 P3.4 为推挽输出，作为喇叭驱动引脚。
 */
void Horn_GPIO_Init(void)
{
    GPIO_InitTypeDef buzzerInitStruct;
    // 配置P3.4为推挽输出
    buzzerInitStruct.Mode = GPIO_OUT_PP;
    buzzerInitStruct.Pin = GPIO_Pin_4;
    GPIO_Inilize(GPIO_P3, &buzzerInitStruct);
}

/**
 * @brief 按指定频率配置喇叭 PWM 输出
 * @param freq 期望发声频率（Hz）
 * @note 先将 PWM8 通道引脚切换到 P3.4（时序更可靠），再配置 PWM8 为 PWM 模式1、
 *       占空比 50%（period/2，避开浮点运算）、输出使能 ENO8P。
 * @note 周期 period = MAIN_Fosc / freq，写入 PWMB 通用寄存器（主输出与计数器使能）。
 * @note 禁用 PWMB 中断（优先级0）。
 */
void Horn_PWM_Config(u16 freq)
{
    PWMx_InitDefine pwmStruct = {0};

    u16 period = (MAIN_Fosc / freq);

    // 先把PWM8引脚切换到P3.4，再配置外设（时序更可靠）
    PWM8_SW(PWM8_SW_P34);

    // 初始化PWM8通道
    pwmStruct.PWM_Mode = CCMRn_PWM_MODE1; // 模式
    pwmStruct.PWM_Duty = period / 2;      // 占空比50%，避开浮点运算
    pwmStruct.PWM_EnoSelect = ENO8P;      // 输出通道选择
    PWM_Configuration(PWM8, &pwmStruct);  // 初始化PWM8通道

    pwmStruct.PWM_Period = period - 1;    // 周期时间
    pwmStruct.PWM_DeadTime = 0;           // 死区发生器设置
    pwmStruct.PWM_MainOutEnable = ENABLE; // 主输出使能
    pwmStruct.PWM_CEN_Enable = ENABLE;    // 使能计数器
    PWM_Configuration(PWMB, &pwmStruct);  // 初始化PWM通用寄存器(周期/使能)

    NVIC_PWM_Init(PWMB, DISABLE, Priority_0);
}

/**
 * @brief 按指定频率播放
 * @param freq 发声频率（Hz），内部调用 Horn_PWM_Config() 配置 PWM。
 */
void Horn_PlayFreq(u16 freq)
{
    Horn_PWM_Config(freq);
}

/**
 * @brief 按指定音调编号播放
 * @param tone 音调编号（1~28，对应 FREQS 表），tone == 0 表示不发音。
 * @note tone 为 0 时调用 Horn_stop() 停止播放。
 */
void Horn_PlayTone(u16 tone)
{
    u16 freq;
    if (tone == 0)
    {
        // 不发音
        Horn_stop();
        return;
    }

    freq = FREQS[tone - 1];
    Horn_PlayFreq(freq);
}

/**
 * @brief 停止播放（关闭 PWM8 输出通道）
 * @note 将 PWM8 的输出通道选择清 0（EnoSelect = 0），从而关闭输出，不再发声。
 */
void Horn_stop()
{
    PWMx_InitDefine pwmStruct = {0};
    pwmStruct.PWM_EnoSelect = 0;         // 输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM8, &pwmStruct); // 初始化PWM,  PWMA,PWMB
}

/**
 * @brief 开启喇叭（使能 PWM8 输出）
 * @note 通过置位 PWMB_ENO 的 ENO8P 位开启喇叭输出。
 */
void Horn_On()
{
    PWMB_ENO |= ENO8P; // 开启喇叭
}

/**
 * @brief 关闭喇叭（禁用 PWM8 输出）
 * @note 通过清零 PWMB_ENO 的 ENO8P 位关闭喇叭输出。
 */
void Horn_Off(void)
{
    PWMB_ENO &= ~ENO8P; // 关闭喇叭
}

/**
 * @brief 鸣笛指定时间（播放指定音调并延时）
 * @param tone 音调编号（1~28）。
 * @param ms   鸣笛持续时长（毫秒）。
 * @note 该函数播放音调后调用 delay_ms() 阻塞延时，期间 CPU 被占用。
 */
void Horn_Beep(u16 tone, unsigned int ms)
{
    Horn_PlayTone(tone);
    delay_ms(ms);
    Horn_stop(); // 鸣笛结束停止播放，避免一直响
}