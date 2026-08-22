#include "STC8H.H"
#include "STC8G_H_GPIO.h"
#include "bsp_led.h"

#define ON 0
#define OFF 1

void LED_C_Init(void)
{
    GPIO_InitTypeDef ledcInitStruct = {0};
    ledcInitStruct.Mode = GPIO_OUT_PP;
    ledcInitStruct.Pin = GPIO_Pin_3;

    GPIO_Inilize(GPIO_P5, &ledcInitStruct);
}

void LED_C_On(void)
{
    LED_C = 1;
}

void LED_C_Off(void)
{
    LED_C = 0;
}

void LED_C_Toggle(void)
{
    LED_C = ~LED_C;
}

void LED_Init(void)
{
    GPIO_InitTypeDef ledInitStruct = {0};

    // 配置P0.7为推挽输出
    ledInitStruct.Mode = GPIO_OUT_PP;
    ledInitStruct.Pin = GPIO_Pin_7;
    GPIO_Inilize(GPIO_P0, &ledInitStruct);

    // 配置P5.2为推挽输出
    ledInitStruct.Mode = GPIO_OUT_PP;
    ledInitStruct.Pin = GPIO_Pin_2;
    GPIO_Inilize(GPIO_P5, &ledInitStruct);

    // 配置P4.5为推挽输出
    ledInitStruct.Mode = GPIO_OUT_PP;
    ledInitStruct.Pin = GPIO_Pin_5;
    GPIO_Inilize(GPIO_P4, &ledInitStruct);

    // 配置P2.7为推挽输出
    ledInitStruct.Mode = GPIO_OUT_PP;
    ledInitStruct.Pin = GPIO_Pin_7;
    GPIO_Inilize(GPIO_P2, &ledInitStruct);
}

void LED_Toggle(void)
{
    LED_L = ~LED_L;
    LED_R = ~LED_R;
    LED_LINE = ~LED_LINE;
    LED_ECHO = ~LED_ECHO;
}

void LED_L_On(void)
{
    LED_L = 1;
}

void LED_L_Off(void)
{
    LED_L = 0;
}

void LED_L_Toggle(void)
{
    LED_L = ~LED_L;
}

void LED_R_On(void)
{
    LED_R = 1;
}

void LED_R_Off(void)
{
    LED_R = 0;
}

void LED_R_Toggle(void)
{
    LED_R = ~LED_R;
}

void LED_LINE_On(void)
{
    LED_LINE = 1;
}

void LED_LINE_Off(void)
{
    LED_LINE = 0;
}

void LED_LINE_Toggle(void)
{
    LED_LINE = ~LED_LINE;
}

void LED_ECHO_On(void)
{
    LED_ECHO = 1;
}

void LED_ECHO_Off(void)
{
    LED_ECHO = 0;
}

void LED_ECHO_Toggle(void)
{
    LED_ECHO = ~LED_ECHO;
}
