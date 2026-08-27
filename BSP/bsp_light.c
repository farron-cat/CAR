#include "bsp_light.h"

#include "STC8G_H_GPIO.h"

#define LIGHT_RUN_PIN P53
#define LIGHT_LEFT_PIN P07
#define LIGHT_RIGHT_PIN P52
#define LIGHT_TRACK_PIN P45
#define LIGHT_RANGE_PIN P27

/**
 * @brief LED初始化
 * @note  配置相关GPIO为推挽输出，并默认关闭所有LED。
 */
void Light_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Mode = GPIO_OUT_PP;
    GPIO_InitStruct.Pin = GPIO_Pin_7;
    GPIO_Inilize(GPIO_P0, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_Pin_7;
    GPIO_Inilize(GPIO_P2, &GPIO_InitStruct);

    GPIO_InitStruct.Pin =
        GPIO_Pin_5;
    GPIO_Inilize(GPIO_P4, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_Inilize(GPIO_P5, &GPIO_InitStruct);

    LIGHT_RUN_PIN = 0;
    LIGHT_LEFT_PIN = 0;
    LIGHT_RIGHT_PIN = 0;
    LIGHT_TRACK_PIN = 0;
    LIGHT_RANGE_PIN = 0;
}

/**
 * @brief 控制LED灯状态
 * @param id 灯的编号
 * @param state 灯的状态
 */
void Light_SetState(Light_IndexTypeDef id, Light_StateTypeDef state)
{
    switch (id)
    {
    case LIGHT_RUN:
        LIGHT_RUN_PIN = state;
        break;
    case LIGHT_LEFT:
        LIGHT_LEFT_PIN = state;
        break;
    case LIGHT_RIGHT:
        LIGHT_RIGHT_PIN = state;
        break;
    case LIGHT_TRACK:
        LIGHT_TRACK_PIN = state;
        break;
    case LIGHT_RANGE:
        LIGHT_RANGE_PIN = state;
        break;
    }
}

/**
 * @brief LED状态翻转。
 * @param id 灯的编号
 */
void Light_ToggleState(Light_IndexTypeDef id)
{
    switch (id)
    {
    case LIGHT_RUN:
        LIGHT_RUN_PIN = !LIGHT_RUN_PIN;
        break;
    case LIGHT_LEFT:
        LIGHT_LEFT_PIN = !LIGHT_LEFT_PIN;
        break;
    case LIGHT_RIGHT:
        LIGHT_RIGHT_PIN = !LIGHT_RIGHT_PIN;
        break;
    case LIGHT_TRACK:
        LIGHT_TRACK_PIN = !LIGHT_TRACK_PIN;
        break;
    case LIGHT_RANGE:
        LIGHT_RANGE_PIN = !LIGHT_RANGE_PIN;
        break;
    }
}
