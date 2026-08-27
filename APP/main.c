#include "STC8H.H"
#include "bsp_led.h"
#include "STC8G_H_Delay.h"
#include "bsp_motor_dirver.h"

#include "bsp_horn.h"

void main(void)
{
    unsigned char i;

    EAXSFR(); // 使用扩展SFR PWM需要
    LED_C_Init();
    LED_Init();
    EA = 1;

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

        for (i = 0; i < 28; i++)
        {
            Horn_Beep(i, 100);
        }
    }
}
