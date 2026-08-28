#ifndef __BSP_MOTOR_H
#define __BSP_MOTOR_H

#include "Config.h"

void Motor_Init(void);
void Motor_SetAll(int LeftFront, int RightFront, int LeftRear, int RightRear);

#endif
