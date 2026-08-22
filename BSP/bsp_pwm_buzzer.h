#ifndef BSP_PWM_BUZZER_H
#define BSP_PWM_BUZZER_H

#define BUZZER P34 // PWM8_2

void Buzzer_Init(void);
void Buzzer_Play(u16 freq);
void Buzzer_beep(u16 tone);
void Buzzer_alarm();

#endif // BSP_PWM_BUZZER_H