/**
 * @file    bsp_ultrasonic.h
 * @brief   Ultrasonic sensor interface (HC-SR04)
 * @details TRIG : P3.3 push-pull output, >=10us high pulse triggers.
 *          ECHO : P4.7 pull-up input, high width proportional to distance.
 *          Blocking API  : Ultrasonic_GetDistance()
 *          Non-block API : Ultrasonic_GetDistance_NB() (Timer3 10us ISR)
 */
#ifndef BSP_ULTRASONIC_H
#define BSP_ULTRASONIC_H

#include "STC8H.H" // sbit pins (P33 / P47)

#define TRIG P47 // trigger pin
#define ECHO P33 // echo pin

/* Return status (0 OK, 1 busy, negative error) */
#define ULTRASONIC_OK 0
#define ULTRASONIC_BUSY 1
#define ULTRASONIC_ERR_NO_ECHO -1
#define ULTRASONIC_ERR_TOO_FAR -2
#define ULTRASONIC_ERR_RANGE -3

void Ultrasonic_Init(void);

char Ultrasonic_GetDistance(float *distance);

char Ultrasonic_GetDistance_NB(float *distance);

void Ultrasonic_NB_Isr(void);

#endif // BSP_ULTRASONIC_H