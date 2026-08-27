#ifndef BSP_LED_H
#define BSP_LED_H

#define LEDOn(led) ((led) = 0)
#define LEDOff(led) ((led) = 1)

#define LED_C P53 // 核心板LED

#define LED_L P07 // 左车灯
#define LED_R P52 // 右车灯

#define LED_LINE P45 // 巡线指示灯

#define LED_ECHO P27 // 超声波指示灯

// 核心板LED功能
void LED_C_Init(void);
void LED_C_On(void);
void LED_C_Off(void);
void LED_C_Toggle(void);

// 车灯与指示灯功能
void LED_Init(void);
void LED_Toggle(void);
// 左车灯
void LED_L_On(void);
void LED_L_Off(void);
void LED_L_Toggle(void);
// 右车灯
void LED_R_On(void);
void LED_R_Off(void);
void LED_R_Toggle(void);
// 巡线指示灯
void LED_LINE_On(void);
void LED_LINE_Off(void);
void LED_LINE_Toggle(void);
// 超声波指示灯
void LED_ECHO_On(void);
void LED_ECHO_Off(void);
void LED_ECHO_Toggle(void);

#endif // BSP_LED_H