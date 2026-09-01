#include "STC8H_PWM.h"
#include "STC8G_H_Switch.h"
#include "STC8G_H_NVIC.h"
#include "STC8G_H_Delay.h"
#include "STC8G_H_ADC.h"

#include "bsp_timer.h"
#include "bsp_motor.h"

PWMx_Duty MotorDuty;

void PWMMotorInit(void)
{
    PWMx_InitDefine PWMx_InitStructure;
    MotorDuty.PWM6_Duty = 0; // 初始占空比 0，默认静止不震动

    // 初始化PWM6通道, 驱动
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1;     // 模式,		CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
    PWMx_InitStructure.PWM_Duty = MotorDuty.PWM6_Duty; // PWM占空比时间, 0~Period
    PWMx_InitStructure.PWM_EnoSelect = ENO6P;          // 输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM6, &PWMx_InitStructure);      // 初始化PWM,  PWMA,PWMB

    PWMx_InitStructure.PWM_Period = 24000 - 1;     // 周期时间,   0~65535
    PWMx_InitStructure.PWM_DeadTime = 0;           // 死区发生器设置, 0~255
    PWMx_InitStructure.PWM_MainOutEnable = ENABLE; // 主输出使能, ENABLE,DISABLE
    PWMx_InitStructure.PWM_CEN_Enable = ENABLE;    // 使能计数器, ENABLE,DISABLE
    PWM_Configuration(PWMB, &PWMx_InitStructure);  // 初始化PWM通用寄存器,  PWMA,PWMB

    PWM6_SW(PWM6_SW_P01); //
    NVIC_PWM_Init(PWMB, DISABLE, Priority_0);
}

// PWM控制MOTOR 从强到弱
void PWMMotor(void)
{
    static unsigned char flag = 0;

    if (!flag)
    {
        MotorDuty.PWM6_Duty++;
        if (MotorDuty.PWM6_Duty >= 24000)
        {
            flag = 1;
        }
    }
    else
    {
        MotorDuty.PWM6_Duty--;
        if (MotorDuty.PWM6_Duty <= 0)
        {
            flag = 0;
        }
    }

    UpdatePwm(PWMB, &MotorDuty); // 更新占空比
    delay_ms(1);
}

// 震动: 满幅震动1秒 -> 停止1秒 -> 从满幅逐渐减弱到0 -> 停止
// 说明: dutyUpdateFlag 由Timer0每5ms置位一次
//   满幅1秒 = 200次(5ms x 200 = 1000ms)
//   停止1秒 = 200次
//   渐弱 23760->0, 每次减40, 约594次(约3秒)
void PWMMotorHtL(void)
{
    static unsigned char state = 0; // 0=震动1秒, 1=停止1秒, 2=逐渐减弱, 3=停止
    static unsigned int count = 0;  // 阶段计数

    if (dutyUpdateFlag == 0)
        return;

    dutyUpdateFlag = 0;

    if (state == 0) // 震动1秒
    {
        MotorDuty.PWM6_Duty = 23760; // 进入震动阶段时确保满幅
        count++;
        if (count >= 200) // 1秒后停止震动
        {
            count = 0;
            MotorDuty.PWM6_Duty = 0;
            state = 1;
        }
    }
    else if (state == 1) // 停止1秒
    {
        count++;
        if (count >= 200) // 1秒后开始震动
        {
            count = 0;
            MotorDuty.PWM6_Duty = 23760;
            state = 2;
        }
    }
    else if (state == 2) // 逐渐减弱
    {
        if (MotorDuty.PWM6_Duty > 40)
        {
            MotorDuty.PWM6_Duty -= 40;
        }
        else
        {
            MotorDuty.PWM6_Duty = 0;
            state = 3;
        }
    }
    else // 停止震动
    {
        MotorOff();
        return; // 停止后不再更新PWM, 保持停止
    }

    UpdatePwm(PWMB, &MotorDuty);
}

// 电位器控制震动强度
// 电位器电压范围 1.7V~2.5V（参考电压2.5V），对应ADC码约2785~4095
// 将 [2785, 4095] 映射到占空比 [0, 23760]，电位器最小(1.7V)时停止震动
void PWMMotorPot(void)
{
    unsigned int adc = Get_ADCResult(ADC_CH13); // 电位器通道13 P0.5
    unsigned long duty;

    if (adc <= 2785) // 1.7V及以下，停止震动
    {
        duty = 0;
    }
    else
    {
        duty = (unsigned long)(adc - 2785) * 23760 / (4095 - 2785); // 2785~4095 -> 0~23760
    }

    MotorDuty.PWM6_Duty = duty;
    UpdatePwm(PWMB, &MotorDuty);
}

// 汽车加速换挡震动模拟
// 用PWM占空比模拟发动机转速(占空比越大震动越强)
// 5个档位: 每档从低转速加速到高转速, 换挡瞬间转速回落并短暂停顿
// 档位越高, 起始/结束转速越高, 加速斜率越大(模拟高档位加速更快)
// 每档加速约1秒, 最高转速保持约500ms, 换挡停顿约200ms, 5档完整循环约8秒
// 各档位参数: 起始占空比, 结束占空比, 加速步长
code unsigned int GearStart[5] = {3000, 5000, 7000, 9000, 11000};  // 各档起始转速(换挡后回落值)
code unsigned int GearEnd[5] = {8000, 11000, 14000, 17000, 20000}; // 各档最高转速(换挡前)
code unsigned char GearStep[5] = {25, 30, 35, 40, 45};             // 各档加速斜率(高档更快)

void PWMMotorShift(void)
{
    static unsigned char gear = 0;  // 当前档位 0~4 (对应1~5档)
    static unsigned int duty = 0;   // 当前占空比(转速)
    static unsigned char state = 0; // 0=加速中, 1=换挡停顿
    static unsigned char pause = 0; // 换挡停顿计数

    if (state == 0) // 加速中
    {
        duty += GearStep[gear];
        if (duty >= GearEnd[gear])
        {
            duty = GearEnd[gear];
            state = 1; // 达到档位上限, 进入换挡
            pause = 0;
        }
    }
    else // 最高转速保持 + 换挡停顿
    {
        pause++;
        if (pause >= 100) // 最高转速保持约500ms (100 * 5ms) 后换挡
        {
            gear++;
            if (gear >= 5)
                gear = 0;           // 5档换完回到1档重新起步
            duty = GearStart[gear]; // 转速回落到下一档起始值
            state = 0;              // 继续加速
        }
    }

    MotorDuty.PWM6_Duty = duty;
    UpdatePwm(PWMB, &MotorDuty); // 更新占空比
    delay_ms(5);
}
