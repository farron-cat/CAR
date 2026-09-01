/**
 * @file    bsp_ultrasonic.c
 * @brief   超声波测距驱动模块（INT1 外部中断 + Timer3 计时）
 * @details 本模块基于硬件中断与定时器实现测距：
 *          - TRIG（P4.7）推挽输出，产生 ≥10us 触发脉冲。
 *          - ECHO（P3.3）接 INT1，上升沿触发外部中断。
 *          - 上升沿中断中清零并启动 Timer3（12T，0.5us/计数）开始计时；
 *            下降沿中断中读取 Timer3 计数值得到回波高电平宽度，
 *            由脉冲宽度换算距离（HC-SR04：58us 对应 1cm）。
 *          - 提供阻塞式 Ultrasonic_GetDistance() 与非阻塞式
 *            Ultrasonic_GetDistance_NB() 两种 API，另附雷达鸣叫任务。
 * @note    系统时钟 24MHz；Timer3 配置为 12T 自由计数，16 位最大
 *          量程 65536*0.5us ≈ 32.7ms（对应约 5.6m），覆盖 HC-SR04。
 * @note    回波计时由硬件完成（Timer3 自由计数），INT1 中断只在边沿
 *          时刻启动/停止计时器并读数，测量精度不受主循环影响。
 */

#include "bsp_ultrasonic.h"
#include "STC8G_H_GPIO.h"   // GPIO_Inilize / GPIO_InitTypeDef
#include "STC8G_H_Exti.h"   // Ext_Inilize / EXT_INT1 / EXT_MODE_RiseFall
#include "STC8G_H_Timer.h"  // Timer_Inilize / Timer3_Run / Timer3_Stop
#include "STC8G_H_NVIC.h"   // NVIC_INT1_Init / NVIC_Timer3_Init
#include "bsp_delay.h"      // delay_us（产生触发脉冲）
#include "bsp_horn.h"       // Horn_PlayTone / Horn_stop（雷达鸣叫）
#include "bsp_timer.h"      // tickMs（非阻塞超时计时）

/* 测距状态机状态定义 */
#define US_IDLE      0 // 空闲，可启动新测量
#define US_WAIT_ECHO 1 // 已发触发，等待 ECHO 上升沿
#define US_WAIT_FALL 2 // 上升沿已到，Timer3 计时中，等待下降沿
#define US_DONE      3 // 测量完成，结果已就绪

#define US_TIMEOUT_MS 60 // 单次测量超时（ms），覆盖无回波/超量程

static volatile u8 s_usState = US_IDLE;       // 当前状态
static volatile u16 s_usElapsed = 0;          // 回波高电平宽度（Timer3 计数值，0.5us/计数）
static volatile unsigned int s_usStartMs = 0; // 本次测量开始时间（tickMs，超时判断用）

/**
 * @brief INT1 外部中断服务函数（ECHO 上升沿 / 下降沿）
 * @note  上升沿（ECHO==1）：清零并启动 Timer3，开始计时。
 *        下降沿（ECHO==0）：停止 Timer3 并读取计数值作为回波宽度。
 *        仅在测量状态（非 US_IDLE）下响应，避免空闲期的干扰边沿。
 */
void Ultrasonic_INT1_Isr(void) interrupt INT1_VECTOR
{
    IE1 = 0; // 清除 INT1 中断标志

    if (s_usState == US_WAIT_ECHO && ECHO == 1) // 上升沿：开始计时
    {
        Timer3_Stop();
        T3H = 0;
        T3L = 0;
        Timer3_Run(ENABLE);
        s_usState = US_WAIT_FALL;
    }
    else if (s_usState == US_WAIT_FALL && ECHO == 0) // 下降沿：停止计时并取结果
    {
        Timer3_Stop();
        s_usElapsed = (u16)(((u16)T3H << 8) | T3L);
        s_usState = US_DONE;
    }
}

/**
 * @brief 初始化超声波模块（GPIO + INT1 + Timer3）
 * @note  TRIG（P4.7）配置为推挽输出，ECHO（P3.3）配置为高阻输入并作为 INT1。
 *        Timer3 配置为 12T 自由计数（0.5us/计数），仅作计时器不产生中断。
 *        INT1 配置为边沿触发（IT1=0，上升/下降沿均触发），优先级 Priority_1。
 */
