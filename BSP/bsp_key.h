/**
 * @file    bsp_key.h
 * @brief   按键驱动模块接口
 * @details 提供核心板及其扩展板独立按键的定义与扫描接口：
 *          - KEY   ：扩展板独立按键，接 P0.5，按下为低电平。
 *          - KEY_C ：核心板独立按键，接 P3.2，按下为低电平。
 *          - KeyEvent 枚举：定义无事件/短按/长按三种按键事件。
 *          提供引脚初始化（含 INT0 中断方式）与按键扫描接口。
 * @note    扫描函数需周期性调用（建议每 10~20ms 一次），且依赖 bsp_timer 的 tickMs 计数。
 */
#ifndef BSP_KEY_H
#define BSP_KEY_H

#define KEY P05    // 独立按键 KEY（P0.5，准双向口，按下为低电平）

#define KEY_C P32  // 核心板独立按键 KEY_C（P3.2，准双向口，按下为低电平）

// 按键事件类型
typedef enum
{
    KEY_RELEASE = 0, // 无事件（按键松开或未操作）
    KEY_SHORT_PRESS, // 短按事件（按下时间 < 1000ms）
    KEY_LONG_PRESS   // 长按事件（按下时间 >= 1000ms）
} KeyEvent;

/**
 * @brief 初始化独立按键引脚（KEY 与 KEY_C）
 * @note 配置 P0.5（KEY）和 P3.2（KEY_C）为准双向口，按下为低电平
 */
void KeyInit();

/**
 * @brief 初始化核心板独立按键 KEY_C
 * @note 配置 P3.2 为准双向口，按键按下时引脚被拉低
 */
void KeyCInit();

/**
 * @brief 初始化核心板独立按键 KEY_C 与 INT0 外部中断
 * @note 配置 INT0 为下降沿触发并使能 INT0 中断（优先级0），供中断方式使用
 */
void KeyCINTInit();

/**
 * @brief 扫描独立按键 KEY（P0.5），返回按键事件
 * @return KeyEvent：KEY_RELEASE / KEY_SHORT_PRESS / KEY_LONG_PRESS
 * @note 需周期性调用（建议每 10~20ms 一次），内部为防抖状态机
 */
KeyEvent Key_Scan(void);

/**
 * @brief 扫描核心板按键 KEY_C（P3.2），返回按键事件
 * @return KeyEvent：KEY_RELEASE / KEY_SHORT_PRESS / KEY_LONG_PRESS
 * @note 需周期性调用（建议每 10~20ms 一次），内部为防抖状态机
 */
KeyEvent KeyC_Scan(void);

#endif // BSP_KEY_H