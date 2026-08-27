/**
 * @file    bsp_key.c
 * @brief   按键驱动模块
 * @details 本模块负责核心板及其扩展板上独立按键的初始化与扫描：
 *          - KEY    ：扩展板独立按键，接 P0.5，按下为低电平。
 *          - KEY_C  ：核心板独立按键，接 P3.2，按下为低电平。
 *          提供两种按键事件扫描接口（Key_Scan / KeyC_Scan），
 *          内部采用四态防抖状态机，可区分短按与长按事件。
 * @note    按键引脚均配置为准双向口（带内部上拉），松开时为高电平。
 * @note    扫描函数依赖 bsp_timer 提供的 tickMs 毫秒计数（由 Timer0 1ms 中断递增），
 *          调用前需先调用 Timer0Init1ms() 并开启全局中断。
 * @note    扫描函数需周期性调用（建议每 10~20ms 调用一次）。
 */

#include "STC8G_H_GPIO.h"
#include "STC8G_H_Exti.h"
#include "STC8G_H_NVIC.h"
#include "STC8G_H_Delay.h"

#include "bsp_key.h"
#include "bsp_timer.h"

/**
 * @brief 初始化独立按键 KEY_1 KEY_2 KEY_3 KEY_4
 * @note 配置P5.1、P5.2、P5.4、P5.5为上拉准双向口
 * @note 按键按下时引脚为低电平，松开时为高电平
 */
void KeyInit()
{
    // 配置P0.5准上拉准双向口
    GPIO_InitTypeDef keyInitStruct;
    keyInitStruct.Mode = GPIO_PullUp;
    keyInitStruct.Pin = GPIO_Pin_5;

    GPIO_Inilize(GPIO_P0, &keyInitStruct);

    // 初始化核心板独立按键 KEY_C
    KeyCInit();
}

/**
 * @brief 初始化核心板独立按键 KEY_C
 * @note 配置P3.2为上拉准双向口，按键按下时引脚被拉低
 */
void KeyCInit()
{
    // 配置 P3.2 为上拉准双向口
    GPIO_InitTypeDef keycInitStruct;
    keycInitStruct.Mode = GPIO_PullUp;
    keycInitStruct.Pin = GPIO_Pin_2;

    GPIO_Inilize(GPIO_P3, &keycInitStruct);
}

/**
 * @brief 初始化核心板独立按键 KEY_C 与 INT0 中断
 * @note 按键KEY_C与INT0中断引脚相连
 * @note 配置INT0为下降沿触发，使能INT0中断（优先级0）
 * @note 按键按下时产生下降沿，触发INT0中断服务函数
 */
void KeyCINTInit()
{
    // 按键KEY_C与INT0中断引脚相连

    // 配置外部中断INT0
    EXTI_InitTypeDef EXTI_KEY_P32;          // 中断模式配置结构体
    EXTI_KEY_P32.EXTI_Mode = EXT_MODE_Fall; // 下降沿触发

    Ext_Inilize(EXT_INT0, &EXTI_KEY_P32); // INT0中断初始化
    NVIC_INT0_Init(ENABLE, Priority_0);   // INT0嵌套向量中断控制器初始化

    // 配置 P3.2 为准双向口
    KeyCInit();
}

// /**
//  * @brief INT0中断服务函数（按键KEY_C中断处理）
//  * @note 中断触发后先延时20ms消抖，确认按键仍处于按下状态
//  * @note 按键确认按下后：LEDC翻转、电机翻转
//  */
// void INT0_ISR_Handler(void) interrupt INT0_VECTOR
// {
//     delay_ms(20); // 消抖延时
//     if (KEY_C == 0)
//     {
//         // 确认仍然按下
//         LEDCToggle();   // LEDC翻转
//         MOTOR = ~MOTOR; // 电机翻转
//     }
// }

/**
 * @brief 扫描核心板按键 KEY_C，返回按键事件
 * @return KeyEvent 返回值：
 *         - KEY_RELEASE   : 无事件（按键未按下或已释放）
 *         - KEY_SHORT_PRESS : 检测到短按（按下持续时间 < 1000ms）
 *         - KEY_LONG_PRESS  : 检测到长按（按下持续时间 >= 1000ms）
 * @note  需周期性调用（建议每 10~20ms 调用一次），内部用静态变量实现防抖状态机：
 *         - 状态0/1：按下消抖，电平连续稳定 DEBOUNCE_MS 才确认按下
 *         - 状态2：确认按下，累计 LONG_PRESS_MS 判长按，松手进入释放去抖
 *         - 状态3：释放消抖，电平连续稳定 DEBOUNCE_MS 才确认释放，未发过长按则判短按
 * @note  每次完整点击（按下+稳定+释放）至多产生一次短按或一次长按，不会重复触发
 * @note  长按阈值、消抖时间可在下方宏中调整（单位为毫秒）
 */
#define LONG_PRESS_MS 1000 // 长按阈值（毫秒）
#define DEBOUNCE_MS 20     // 消抖时间（毫秒）

