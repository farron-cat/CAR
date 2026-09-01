/**
 * @file    bsp_motor_driver.h
 * @brief   四轮直流电机驱动模块（PWM硬件控制）头文件
 * @details 对外提供速度结构体、初始化及运动控制接口
 * @note    使用前必须调用 Motors_Init() 完成硬件初始化
 */

#ifndef __BSP_MOTOR_DRIVER_H__
#define __BSP_MOTOR_DRIVER_H__

#include "STC8H_PWM.h"

/**
 * @brief 四轮速度配置结构体
 * @note  速度范围：-100 ~ 100
 *        - 正数：前进方向（对应PWM低电平占空比增大）
 *        - 负数：后退方向
 *        - 0：停止该轮输出
 */
typedef struct
{
    int RR_speed; // 右后轮 (PWM1)
    int RL_speed; // 左后轮 (PWM2)
    int FR_speed; // 右前轮 (PWM3)
    int FL_speed; // 左前轮 (PWM4)
} Motor_Config_t;

/**
 * @brief 电机PWM硬件初始化（必须在上电时调用一次）
 * @note  配置周期1kHz、死区时间、引脚映射，运行时绝对不要重复调用
 */
void Motors_Init(void);

/**
 * @brief 运行时更新四轮速度（核心函数，轻量级，仅操作CCR和ENO寄存器）
 * @param cfg 四轮速度配置
 */
void Motors_Update(Motor_Config_t cfg);

/**
 * @brief 小车全速前进
 * @param speed 速度 0~100
 */
void Motors_Forward(int speed);

/**
 * @brief 小车全速后退
 * @param speed 速度 0~100
 */
void Motors_Backward(int speed);

/**
 * @brief 小车左平移（横向移动）
 * @param speed 速度 0~100
 * @param dir   模式选择：1(右侧外翻) / -1(左侧内翻) / 0(组合效果)
 */
void Motors_Left(int speed, int dir);

/**
 * @brief 小车右平移（横向移动）
 * @param speed 速度 0~100
 * @param dir   模式选择：1(右侧内翻) / -1(左侧外翻) / 0(组合效果)
 */
void Motors_Right(int speed, int dir);

/**
 * @brief 小车原地旋转（打转）
 * @param speed 速度 0~100
 * @param dir   方向：0(逆时针) / 非0(顺时针)
 */
void Motors_Around(int speed, int dir);

/**
 * @brief 立即停止所有电机（滑行停止）
 */
void Motors_Stop(void);

#endif