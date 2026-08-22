#include "STC8H_PWM.h"
#include "STC8G_H_Switch.h"
#include "STC8G_H_NVIC.h"
#include "STC8G_H_Delay.h"

#include "bsp_timer.h"

PWMx_Duty LEDDuty;

// 256点正弦呼吸灯表, 值域 0~24000 (对应PWM周期24000)
// 人眼对亮度感知为对数关系, 正弦曲线最自然丝滑
code unsigned int BreathTable[256] = {
    12000, 12296, 12591, 12886, 13181, 13475, 13768, 14060, 14350, 14639, 14927, 15213, 15497, 15778, 16058, 16335,
    16609, 16881, 17149, 17415, 17677, 17936, 18191, 18442, 18690, 18933, 19172, 19407, 19638, 19863, 20084, 20300,
    20511, 20717, 20918, 21113, 21302, 21486, 21665, 21837, 22003, 22163, 22318, 22465, 22607, 22742, 22870, 22992,
    23108, 23216, 23318, 23413, 23501, 23581, 23655, 23722, 23782, 23834, 23880, 23918, 23949, 23972, 23989, 23998,
    24000, 23994, 23982, 23962, 23934, 23900, 23858, 23809, 23753, 23690, 23619, 23542, 23457, 23366, 23268, 23163,
    23051, 22932, 22807, 22675, 22537, 22392, 22241, 22084, 21921, 21751, 21576, 21395, 21208, 21016, 20818, 20615,
    20407, 20193, 19974, 19751, 19523, 19290, 19053, 18812, 18567, 18317, 18064, 17807, 17546, 17283, 17016, 16745,
    16472, 16197, 15918, 15638, 15355, 15070, 14783, 14495, 14205, 13914, 13621, 13328, 13034, 12739, 12443, 12148,
    11852, 11557, 11261, 10966, 10672, 10379, 10086, 9795, 9505, 9217, 8930, 8645, 8362, 8082, 7803, 7528,
    7255, 6984, 6717, 6454, 6193, 5936, 5683, 5433, 5188, 4947, 4710, 4477, 4249, 4026, 3807, 3593,
    3385, 3182, 2984, 2792, 2605, 2424, 2249, 2079, 1916, 1759, 1608, 1463, 1325, 1193, 1068, 949,
    837, 732, 634, 543, 458, 381, 310, 247, 191, 142, 100, 66, 38, 18, 6, 0,
    2, 11, 28, 51, 82, 120, 166, 218, 278, 345, 419, 499, 587, 682, 784, 892,
    1008, 1130, 1258, 1393, 1535, 1682, 1837, 1997, 2163, 2335, 2514, 2698, 2887, 3082, 3283, 3489,
    3700, 3916, 4137, 4362, 4593, 4828, 5067, 5310, 5558, 5809, 6064, 6323, 6585, 6851, 7119, 7391,
    7665, 7942, 8222, 8503, 8787, 9073, 9361, 9650, 9940, 10232, 10525, 10819, 11114, 11409, 11704, 12000};

