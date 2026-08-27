#ifndef BSP_HORN_H
#define BSP_HORN_H

#define BUZZER P34 // PWM8_2

void Horn_Init(void);
void Horn_GPIO_Init(void);
void Horn_PWM_Config(u16 freq);

void Horn_PlayFreq(u16 freq);
void Horn_PlayTone(u16 tone);

void Horn_stop();

void Horn_On(void);
void Horn_Off(void);

void Horn_Beep(u16 tone, unsigned int ms);

#endif // BSP_HORN_H