void Ultrasonic_Init(void)
{
    GPIO_InitTypeDef trigInitStruct = {0};
    GPIO_InitTypeDef echoInitStruct = {0};
    EXTI_InitTypeDef extiInitStruct = {0};
    TIM_InitTypeDef timInitStruct = {0};

    // 1. TRIG: 推挽输出
    trigInitStruct.Mode = GPIO_OUT_PP;
    trigInitStruct.Pin = GPIO_Pin_7;
    GPIO_Inilize(GPIO_P4, &trigInitStruct);
    TRIG = 0;

    // 2. ECHO: 高阻输入（准双向口亦可）
    echoInitStruct.Mode = GPIO_HighZ;
    echoInitStruct.Pin = GPIO_Pin_3;
    GPIO_Inilize(GPIO_P3, &echoInitStruct);

    // 3. Timer3: 16 位自动重载（Timer3 固定模式），12T 时钟，0.5us/计数
    timInitStruct.TIM_Mode = TIM_16BitAutoReload;
    timInitStruct.TIM_ClkSource = TIM_CLOCK_12T;
    timInitStruct.TIM_ClkOut = DISABLE;
    timInitStruct.TIM_Value = 0;
    timInitStruct.TIM_PS = 0;
    timInitStruct.TIM_Run = ENABLE;
    Timer_Inilize(Timer3, &timInitStruct);
    NVIC_Timer3_Init(DISABLE, Priority_0); // 禁用 Timer3 中断，仅作自由计数器

    // 4. INT1: 边沿触发（IT1=0，上升/下降沿），使能中断，优先级高于 Timer0/UART2
    extiInitStruct.EXTI_Mode = EXT_MODE_RiseFall;
    Ext_Inilize(EXT_INT1, &extiInitStruct);
    NVIC_INT1_Init(ENABLE, Priority_1);
    IE1 = 0;
}

/**
 * @brief 启动一次非阻塞测量
 * @note  产生 ≥10us 的 TRIG 高电平脉冲（阻塞约 20us，可忽略），
 *        记录开始时间并进入等待回波状态。
 */
static void Ultrasonic_StartMeasurement(void)
{
    TRIG = 1;
    delay_us(10);
    delay_us(10);
    TRIG = 0;

    s_usStartMs = tickMs;
    s_usState = US_WAIT_ECHO;
}

/**
 * @brief 阻塞式单次测距
 * @param distance 输出参数，存放测量得到的距离值（单位 cm）
 * @return char 执行状态码：同非阻塞接口
 * @note  基于 Ultrasonic_GetDistance_NB() 忙等实现，内部有 60ms 超时，
 *        不会无限阻塞。
 */
char Ultrasonic_GetDistance(float *distance)
{
    char ret;

    do
    {
        ret = Ultrasonic_GetDistance_NB(distance);
    } while (ret == ULTRASONIC_BUSY);

    return ret;
}

/**
 * @brief 非阻塞式获取距离（INT1 中断 + Timer3 计时）
 * @param distance 输出参数，成功时存放距离值（单位 cm）
 * @return char 状态码：
 *         - ULTRASONIC_OK         测量完成，distance 有效
 *         - ULTRASONIC_BUSY       测量进行中，需继续调用
 *         - 其他错误码（NO_ECHO / TOO_FAR / RANGE）
 * @note  1. 首次调用本函数会启动一次测量（空闲时产生 TRIG 触发脉冲）。
 *        2. 之后应频繁调用（如主循环），直到返回非 BUSY。
 *        3. 测量完成（DONE）后自动回到 IDLE 状态，可再次触发。
 *        4. ECHO 上升沿/下降沿在 INT1 中断中处理，Timer3 硬件计时，
 *           测量精度不依赖主循环调用频率。
 */
char Ultrasonic_GetDistance_NB(float *distance)
{
    if (s_usState == US_DONE) // 测量完成，取出结果
    {
        u16 elapsed;

        EA = 0; // 原子操作，保护共享变量
        elapsed = s_usElapsed;
        s_usElapsed = 0;
        s_usState = US_IDLE;
        EA = 1;

        // 距离 = 计数值 * 0.5us/计数 / 58us/cm（HC-SR04 标准换算）
        *distance = ((float)elapsed * 0.5f) / 58.0f;

        if (*distance < 2.0f || *distance > 400.0f)
            return ULTRASONIC_ERR_RANGE;
        return ULTRASONIC_OK;
    }

    if (s_usState == US_IDLE) // 空闲，启动新测量
    {
        Ultrasonic_StartMeasurement();
        return ULTRASONIC_BUSY;
    }

    // 测量进行中：超时保护（无回波 → NO_ECHO；回波过长 → TOO_FAR）
    if ((unsigned int)(tickMs - s_usStartMs) > US_TIMEOUT_MS)
    {
        u8 timedOutState;

        EA = 0;
        timedOutState = s_usState;
        s_usState = US_IDLE;
        s_usElapsed = 0;
        EA = 1;

        Timer3_Stop();
        IE1 = 0;

        return (timedOutState == US_WAIT_FALL) ? ULTRASONIC_ERR_TOO_FAR : ULTRASONIC_ERR_NO_ECHO;
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
 *        4. 鸣叫持续 100ms 后自动停止，若距离超出范围则立即停止。
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
