#ifndef __BSP_MOTOR_H
#define __BSP_MOTOR_H

#include "Config.h"

typedef enum
{
    Motor_LF = 1,   /* 左前 */
    Motor_RF,       /* 右前 */
    Motor_LR,       /* 左后 */
    Motor_RR,       /* 右后 */
} Motor_IndexTypeDef;

void Motor_Init(void);
void Motor_SetAll(int LeftFront, int RightFront, int LeftRear, int RightRear);

#endif
