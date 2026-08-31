/**
 * @file    bsp_tracker.h
 * @brief   五路循迹传感器模块（含PID控制器）
 * @version 2.0
 * @date    2026-08-31
 * @details 本模块驱动5路红外循迹传感器（P0.0~P0.4），
 *          提供原始位置检测（加权平均）和PID控制量计算。
 *          位置偏差范围为 -64 ~ +64（0表示中线），
 *          PID输出控制量可用于转向舵机或差速电机。
 * @note    传感器输出为数字电平：0=黑线（有效），1=白底。
 *          使用前需调用 Tracker_Init() 初始化GPIO。
 */

#ifndef BSP_TRACKER_H
#define BSP_TRACKER_H

#include "Config.h"

/* ==================== 引脚定义 ==================== */
#define TRACK_0 P00 // 最左
#define TRACK_1 P01 // 左
#define TRACK_2 P02 // 中
#define TRACK_3 P03 // 右
#define TRACK_4 P04 // 最右

/* ==================== PID 参数结构体 ==================== */
typedef struct
{
    float Kp;          // 比例系数
    float Ki;          // 积分系数
    float Kd;          // 微分系数
    float Integral;    // 积分累加值
    float LastError;   // 上次偏差（用于微分）
    float MaxIntegral; // 积分限幅（防止积分饱和）
    float MaxOutput;   // 输出限幅（防止过冲）
} PID_TypeDef;

/* ==================== 函数声明 ==================== */

/**
 * @brief 初始化循迹传感器GPIO（P0.0~P0.4为准双向口）
 */
void Tracker_Init(void);

/**
 * @brief 扫描所有传感器，将电平状态存入数组
 * @param states 长度为5的u8数组，用于存放每个传感器的逻辑电平
 *               （0=黑线，1=白底）
 */
void Tracker_Scan(u8 states[]);

/**
 * @brief 获取当前黑线位置偏差（加权平均法）
 * @return int 位置偏差值，范围 -64 ~ +64，0表示正中；
 *             返回 999 表示未检测到黑线（全部为白）
 * @note  权重：左起 -64, -32, 0, +32, +64
 */
int Tracker_Get_Position(void);

/**
 * @brief 初始化PID控制器参数
 * @param pid      PID结构体指针
 * @param kp       比例系数
 * @param ki       积分系数
 * @param kd       微分系数
 * @param maxInt   积分限幅（绝对值）
 * @param maxOut   输出限幅（绝对值）
 */
void PID_Init(PID_TypeDef *pid,
              float kp, float ki, float kd,
              float maxInt, float maxOut);

/**
 * @brief 执行PID计算（位置式）
 * @param pid   PID结构体指针
 * @param error 当前偏差（目标值 - 测量值），目标值通常为0
 * @return float 控制量（已限幅）
 * @note  每周期调用一次，内部自动更新积分和微分项。
 *        积分项在误差方向改变时自动清零（防止过冲）。
 */
float PID_Calc(PID_TypeDef *pid, float error);

/**
 * @brief 重置PID积分项和上次误差（用于急停或模式切换）
 * @param pid PID结构体指针
 */
void PID_Reset(PID_TypeDef *pid);

#endif // BSP_TRACKER_H