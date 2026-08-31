/**
 * @file    bsp_ultrasonic.c
 * @brief   超声波测距驱动模块（阻塞 + 非阻塞 API）
 * @details 本模块提供两种测距方式：
 *          - 阻塞式：Ultrasonic_GetDistance()，通过延时等待回波，简单直接。
 *          - 非阻塞式：Ultrasonic_GetDistance_NB()，基于定时器中断状态机，
 *            配合 Ultrasonic_NB_Isr()（需在 10us 定时器中断中调用）实现。
 *          另附雷达鸣叫任务 Ultrasonic_Radar_Task()，根据测距结果动态调整
 *          鸣叫间隔（2cm~20cm 线性映射为 100ms~3000ms）。
 * @note    系统时钟 24MHz，时间基准为 10us（由定时器3中断提供）。
 *          依赖 bsp_delay.h（阻塞延时）、bsp_horn.h（鸣叫）、bsp_timer.h（tickMs）。
 */

#include "bsp_ultrasonic.h"
#include "STC8G_H_GPIO.h" // GPIO_Inilize / GPIO_InitTypeDef
#include "bsp_delay.h"    // delay_us (blocking API)
#include "bsp_horn.h"     // Horn_PlayTone / Horn_stop（雷达鸣叫）
#include "bsp_timer.h"    // tickMs（非阻塞计时）

/**
 * @brief 初始化超声波模块引脚
 * @note  TRIG（P4.7）配置为推挽输出，ECHO（P3.3）配置为高阻输入。
 *        定时器3的 10us 中断需外部开启，本函数不负责定时器初始化。
 */
void Ultrasonic_Init(void)
{
    GPIO_InitTypeDef trigInitStruct = {0};
    GPIO_InitTypeDef echoInitStruct = {0};

    // TRIG: 推挽输出
    trigInitStruct.Mode = GPIO_OUT_PP;
    trigInitStruct.Pin = GPIO_Pin_7;
    GPIO_Inilize(GPIO_P4, &trigInitStruct);

    // ECHO: 高阻输入（准双向口亦可）
    echoInitStruct.Mode = GPIO_HighZ;
    echoInitStruct.Pin = GPIO_Pin_3;
    GPIO_Inilize(GPIO_P3, &echoInitStruct);
}

/**
 * @brief 阻塞式单次测距
 * @param distance 输出参数，存放测量得到的距离值（单位 cm）
 * @return char 执行状态：
 *         - ULTRASONIC_OK         (0)  成功
 *         - ULTRASONIC_ERR_NO_ECHO   未收到回波高电平（超时 5ms）
 *         - ULTRASONIC_ERR_TOO_FAR   回波宽度超时（30ms，对应约 5m）
 *         - ULTRASONIC_ERR_RANGE     结果超出有效范围（2~400cm）
 * @note  1. 发送 TRIG 高电平至少 10us（本函数保留 2 倍余量）。
 *        2. 等待 ECHO 上升沿，超时 5ms。
 *        3. 测量 ECHO 高电平宽度，最大 30ms。
 *        4. 距离 = (高电平宽度 * 声速) / 2，声速取 34 cm/ms。
 *        5. 有效距离 2~400cm，超出则返回错误。
 */
char Ultrasonic_GetDistance(float *distance)
{
    u16 cnt;

    // 1. 产生触发信号：TRIG 高电平保持 ≥10us（实际 20us），再拉低
    TRIG = 1;
    delay_us(10);
    delay_us(10);
    TRIG = 0;

    // 2. 等待 ECHO 变为高电平，超时 500*10us = 5ms
    cnt = 0;
    while (ECHO == 0 && cnt < 500)
    {
        cnt++;
        delay_us(10);
    }
    if (cnt >= 500)
        return ULTRASONIC_ERR_NO_ECHO;

    // 3. 测量 ECHO 高电平宽度，超时 3000*10us = 30ms
    cnt = 0;
    while (ECHO == 1 && cnt < 3000)
    {
        cnt++;
        delay_us(10);
    }
    if (cnt >= 3000)
        return ULTRASONIC_ERR_TOO_FAR;

    // 4. 计算距离：cnt 为 10us 个数，换算成 ms，再乘声速 34cm/ms，除以 2（往返）
    *distance = ((cnt * 0.01f) * 34.0f) / 2.0f;

    // 5. 有效距离范围 2~400cm
    if (*distance < 2.0f || *distance > 400.0f)
        return ULTRASONIC_ERR_RANGE;

    return ULTRASONIC_OK;
}

// 非阻塞状态机（定时器中断服务）

/* 状态机状态定义 */
#define US_IDLE 0      // 空闲，可启动新测量
#define US_TRIG_HOLD 1 // 正在保持 TRIG 高电平
#define US_WAIT_HIGH 2 // 等待 ECHO 上升沿
#define US_MEASURE 3   // 测量 ECHO 高电平宽度
#define US_DONE 4      // 测量完成，结果已就绪

static volatile u8 s_usState = US_IDLE; // 当前状态
static volatile u16 s_usCnt10 = 0;      // 10us 计数器
static volatile u16 s_usResultCnt = 0;  // 测量结果（高电平宽度，单位 10us）
static volatile char s_usError = 0;     // 错误码（0 表示无错误）

/**
 * @brief 非阻塞状态机服务函数，需在 10us 定时器中断中调用
 * @note  每 10us 调用一次，根据当前状态执行相应动作：
 *        - US_TRIG_HOLD：计数达到 2（20us）后拉低 TRIG，转入 WAIT_HIGH
 *        - US_WAIT_HIGH：检测 ECHO 上升沿或超时（5ms）
 *        - US_MEASURE：检测 ECHO 下降沿或超时（30ms），完成后转入 DONE
 */
