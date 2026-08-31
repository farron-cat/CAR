#ifndef BSP_LIGHT_H
#define BSP_LIGHT_H

#include "Config.h"

/**
 * @brief 车灯指示灯枚举
 */
typedef enum
{
    LIGHT_RUN = 0, /* 运行指示灯 */
    LIGHT_LEFT,    /* 左车灯 */
    LIGHT_RIGHT,   /* 右车灯 */
    LIGHT_TRACK,   /* 巡线指示灯 */
    LIGHT_RANGE    /* 测距指示灯 */
} Light_IndexTypeDef;

/**
 * @brief 车灯状态枚举
 */
typedef enum
{
    LIGHT_OFF = 0,
    LIGHT_ON = 1
} Light_StateTypeDef;

void Light_Init(void);
void Light_SetState(Light_IndexTypeDef id, Light_StateTypeDef state);
void Light_ToggleState(Light_IndexTypeDef id);

#endif