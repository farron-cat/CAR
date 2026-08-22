#include "STC8H.H"
#include <intrins.h>
#include "bsp_delay.h"

void Delay25ms(void) //@24.000MHz
{
    unsigned char data i, j, k;

    _nop_();
    _nop_();
    i = 3;
    j = 72;
    k = 161;
    do
    {
        do
        {
            while (--k)
                ;
        } while (--j);
    } while (--i);
}

void Delay50ms(void) //@24.000MHz
{
    unsigned char data i, j, k;

    _nop_();
    i = 7;
    j = 23;
    k = 105;
    do
    {
        do
        {
            while (--k)
                ;
        } while (--j);
    } while (--i);
}

void Delay200ms(void) //@24.000MHz
{
    unsigned char data i, j, k;

    _nop_();
    _nop_();
    i = 19;
    j = 62;
    k = 43;
    do
    {
        do
        {
            while (--k)
                ;
        } while (--j);
    } while (--i);
}

void Delay500ms(void) //@24.000MHz
{
    unsigned char data i, j, k;

    _nop_(); // keil 空操作函数
    _nop_();
    i = 46;
    j = 153;
    k = 245;
    do
    {
        do
        {
            while (--k)
                ;
        } while (--j);
    } while (--i);
}

void Delay600ms(void) //@24.000MHz
{
    unsigned char data i, j, k;

    _nop_();
    _nop_();
    i = 55;
    j = 184;
    k = 141;
    do
    {
        do
        {
            while (--k)
                ;
        } while (--j);
    } while (--i);
}

void Delay1000ms(void) //@24.000MHz
{
    unsigned char data i, j, k;

    _nop_();
    _nop_();
    i = 122;
    j = 193;
    k = 128;
    do
    {
        do
        {
            while (--k)
                ;
        } while (--j);
    } while (--i);
}

void Delay2000ms(void) //@24.000MHz
{
    unsigned char data i, j, k;

    _nop_();
    _nop_();
    i = 183;
    j = 100;
    k = 225;
    do
    {
        do
        {
            while (--k)
                ;
        } while (--j);
    } while (--i);
}

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

// void delay_ms(unsigned int ms)
// {
//     while (ms--)
//     {
//         delay_us(1000);
//     }
// }

/**
 * Delay956us - Do 音半周期延时
 * @note 用于生成 Do 音的方波，高电平或低电平持续约 956μs
 * @note 适用于 24.000MHz 系统时钟
 */
void Delay956us(void)
{
    unsigned char data i, j;

    _nop_();
    _nop_();
    i = 30;
    j = 201;
    do
    {
        while (--j)
            ;
    } while (--i);
}

// 高音do
void Delay478us(void) //@24.000MHz
{
    unsigned char data i, j;

    i = 12;
    j = 37;
    do
    {
        while (--j)
            ;
    } while (--i);
}

void Delay852us(void) //@24.000MHz
{
    unsigned char data i, j;

    _nop_();
    _nop_();
    i = 27;
    j = 139;
    do
    {
        while (--j)
            ;
    } while (--i);
}

// 高音re
void Delay426us(void) //@24.000MHz
{
    unsigned char data i, j;

    i = 10;
    j = 239;
    do
    {
        while (--j)
            ;
    } while (--i);
}

void Delay759us(void) //@24.000MHz
{
    unsigned char data i, j;

    _nop_();
    _nop_();
    i = 24;
    j = 165;
    do
    {
        while (--j)
            ;
    } while (--i);
}

// 高音mi
void Delay383us(void) //@24.000MHz
{
    unsigned char data i, j;

    i = 9;
    j = 238;
    do
    {
        while (--j)
            ;
    } while (--i);
}

void Delay716us(void) //@24.000MHz
{
    unsigned char data i, j;

    _nop_();
    i = 23;
    j = 78;
    do
    {
        while (--j)
            ;
    } while (--i);
}
// 高音fa
void Delay347us(void) //@24.000MHz
{
    unsigned char data i, j;

    i = 9;
    j = 22;
    do
    {
        while (--j)
            ;
    } while (--i);
}

void Delay638us(void) //@24.000MHz
{
    unsigned char data i, j;

    _nop_();
    i = 20;
    j = 224;
    do
    {
        while (--j)
            ;
    } while (--i);
}

// 高音so
void Delay311us(void) //@24.000MHz
{
    unsigned char data i, j;

    i = 8;
    j = 63;
    do
    {
        while (--j)
            ;
    } while (--i);
}

void Delay568us(void) //@24.000MHz
{
    unsigned char data i, j;

    _nop_();
    _nop_();
    i = 18;
    j = 177;
    do
    {
        while (--j)
            ;
    } while (--i);
}

void Delay276us(void) //@24.000MHz
{
    unsigned char data i, j;

    i = 7;
    j = 110;
    do
    {
        while (--j)
            ;
    } while (--i);
}

void Delay506us(void) //@24.000MHz
{
    unsigned char data i, j;

    i = 12;
    j = 205;
    do
    {
        while (--j)
            ;
    } while (--i);
}

void Delay244us(void) //@24.000MHz
{
    unsigned char data i, j;

    i = 6;
    j = 175;
    do
    {
        while (--j)
            ;
    } while (--i);
}