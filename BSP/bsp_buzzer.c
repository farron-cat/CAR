#include "STC8H.H"
#include "bsp_buzzer.h"
#include "bsp_delay.h"
#include "bsp_timer.h"
#include "STC8G_H_Timer.h"
#include "STC8G_H_NVIC.h"

void BuzzerInit()
{
    // 配置 P0.0 为推挽输出模式（M1=0, M0=1）
    P0M1 &= ~(1 << 0); // P0M1.0 = 0
    P0M0 |= (1 << 0);  // P0M0.0 = 1

    // TODO:会导致数码管不显示
    // // Timer2 一次性初始化（蜂鸣器方波发生器专用）
    // Timer2_CLK_Select(1); // Timer2 使用 1T 时钟（与半周期重载计算一致）
    // Timer2_CLK_Output(0); // 禁止 T2 时钟输出
    // Timer2_Interrupt(0);  // 先关中断
    // Timer2_Run(0);        // 先停止计数
}

/*====================================================================
 * 非阻塞播放驱动（Timer2 中断产生方波 + tickMs 状态机调度）
 *====================================================================*/

// 音符表：{半周期(us), 发声时长(ms)}
// 半周期来自原阻塞实现中的延时值：Do=956, Re=852, Mi=759, Fa=716, So=638, La=568, Si=506
typedef struct
{
    unsigned int halfPeriod; // 半周期微秒数
    unsigned char durMs;     // 音符发声时长（毫秒）
} Buzzer_Note_t;

// 《小星星》C大调：Do Do So So La La So | Fa Fa Mi Mi Re Re(延长)
// 半周期(us)：Do=956 So=638 La=568 Fa=716 Mi=759 Re=852
static const unsigned char code starHalf[14] = {
    956, 956, 638, 638, 568, 568, 638, // 一闪一闪亮晶晶
    716, 716, 759, 759, 852, 852, 956  // 满天都是小星星
};

#define STAR_NOTE_CNT 14      // 音符个数
#define STAR_NOTE_MS 400      // 每个音符发声时长
#define STAR_NOTE_GAP_MS 50   // 音符间停顿
#define STAR_LOOP_GAP_MS 1000 // 播完一轮后停顿
#define STAR_DO_HALF 956      // C5 Do 半周期（用于换算无需，此处保留示意）

static unsigned char starState = 0; // 0=空闲 1=发声 2=音间停顿 3=轮间停顿
static unsigned char starIdx = 0;   // 当前音符索引
static unsigned int starLastMs = 0; // 上次状态切换时间

// 以指定半周期(us)开始发声：配置Timer2为1T、16位自动重载，中断翻转方波
void Buzzer_ToneStart(unsigned int halfPeriodUs)
{
    unsigned long ticks;

    // 1T时钟：tick 数 = 半周期(us) × (MAIN_Fosc/1000000)
    ticks = (unsigned long)halfPeriodUs * (MAIN_Fosc / 1000000UL);

    // 使能Timer2中断 + 启动Timer2
    Timer2_Interrupt(1);
    Timer2_Run(0);
    T2_Load(65536UL - ticks); // 装载重载值（半周期）
    Timer2_Run(1);

    BUZZER = 1; // 起始输出高，中断翻转产生方波
}

// 停止发声
void Buzzer_ToneOff(void)
{
    Timer2_Interrupt(0); // 关中断
    Timer2_Run(0);       // 停定时器
    BUZZER = 0;          // 静音
}

// Timer2中断服务：翻转蜂鸣器引脚（由ISR_Handler调用）
void Buzzer_ToneIsr(void)
{
    BUZZER = ~BUZZER;
}

// 开始播放小星星（非阻塞）
void LittleStar_Start(void)
{
    Buzzer_ToneStart(starHalf[0]); // 播放第一个音符
    starIdx = 0;
    starState = 1; // 发声
    starLastMs = tickMs;
}