void PWMLEDInit(void)
{
    PWMx_InitDefine PWMx_InitStructure;

    // 初始化PWM1通道, 驱动 LED8(P2.0/PWM1P) 和 LED7(P2.1/PWM1N)
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1;    // 模式,		CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
    PWMx_InitStructure.PWM_Duty = LEDDuty.PWM1_Duty;  // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO1P | ENO1N; // 输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM1, &PWMx_InitStructure);     // 初始化PWM,  PWMA,PWMB

    // 初始化PWM2通道, 驱动 LED6(P2.2/PWM2P) 和 LED5(P2.3/PWM2N)
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1;    // 模式,		CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
    PWMx_InitStructure.PWM_Duty = LEDDuty.PWM2_Duty;  // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO2P | ENO2N; // 输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM2, &PWMx_InitStructure);     // 初始化PWM,  PWMA,PWMB

    // 初始化PWM3通道, 驱动 LED4(P1.4/PWM3P) 和 LED3(P1.5/PWM3N)
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1;    // 模式,		CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
    PWMx_InitStructure.PWM_Duty = LEDDuty.PWM3_Duty;  // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO3P | ENO3N; // 输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM3, &PWMx_InitStructure);     // 初始化PWM,  PWMA,PWMB

    // 初始化PWM4通道, 驱动 LED2(P2.6/PWM4P) 和 LED1(P2.7/PWM4N)
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1;    // 模式,		CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
    PWMx_InitStructure.PWM_Duty = LEDDuty.PWM4_Duty;  // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO4P | ENO4N; // 输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM4, &PWMx_InitStructure);     // 初始化PWM,  PWMA,PWMB

    // LED低电平点亮, 配置PWM输出为低电平有效
    PWMA_CC1P_LowValid();
    PWMA_CC1NP_LowValid();
    PWMA_CC2P_LowValid();
    PWMA_CC2NP_LowValid();
    PWMA_CC3P_LowValid();
    PWMA_CC3NP_LowValid();
    PWMA_CC4P_LowValid();
    PWMA_CC4NP_LowValid();

    PWMx_InitStructure.PWM_Period = 24000 - 1;     // 周期时间,   0~65535
    PWMx_InitStructure.PWM_DeadTime = 0;           // 死区发生器设置, 0~255
    PWMx_InitStructure.PWM_MainOutEnable = ENABLE; // 主输出使能, ENABLE,DISABLE
    PWMx_InitStructure.PWM_CEN_Enable = ENABLE;    // 使能计数器, ENABLE,DISABLE
    PWM_Configuration(PWMA, &PWMx_InitStructure);  // 初始化PWM通用寄存器,  PWMA,PWMB

    PWM1_SW(PWM1_SW_P20_P21); // LED8 LED7
    PWM2_SW(PWM2_SW_P22_P23); // LED6 LED5
    PWM3_SW(PWM3_SW_P14_P15); // LED4 LED3
    PWM4_SW(PWM4_SW_P26_P27); // 切换PWM4输出脚到P2.6/LED2 P2.7/LED1
    NVIC_PWM_Init(PWMA, DISABLE, Priority_0);
}

// 丝滑呼吸灯: 8个LED同步呼吸
// 由Timer0中断(2ms)置dutyUpdateFlag驱动, 每2ms更新一次占空比
// 256点正弦查表, 完整呼吸周期约0.5秒
void PWMBreath(void)
{
    static unsigned char i = 0;

    // 定时器未到更新时间, 直接返回
    if (dutyUpdateFlag == 0)
        return;

    dutyUpdateFlag = 0; // 清除更新标志

    LEDDuty.PWM1_Duty = BreathTable[i];
    LEDDuty.PWM2_Duty = BreathTable[i];
    LEDDuty.PWM3_Duty = BreathTable[i];
    LEDDuty.PWM4_Duty = BreathTable[i];
    UpdatePwm(PWMA, &LEDDuty);

    i++;
    if (i >= 256)
        i = 0; // 循环呼吸
}

// 左右流动波浪呼吸灯: 波浪在8个LED上左右流动一个周期
// 4个PWM通道相位依次偏移64点(90度), 形成波浪流动效果
// PWM1P(LED8) PWM2P(LED6) PWM3P(LED4) PWM4P(LED2) 依次亮起
// PWM1N(LED7) PWM2N(LED5) PWM3N(LED3) PWM4N(LED1) 反向流动
void PWMFlowWave(void)
{
    unsigned char i = 0;

    // 定时器未到更新时间, 直接返回
    if (dutyUpdateFlag == 0)
        return;

    dutyUpdateFlag = 0; // 清除更新标志

    // 4个PWM通道相位依次偏移64点(90度), 形成左右流动波浪
    LEDDuty.PWM1_Duty = BreathTable[i];
    LEDDuty.PWM2_Duty = BreathTable[(i + 64) & 0xFF];
    LEDDuty.PWM3_Duty = BreathTable[(i + 128) & 0xFF];
    LEDDuty.PWM4_Duty = BreathTable[(i + 192) & 0xFF];
    UpdatePwm(PWMA, &LEDDuty);

    i++;
    if (i >= 256)
        i = 0; // 循环流动
}
