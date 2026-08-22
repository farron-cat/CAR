#include "STC8G_H_GPIO.h"
#include "STC8G_H_Exti.h"
#include "STC8G_H_NVIC.h"
#include "STC8G_H_Delay.h"
#include "bsp_key.h"

#include "bsp_led.h"
#include "bsp_motor.h"

/**
 * @brief 初始化独立按键 KEY_1 KEY_2 KEY_3 KEY_4
 * @note 配置P5.1、P5.2、P5.4、P5.5为上拉准双向口
 * @note 按键按下时引脚为低电平，松开时为高电平
 */
void KeyInit()
{
    // 配置P5.1 P5.2 P5.4 P5.5为上拉准双向口
    GPIO_InitTypeDef GPIO_KEY_P5_PU;
    GPIO_KEY_P5_PU.Mode = GPIO_PullUp;
    GPIO_KEY_P5_PU.Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_4 | GPIO_Pin_5;

    GPIO_Inilize(GPIO_P5, &GPIO_KEY_P5_PU);
}

/**
 * @brief 初始化核心板独立按键 KEY_C
 * @note 配置P3.2为高阻模式，按键按下时引脚被拉低
 */
void KeyCInit()
{
    // 配置 P3.2 为高阻模式
    GPIO_InitTypeDef GPIO_KEY_P32_HZ;
    GPIO_KEY_P32_HZ.Mode = GPIO_HighZ;
    GPIO_KEY_P32_HZ.Pin = GPIO_Pin_2;

    GPIO_Inilize(GPIO_P3, &GPIO_KEY_P32_HZ);
}

/**
 * @brief 初始化核心板独立按键 KEY_C 与 INT0 中断
 * @note 按键KEY_C与INT0中断引脚相连
 * @note 配置INT0为下降沿触发，使能INT0中断（优先级0）
 * @note 按键按下时产生下降沿，触发INT0中断服务函数
 */
void KeyCINTInit()
{
    // 按键KEY_C与INT0中断引脚相连

    // 配置外部中断INT0
    EXTI_InitTypeDef EXTI_KEY_P32;          // 中断模式配置结构体
    EXTI_KEY_P32.EXTI_Mode = EXT_MODE_Fall; // 下降沿触发

    Ext_Inilize(EXT_INT0, &EXTI_KEY_P32); // INT0中断初始化
    NVIC_INT0_Init(ENABLE, Priority_0);   // INT0嵌套向量中断控制器初始化

    // 配置 P3.2 为高阻模式
    KeyCInit();
}

/**
 * @brief INT0中断服务函数（按键KEY_C中断处理）
 * @note 中断触发后先延时20ms消抖，确认按键仍处于按下状态
 * @note 按键确认按下后：LEDC翻转、电机翻转
 */
void INT0_ISR_Handler(void) interrupt INT0_VECTOR
{
    delay_ms(20); // 消抖延时
    if (KEY_C == 0)
    {
        // 确认仍然按下
        LEDCToggle();   // LEDC翻转
        MOTOR = ~MOTOR; // 电机翻转
    }
}