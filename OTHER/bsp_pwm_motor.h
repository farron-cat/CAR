#ifndef BSP_PWM_MOTOR_H
#define BSP_PWM_MOTOR_H

#include "STC8H_PWM.h"

extern PWMx_Duty MotorDuty;

void PWMMotorInit(void);
void PWMMotor(void);
void PWMMotorHtL(void);
void PWMMotorPot(void);
void PWMMotorShift(void);

#endif // BSP_PWM_MOTOR_H
