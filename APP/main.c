#include "STC8H.H"
#include "bsp_led.h"
#include "STC8G_H_Delay.h"
#include "bsp_motor_dirver.h"

#include "bsp_horn.h"
#include "bsp_motor_io.h"

void main(void)
{
    unsigned char i;

    EAXSFR(); // 使用扩展SFR PWM需要
    LED_C_Init();
    LED_Init();
    EA = 1;

    Motor_Init();
    while (1)
    {
        // delay_ms(200);
        // LED_C_Toggle();
        // LED_Toggle();

        // Motors_Forward(100);
        // Motors_Backward(100);
        // Motors_Left(50, 0);

        // for (i = 0; i < 10; i++)
        // {
        //     Motors_Forward(i * 10);
        //     delay_ms(2000);
        // }

        // for (i = 0; i < 28; i++)
        // {
        //     Horn_Beep(i, 100);
        // }
        // Motor_SetDirection(MOTOR_FORWARD);

        Motor_RunWithDelay(MOTOR_FORWARD, 70, 2000);
        // delay_ms(2000);
        Motor_RunWithDelay(MOTOR_FORWARD, 50, 2000);
        // delay_ms(2000);
        Motor_RunWithDelay(MOTOR_FORWARD, 30, 2000);
        // delay_ms(2000);
    }
}
