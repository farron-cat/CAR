/**
 * @file    bsp_ultrasonic.c
 * @brief   Ultrasonic driver: blocking + non-blocking APIs.
 * @note    System clock 24MHz; time unit = 10us.
 */

#include "bsp_ultrasonic.h"
#include "STC8G_H_GPIO.h" // GPIO_Inilize / GPIO_InitTypeDef
#include "bsp_delay.h"    // delay_us (blocking API)

/**
 * @brief Init pinst + Timer3 10us ISR tick.
 */
void Ultrasonic_Init(void)
{
    GPIO_InitTypeDef trigInitStruct = {0};
    GPIO_InitTypeDef echoInitStruct = {0};

    // TRIG: push-pull output
    trigInitStruct.Mode = GPIO_OUT_PP;
    trigInitStruct.Pin = GPIO_Pin_7;
    GPIO_Inilize(GPIO_P4, &trigInitStruct);

    // ECHO: quasi-bidirectional (pull-up input)
    echoInitStruct.Mode = GPIO_HighZ;
    echoInitStruct.Pin = GPIO_Pin_3;
    GPIO_Inilize(GPIO_P3, &echoInitStruct);
}

/**
 * @brief Blocking distance measurement.
 */
char Ultrasonic_GetDistance(float *distance)
{
    u16 cnt;

    // 1. TRIG high >=10us (2x margin), then low
    TRIG = 1;
    delay_us(10);
    delay_us(10);
    TRIG = 0;

    // 2. Wait ECHO high, timeout 500*10us = 5ms
    cnt = 0;
    while (ECHO == 0 && cnt < 500)
    {
        cnt++;
        delay_us(10);
    }
    if (cnt >= 500)
        return ULTRASONIC_ERR_NO_ECHO;

    // 3. Measure ECHO high width, timeout 3000*10us = 30ms
    cnt = 0;
    while (ECHO == 1 && cnt < 3000)
    {
        cnt++;
        delay_us(10);
    }
    if (cnt >= 3000)
        return ULTRASONIC_ERR_TOO_FAR;

    // 4. dist(cm) = (cnt * 0.01ms) * 34(cm/ms) / 2
    *distance = ((cnt * 0.01f) * 34.0f) / 2.0f;

    // 5. valid range 2~400cm
    if (*distance < 2.0f || *distance > 400.0f)
        return ULTRASONIC_ERR_RANGE;

    return ULTRASONIC_OK;
}

/* ---------- Non-blocking: Timer3 10us ISR state machine ---------- */

#define US_IDLE 0      // idle
#define US_TRIG_HOLD 1 // keep TRIG high
#define US_WAIT_HIGH 2 // wait ECHO rising
#define US_MEASURE 3   // count ECHO high width
#define US_DONE 4      // result ready

static volatile u8 s_usState = US_IDLE;
static volatile u16 s_usCnt10 = 0;
static volatile u16 s_usResultCnt = 0;
static volatile char s_usError = 0;

void Ultrasonic_NB_Isr(void)
{
    switch (s_usState)
    {
    case US_IDLE:
        break;

    case US_TRIG_HOLD:
        if (++s_usCnt10 >= 2)
        {
            TRIG = 0;
            s_usCnt10 = 0;
            s_usState = US_WAIT_HIGH;
        }
        break;

    case US_WAIT_HIGH:
        if (ECHO == 1)
        {
            s_usCnt10 = 0;
            s_usState = US_MEASURE;
        }
        else if (++s_usCnt10 >= 500)
        {
            s_usError = ULTRASONIC_ERR_NO_ECHO;
            s_usState = US_DONE;
        }
        break;

    case US_MEASURE:
        if (ECHO == 0)
        {
            s_usResultCnt = s_usCnt10;
            s_usError = 0;
            s_usState = US_DONE;
        }
        else if (++s_usCnt10 >= 3000)
        {
            s_usError = ULTRASONIC_ERR_TOO_FAR;
            s_usState = US_DONE;
        }
        break;

    default:
        break;
    }
}

char Ultrasonic_GetDistance_NB(float *distance)
{
    if (s_usState == US_DONE)
    {
        u16 cnt;
        char err;

        EA = 0;
        cnt = s_usResultCnt;
        err = s_usError;
        s_usState = US_IDLE;
        s_usResultCnt = 0;
        s_usError = 0;
        EA = 1;

        if (err != 0)
            return err;

        *distance = ((cnt * 0.01f) * 34.0f) / 2.0f;
        if (*distance < 2.0f || *distance > 400.0f)
            return ULTRASONIC_ERR_RANGE;
        return ULTRASONIC_OK;
    }

    if (s_usState == US_IDLE)
    {
        EA = 0;
        s_usState = US_TRIG_HOLD;
        s_usCnt10 = 0;
        s_usError = 0;
        TRIG = 1;
        EA = 1;
    }

    return ULTRASONIC_BUSY;
}
