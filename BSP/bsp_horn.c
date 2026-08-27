#include "STC8G_H_GPIO.h"
#include "STC8H_PWM.h"
#include "STC8G_H_Switch.h"
#include "STC8G_H_NVIC.h"
#include "STC8G_H_Delay.h"

#include "bsp_horn.h"
#include "bsp_delay.h"

u16 code FREQS[] = {
    523 * 1,
    587 * 1,
    659 * 1,
    698 * 1,
    784 * 1,
    880 * 1,
    988 * 1,
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

// 初始化喇叭
void Horn_Init(void)
{
    EAXSFR();
    Horn_GPIO_Init();
    BUZZER = 0;
}

// 初始化喇叭GPIO
void Horn_GPIO_Init(void)
{
    GPIO_InitTypeDef buzzerInitStruct;
    // 配置P3.4为推挽输出
    buzzerInitStruct.Mode = GPIO_OUT_PP;
    buzzerInitStruct.Pin = GPIO_Pin_4;
    GPIO_Inilize(GPIO_P3, &buzzerInitStruct);
}

// 配置喇叭PWM
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

// 按照指定频率播放
void Horn_PlayFreq(u16 freq)
{
    Horn_PWM_Config(freq);
}

// 按照指定音调播放
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

// 停止播放
void Horn_stop()
{
    PWMx_InitDefine pwmStruct = {0};
    pwmStruct.PWM_EnoSelect = 0;         // 输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM8, &pwmStruct); // 初始化PWM,  PWMA,PWMB
}

// 开启喇叭
void Horn_On()
{
    PWMB_ENO |= ENO8P; // 开启喇叭
}

// 关闭喇叭
void Horn_Off(void)
{
    PWMB_ENO &= ~ENO8P; // 关闭喇叭
}

// 鸣笛指定时间
void Horn_Beep(u16 tone, unsigned int ms)
{
    Horn_PlayTone(tone);
    delay_ms(ms);
}