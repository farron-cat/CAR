#ifndef BSP_PWM_LED_H
#define BSP_PWM_LED_H

#include "STC8H_PWM.h"

extern PWMx_Duty LEDDuty;

void PWMLEDInit(void);
void PWMBreath(void);
void PWMFlowWave(void);

#endif // BSP_PWM_LED_H