KeyEvent KeyC_Scan(void)
{
    // 状态机状态：0=空闲，1=按下去抖中，2=确认按下，3=释放去抖中
    static unsigned char state = 0;
    // 计时起点（消抖或长按计时的起始毫秒值）
    static unsigned int press_start_ms = 0;
    // 长按事件是否已发送（防止重复触发）
    static unsigned char event_sent = 0;

    unsigned char current_level = KEY_C; // 读取当前引脚电平（0=按下，1=松开）
    KeyEvent ret = KEY_RELEASE;

    switch (state)
    {
    case 0: // 空闲：检测到按下候选，进入按下去抖
        if (current_level == 0)
        {
            press_start_ms = tickMs; // 记录消抖起始时刻
            state = 1;
        }
        break;

    case 1: // 按下去抖中：电平连续稳定 DEBOUNCE_MS 才确认按下
        if (current_level == 0)
        {
            if ((unsigned int)(tickMs - press_start_ms) >= DEBOUNCE_MS)
            {
                state = 2;               // 确认按下
                event_sent = 0;          // 重置事件标志
                press_start_ms = tickMs; // 重新计时，作为长按计时起点
            }
        }
        else
        {
            state = 0; // 抖动弹回，废弃该次按下
        }
        break;

    case 2:                     // 确认按下：判断长按，或进入释放去抖
        if (current_level == 1) // 检测到松开，进入释放去抖
        {
            press_start_ms = tickMs;
            state = 3;
        }
        else if (!event_sent && (unsigned int)(tickMs - press_start_ms) >= LONG_PRESS_MS)
        {
            ret = KEY_LONG_PRESS; // 产生长按事件
            event_sent = 1;       // 防止重复触发
        }
        break;

    case 3: // 释放去抖：电平稳定 DEBOUNCE_MS 才确认释放
        if (current_level == 1)
        {
            if ((unsigned int)(tickMs - press_start_ms) >= DEBOUNCE_MS)
            {
                // 确认释放：未发过长按则判定为短按
                if (!event_sent)
                {
                    ret = KEY_SHORT_PRESS; // 产生短按事件
                }
                event_sent = 0;
                state = 0; // 回到空闲
            }
        }
        else // 释放抖动按回，回到确认按下并重新计时长按
        {
            press_start_ms = tickMs;
            state = 2;
        }
        break;
    }

    return ret;
}

/**
 * @brief 扫描独立按键 KEY（P0.5），返回按键事件
 * @return KeyEvent 返回值：
 *         - KEY_RELEASE   : 无事件（按键未按下或已释放）
 *         - KEY_SHORT_PRESS : 检测到短按（按下持续时间 < 1000ms）
 *         - KEY_LONG_PRESS  : 检测到长按（按下持续时间 >= 1000ms）
 * @note  需周期性调用（建议每 10~20ms 调用一次），内部用静态变量实现防抖状态机：
 *         - 状态0/1：按下消抖，电平连续稳定 DEBOUNCE_MS 才确认按下
 *         - 状态2：确认按下，累计 LONG_PRESS_MS 判长按，松手进入释放去抖
 *         - 状态3：释放消抖，电平连续稳定 DEBOUNCE_MS 才确认释放，未发过长按则判短按
 * @note  每次完整点击（按下+稳定+释放）至多产生一次短按或一次长按，不会重复触发
 * @note  与 KeyC_Scan 逻辑一致，仅扫描 KEY 引脚（KEY = P0.5）
 */
KeyEvent Key_Scan(void)
{
    // 状态机状态：0=空闲，1=按下去抖中，2=确认按下，3=释放去抖中
    static unsigned char state = 0;
    // 计时起点（消抖或长按计时的起始毫秒值）
    static unsigned int press_start_ms = 0;
    // 长按事件是否已发送（防止重复触发）
    static unsigned char event_sent = 0;

    unsigned char current_level = KEY; // 读取当前引脚电平（0=按下，1=松开）
    KeyEvent ret = KEY_RELEASE;

    switch (state)
    {
    case 0: // 空闲：检测到按下候选，进入按下去抖
        if (current_level == 0)
        {
            press_start_ms = tickMs; // 记录消抖起始时刻
            state = 1;
        }
        break;

    case 1: // 按下去抖中：电平连续稳定 DEBOUNCE_MS 才确认按下
        if (current_level == 0)
        {
            if ((unsigned int)(tickMs - press_start_ms) >= DEBOUNCE_MS)
            {
                state = 2;               // 确认按下
                event_sent = 0;          // 重置事件标志
                press_start_ms = tickMs; // 重新计时，作为长按计时起点
            }
        }
        else
        {
            state = 0; // 抖动弹回，废弃该次按下
        }
        break;

    case 2:                     // 确认按下：判断长按，或进入释放去抖
        if (current_level == 1) // 检测到松开，进入释放去抖
        {
            press_start_ms = tickMs;
            state = 3;
        }
        else if (!event_sent && (unsigned int)(tickMs - press_start_ms) >= LONG_PRESS_MS)
        {
            ret = KEY_LONG_PRESS; // 产生长按事件
            event_sent = 1;       // 防止重复触发
        }
        break;

    case 3: // 释放去抖：电平稳定 DEBOUNCE_MS 才确认释放
        if (current_level == 1)
        {
            if ((unsigned int)(tickMs - press_start_ms) >= DEBOUNCE_MS)
            {
                // 确认释放：未发过长按则判定为短按
                if (!event_sent)
                {
                    ret = KEY_SHORT_PRESS; // 产生短按事件
                }
                event_sent = 0;
                state = 0; // 回到空闲
            }
        }
        else // 释放抖动按回，回到确认按下并重新计时长按
        {
            press_start_ms = tickMs;
            state = 2;
        }
        break;
    }

    return ret;
}