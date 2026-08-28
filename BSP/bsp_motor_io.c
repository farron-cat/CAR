/**
 * @file    bsp_motor_io.c
 * @brief   电机 I/O 控制驱动模块
 * @details 本模块负责四路小车的直流电机控制引脚初始化与运转控制：
 *          - 前进/后退使能引脚：P1 口的 P1.4 ~ P1.7、P2 口的 P2.0 ~ P2.3。
 *          - 每个电机含正转（F）与反转（B）两个引脚，通过电平组合控制转向。
 *          提供电机引脚初始化、方向设置、软件 PWM 变速（阻塞式）及停止功能。
 * @note    所有控制引脚均配置为推挽输出（GPIO_OUT_PP），由高低电平直接驱动。
 * @note    Motor_RunWithDelay() 为阻塞式软件 PWM（20ms 周期），运行期间会将
 *          CPU 占用，无法响应其它任务；如需非阻塞控制请改用硬件 PWM 或定时器方案。
 * @note    软件 PWM 通过延时实现，调速精度受 delay_ms() 分辨率限制。
 */

#include "STC8G_H_GPIO.h"
#include "STC8G_H_Delay.h"
#include "bsp_motor_io.h"

/**
 * @brief 初始化所有电机控制引脚为推挽输出
 */
void Motor_Init(void)
{
    GPIO_InitTypeDef motorInitStruct;

    // 配置 P1.4 ~ P1.7 为推挽输出（右前、左前轮）
    motorInitStruct.Mode = GPIO_OUT_PP;
    motorInitStruct.Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Inilize(GPIO_P1, &motorInitStruct);

    // 配置 P2.0 ~ P2.3 为推挽输出（右后、左后轮）
    motorInitStruct.Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_Inilize(GPIO_P2, &motorInitStruct);
}

/**
 * @brief 设置所有电机转向（正转或反转）
 * @param direction MOTOR_FORWARD 或 MOTOR_BACKWARD
 */
void Motor_SetDirection(unsigned char direction)
{
    if (direction == MOTOR_FORWARD)
    {
        // 所有轮正转：正转引脚置高，反转引脚置低
        MOTOR_RR_F = 1;
        MOTOR_RR_B = 0;
        MOTOR_RL_F = 1;
        MOTOR_RL_B = 0;
        MOTOR_FR_F = 1;
        MOTOR_FR_B = 0;
        MOTOR_FL_F = 1;
        MOTOR_FL_B = 0;
    }
    else if (direction == MOTOR_BACKWARD)
    {
        // 所有轮反转：正转引脚置低，反转引脚置高
        MOTOR_RR_F = 0;
        MOTOR_RR_B = 1;
        MOTOR_RL_F = 0;
        MOTOR_RL_B = 1;
        MOTOR_FR_F = 0;
        MOTOR_FR_B = 1;
        MOTOR_FL_F = 0;
        MOTOR_FL_B = 1;
    }
}

/**
 * @brief 使用延时软件 PWM 控制电机速度（阻塞式）
 * @param direction 方向：MOTOR_FORWARD / MOTOR_BACKWARD
 * @param speed     速度等级：0~100（0停止，100全速）
 * @param duration_ms 持续运行的总时间（单位 ms），达到后停止
 * @note 此函数会阻塞执行，期间无法响应其他任务
 */
void Motor_RunWithDelay(unsigned char direction, unsigned char speed, unsigned int duration_ms)
{
    unsigned int cycle_ms = 20; // 基本周期 20ms（50Hz，人耳无噪音）
    unsigned int on_time = (unsigned long)speed * cycle_ms / 100;
    unsigned int off_time = cycle_ms - on_time;
    unsigned int elapsed = 0;

    // 如果速度为0，直接停止并返回
    if (speed == 0)
    {
        Motor_Stop(); // 需要实现停止函数（所有引脚置低）
        return;
    }

    while (elapsed < duration_ms)
    {
        // 正转（或反转）阶段
        Motor_SetDirection(direction); // 全速运行
        delay_ms(on_time);
        elapsed += on_time;

        if (elapsed >= duration_ms)
            break;

        // 停止阶段（滑行：所有引脚置低）
        Motor_Stop();
        delay_ms(off_time);
        elapsed += off_time;
    }

    // 最后停止电机
    Motor_Stop();
}

/**
 * @brief 停止所有电机
 * @note  将所有电机控制引脚（正转与反转）全部置低，使电机处于释放/停止状态。
 * @note  该函数为 Motor_RunWithDelay() 的辅助停止函数，也可单独调用。
 */
void Motor_Stop(void)
{
    MOTOR_RR_F = 0;
    MOTOR_RR_B = 0;
    MOTOR_RL_F = 0;
    MOTOR_RL_B = 0;
    MOTOR_FR_F = 0;
    MOTOR_FR_B = 0;
    MOTOR_FL_F = 0;
    MOTOR_FL_B = 0;
}