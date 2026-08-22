#ifndef BSP_LED_H
#define BSP_LED_H

#define LEDOn(led) ((led) = 0)
#define LEDOff(led) ((led) = 1)

#define LED_C P53

#define LED_L P07
#define LED_R P52

#define LED_LINE P45

#define LED_ECHO P27

void LED_C_Init(void);
void LED_C_On(void);
void LED_C_Off(void);
void LED_C_Toggle(void);

void LED_Init(void);
void LED_Toggle(void);

void LED_L_On(void);
void LED_L_Off(void);
void LED_L_Toggle(void);

void LED_R_On(void);
void LED_R_Off(void);
void LED_R_Toggle(void);

void LED_LINE_On(void);
void LED_LINE_Off(void);
void LED_LINE_Toggle(void);

void LED_ECHO_On(void);
void LED_ECHO_Off(void);
void LED_ECHO_Toggle(void);

#endif // BSP_LED_H