// 每主循环调用一次推进播放
void LittleStar_Task(void)
{
    if (starState == 0)
        return;

    switch (starState)
    {
    case 1: // 发声：持续 STAR_NOTE_MS 后进入音间停顿
        if ((unsigned int)(tickMs - starLastMs) >= STAR_NOTE_MS)
        {
            Buzzer_ToneOff();
            starState = 2;
            starLastMs = tickMs;
        }
        break;

    case 2: // 音间停顿：到期后播下一音或轮间停顿
        if ((unsigned int)(tickMs - starLastMs) >= STAR_NOTE_GAP_MS)
        {
            starIdx++;
            if (starIdx >= STAR_NOTE_CNT)
            {
                starState = 3; // 整曲结束，进入轮间停顿
                starLastMs = tickMs;
            }
            else
            {
                Buzzer_ToneStart(starHalf[starIdx]);
                starState = 1;
                starLastMs = tickMs;
            }
        }
        break;

    case 3: // 轮间停顿：暂停发射停2曲后从头播放
        if ((unsigned int)(tickMs - starLastMs) >= STAR_LOOP_GAP_MS)
        {
            starIdx = 0;
            Buzzer_ToneStart(starHalf[0]);
            starState = 1;
            starLastMs = tickMs;
        }
        break;

    default:
        starState = 0;
        break;
    }
}

// 返回1表示正在播放，0表示空闲
unsigned char LittleStar_Playing(void)
{
    return (starState != 0);
}

//====================================================================
// 下列为基础阻塞式音符播放
//====================================================================

/**
 * Do - 播放 Do 音符
 * @note 蜂鸣器发声约 400ms（1拍）
 */
void Do(void)
{
    unsigned int i;
    for (i = 0; i < 209; i++) // 400ms ÷ (956us×2) ≈ 209
    {
        BUZZER = 1;
        Delay956us();
        BUZZER = 0;
        Delay956us();
    }
}

void Re(void)
{
    unsigned int i;
    for (i = 0; i < 235; i++)
    {
        BUZZER = 1;
        Delay852us();
        BUZZER = 0;
        Delay852us();
    }
}

void Mi(void)
{
    unsigned int i;
    for (i = 0; i < 263; i++)
    {
        BUZZER = 1;
        Delay759us();
        BUZZER = 0;
        Delay759us();
    }
}

void Fa(void)
{
    unsigned int i;
    for (i = 0; i < 279; i++) // 400ms ÷ (716us×2) ≈ 279
    {
        BUZZER = 1;
        Delay716us();
        BUZZER = 0;
        Delay716us();
    }
}

void So(void)
{
    unsigned int i;
    for (i = 0; i < 313; i++)
    {
        BUZZER = 1;
        Delay638us();
        BUZZER = 0;
        Delay638us();
    }
}

void La(void)
{
    unsigned int i;
    for (i = 0; i < 352; i++)
    {
        BUZZER = 1;
        Delay568us();
        BUZZER = 0;
        Delay568us();
    }
}

void Si(void)
{
    unsigned int i;
    for (i = 0; i < 395; i++)
    {
        BUZZER = 1;
        Delay506us();
        BUZZER = 0;
        Delay506us();
    }
}

void Buzzer_Off(void)
{
    BUZZER = 0;
}

// 原有阻塞式捂长版本（保留兼容，不推荐新代码使用）
void LittleStar()
{
    //---------------- 第1段----------------//
    Do();
    Delay50ms();
    Do();
    Delay50ms();
    So();
    Delay50ms();
    So();
    Delay50ms();

    La();
    Delay50ms();
    La();
    Delay50ms();
    So();
    So();
    Delay50ms();

    //---------------- 第2段----------------//
    Fa();
    Delay50ms();
    Fa();
    Delay50ms();
    Mi();
    Delay50ms();
    Mi();
    Delay50ms();

    Re();
    Delay50ms();
    Re();
    Delay50ms();
    Do();
    Do();
    Delay50ms();

    Buzzer_Off(); // 关闭蜂鸣器

    // 播完等1秒再重复
    Delay1000ms();
}