void Ultrasonic_NB_Isr(void)
{
    switch (s_usState)
    {
    case US_IDLE:
        break;

    case US_TRIG_HOLD:
        if (++s_usCnt10 >= 2) // 20us 后拉低 TRIG
        {
            TRIG = 0;
            s_usCnt10 = 0;
            s_usState = US_WAIT_HIGH;
        }
        break;

    case US_WAIT_HIGH:
        if (ECHO == 1) // 检测到上升沿，开始测量
        {
            s_usCnt10 = 0;
            s_usState = US_MEASURE;
        }
        else if (++s_usCnt10 >= 500) // 5ms 内未出现上升沿，超时
        {
            s_usError = ULTRASONIC_ERR_NO_ECHO;
            s_usState = US_DONE;
        }
        break;

    case US_MEASURE:
        if (ECHO == 0) // 下降沿，测量结束
        {
            s_usResultCnt = s_usCnt10;
            s_usError = 0;
            s_usState = US_DONE;
        }
        else if (++s_usCnt10 >= 3000) // 高电平持续超过 30ms，超时
        {
            s_usError = ULTRASONIC_ERR_TOO_FAR;
            s_usState = US_DONE;
        }
        break;

    default:
        break;
    }
}

/**
 * @brief 非阻塞式获取距离（状态机驱动）
 * @param distance 输出参数，成功时存放距离值（单位 cm）
 * @return char 状态码：
 *         - ULTRASONIC_OK         测量完成，distance 有效
 *         - ULTRASONIC_BUSY       测量进行中，需继续调用
 *         - 其他错误码（NO_ECHO / TOO_FAR / RANGE）
 * @note  1. 首次调用本函数会启动一次测量（状态从 IDLE 转为 TRIG_HOLD）。
 *        2. 之后应频繁调用（如主循环），直到返回非 BUSY。
 *        3. 测量完成后自动回到 IDLE 状态，可再次触发。
 *        4. 读取结果时通过关中断保护共享变量，保证原子性。
 */
char Ultrasonic_GetDistance_NB(float *distance)
{
    if (s_usState == US_DONE) // 测量完成，取出结果
    {
        u16 cnt;
        char err;

        EA = 0; // 原子操作，保护共享变量
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

    if (s_usState == US_IDLE) // 空闲，启动新测量
    {
        EA = 0;
        s_usState = US_TRIG_HOLD;
        s_usCnt10 = 0;
        s_usError = 0;
        TRIG = 1; // 拉高 TRIG
        EA = 1;
    }

    return ULTRASONIC_BUSY;
}

// 雷达鸣叫任务（非阻塞）

#define RADAR_MAX_CM 20.0f // 触发上限 (cm)
#define RADAR_MIN_CM 2.0f  // 触发下限 (cm)
#define RADAR_BEEP_MS 100  // 鸣叫持续时间 (ms)
#define RADAR_TONE 1       // 音调编号（1~28）

static unsigned int s_radarLastMs = 0; // 上次鸣叫开始时间 (tickMs)
static unsigned char s_radarOn = 0;    // 1 表示正在鸣叫
static unsigned int s_radarEndMs = 0;  // 当前鸣叫结束时间

/**
 * @brief 根据距离计算下一次鸣叫间隔（线性映射）
 * @param dist 当前距离（cm）
 * @return unsigned int 间隔时间（ms），范围 100~3000ms
 * @note  距离 2cm 时返回 100ms，距离 20cm 时返回 3000ms，中间线性插值。
 */
static unsigned int Radar_CalcInterval(float dist)
{
    float t;
    if (dist < RADAR_MIN_CM)
        dist = RADAR_MIN_CM;
    if (dist > RADAR_MAX_CM)
        dist = RADAR_MAX_CM;
    t = (dist - RADAR_MIN_CM) / (RADAR_MAX_CM - RADAR_MIN_CM); // 归一化 0~1
    return (unsigned int)(100u + (3000u - 100u) * t);
}

/**
 * @brief 雷达鸣叫任务（非阻塞），需在主循环中约每 10ms 调用一次
 * @note  1. 每次调用尝试进行一次非阻塞测距。
 *        2. 若测距完成且距离在 2~20cm 内，且距上次鸣叫满足间隔，则触发鸣叫。
 *        3. 鸣叫间隔随距离线性变化：2cm→100ms，20cm→3000ms。
 *        4. 鸣叫持续 200ms 后自动停止，若距离超出范围则立即停止。
 */
void Ultrasonic_Radar_Task(void)
{
    float dist;
    char ret = Ultrasonic_GetDistance_NB(&dist); // 唯一调用点

    // 完成一次测量：打印结果（保留调试）并判断是否触发
    if (ret == ULTRASONIC_OK)
    {
        printf("Distance: %.2f cm\n", dist); // 打印距离

        if (dist >= RADAR_MIN_CM && dist <= RADAR_MAX_CM)
        {
            // 满足距离范围，且未在鸣叫，且到达间隔时间 → 触发鸣叫
            if (!s_radarOn && (unsigned int)(tickMs - s_radarLastMs) >= Radar_CalcInterval(dist))
            {
                Horn_PlayTone(RADAR_TONE);
                s_radarLastMs = tickMs;
                s_radarEndMs = tickMs;
                s_radarOn = 1;
            }
        }
        else if (s_radarOn) // 超出范围 → 立即停止
        {
            Horn_stop();
            s_radarOn = 0;
        }
    }

    // 鸣叫持续时间到，停止鸣叫（独立于测距结果）
    if (s_radarOn && (unsigned int)(tickMs - s_radarEndMs) >= RADAR_BEEP_MS)
    {
        Horn_stop();
        s_radarOn = 0;
    }
}