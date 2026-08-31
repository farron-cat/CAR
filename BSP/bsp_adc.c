#include "STC8G_H_GPIO.h"
#include "STC8G_H_ADC.h"
#include "STC8G_H_NVIC.h"

// ADC初始化，包含电位器电压和NTC热敏电阻对应的GPIO初始化
void ADC_Init(void)
{
    // 定义初始化结构体
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    // P1.3 设置为高阻输入 电池电压 channel 3
    GPIO_InitStructure.Pin = GPIO_Pin_3;
    GPIO_InitStructure.Mode = GPIO_HighZ;
    GPIO_Inilize(GPIO_P1, &GPIO_InitStructure);

    // 初始化ADC
    ADC_InitStructure.ADC_SMPduty = 31;                    // ADC 模拟信号采样时间控制, 0~31（注意： SMPDUTY 一定不能设置小于 10）
    ADC_InitStructure.ADC_CsSetup = 0;                     // ADC 通道选择时间控制 0(默认),1
    ADC_InitStructure.ADC_CsHold = 1;                      // ADC 通道选择保持时间控制 0,1(默认),2,3
    ADC_InitStructure.ADC_Speed = ADC_SPEED_2X16T;         // 设置 ADC 工作时钟频率	ADC_SPEED_2X1T~ADC_SPEED_2X16T
    ADC_InitStructure.ADC_AdjResult = ADC_RIGHT_JUSTIFIED; // ADC结果调整,	ADC_LEFT_JUSTIFIED,ADC_RIGHT_JUSTIFIED
    ADC_Inilize(&ADC_InitStructure);                       // 初始化
    ADC_PowerControl(ENABLE);                              // ADC电源开关, ENABLE或DISABLE
    NVIC_ADC_Init(DISABLE, Priority_0);                    // 中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
}

// 读取电池电压
float ADC_Battary_Voltage(void)
{
    unsigned int adcCode = 0;
    float voltage = 0.0f;

    adcCode = Get_ADCResult(ADC_CH3);      // 注意通道选择 channel 3 P1.3 (查表得到)
    voltage = (float)adcCode * 2.5 / 4096; // 2.5V是参考电压，4096是12位分辨率（0~4095）

    // 电阻比 10/61
    voltage = voltage * 61 / 10;

    return voltage;
}