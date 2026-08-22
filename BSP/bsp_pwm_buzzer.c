#include "STC8G_H_GPIO.h"
#include "STC8G_H_PWM.h"
#include "STC8G_H_Switch.h"
#include "STC8G_H_NVIC.h"

#include "bsp_pwm_buzzer.h"

//			  C`	D`    E`    F`	  G`	A`	  B`    C``
// u16 hz[] = {1047, 1175, 1319, 1397, 1568, 1760, 1976, 2093};
//			  C	    D    E 	 F	  G	   A	B	 C`
// u16 hz[] = {523, 587, 659, 698, 784, 880, 988, 1047};
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

// 初始化蜂鸣器GPIO
void Buzzer_GPIO_Init(void)
{
    GPIO_InitTypeDef buzzerInitStruct;
    // 配置P3.4为推挽输出
    ledInitStruct.Mode = GPIO_OUT_PP;
    ledInitStruct.Pin = GPIO_Pin_4;
    GPIO_Inilize(GPIO_P3, &buzzerInitStruct);
}

// 配置蜂鸣器PWM
void Buzzer_PWM_Config(u16 freq)
{
    PWMx_InitDefine pwmStruct;

    u16 period = (MAIN_Fosc / freq);

    // 初始化PWM8通道
    pwmStruct.PWM_Mode = CCMRn_PWM_MODE1;   // 模式
    pwmStruct.PWM_Duty = (u16)period * 0.5; // PWM占空比时间, 0~Period
    pwmStruct.PWM_EnoSelect = ENO8P;        // 输出通道选择
    PWM_Configuration(PWM8, &pwmStruct);    // 初始化PWM

    pwmStruct.PWM_Period = period - 1;    // 周期时间
    pwmStruct.PWM_DeadTime = 0;           // 死区发生器设置
    pwmStruct.PWM_MainOutEnable = ENABLE; // 主输出使能
    pwmStruct.PWM_CEN_Enable = ENABLE;    // 使能计数器
    PWM_Configuration(PWMB, &pwmStruct);  // 初始化PWM通用寄存器

    PWM8_SW(PWM8_SW_P34); // PWM8_SW_P34
    NVIC_PWM_Init(PWMB, DISABLE, Priority_0);
}

// 初始化蜂鸣器
void Buzzer_Init(void)
{
    EAXSFR();
    Buzzer_GPIO_Init();
    BUZZER = 0;
}

// 按照指定频率播放
void Buzzer_play(u16 freq)
{
    PWM_config(freq);
}

// 根据索引取出对应的音调
void Buzzer_beep(u16 tone)
{
    u16 freq;
    if (tone == 0)
    {
        // 不发音
        Buzzer_stop();
        return;
    }

    freq = FREQS[tone - 1];
    Buzzer_play(freq);
}

// 停止播放
void Buzzer_stop()
{

    PWMx_InitDefine pwmStruct;
    pwmStruct.PWM_EnoSelect = 0;         // 输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM8, &pwmStruct); // 初始化PWM,  PWMA,PWMB
}

void Buzzer_alarm()
{
    Buzzer_beep(10);
}
