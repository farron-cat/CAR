#include "STC8H.H"
#include <intrins.h>
#include "bsp_delay.h"

void delay_us(unsigned int us)
{
    unsigned int i;
    // 1T 单片机，循环体约消耗 4 个时钟周期
    // 故每微秒需要的循环次数 = FOSC/1000000 / 4
    // 但还要考虑函数调用和循环控制开销，实际可能需微调
    unsigned int loop = 6; // 24MHz -> 6

    while (us--)
    {
        i = loop;
        while (i--)
            ;
    }
}
