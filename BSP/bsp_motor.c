#include "STC8G_H_GPIO.h"
#include "bsp_motor.h"

void MotorInit(void)
{
    // 配置P0.1为推挽输出
    GPIO_InitTypeDef GPIO_MOTOR_P01_PP = {0};
    GPIO_MOTOR_P01_PP.Mode = GPIO_OUT_PP;
    GPIO_MOTOR_P01_PP.Pin = GPIO_P1;
    GPIO_Inilize(GPIO_P0, &GPIO_MOTOR_P01_PP);

    // 默认关闭状态
    MOTOR = 0;
}

void MotorOn()
{
    MOTOR = 1;
}

void MotorOff()
{
    MOTOR = 0;
}