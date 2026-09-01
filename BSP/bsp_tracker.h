/**
 * @file    bsp_tracker.h
 * @brief   五路循迹传感器模块（含PID控制器与循迹任务）
 * @details 本模块驱动5路红外循迹传感器（P0.0~P0.4），提供：
 *          - 位置检测：加权平均（-64 ~ +64，0表示中线）。
 *          - PID控制器：位置式，带积分限幅与微分先行。
 *          - 循迹任务 Tracker_Update()：丢线记忆外推 + 原地搜索、
 *            PID输出 → 左右轮差速 → MotorDirver_PWM_Config 驱动电机。
 *          循迹模式下偏差 >0 表示线偏右，向右转向；偏差 <0 线偏左，向左转向。
 * @note    传感器输出为数字电平：0=黑线（有效），1=白底。
 * @note    使用循迹任务需先调用 Tracker_Init()，并让电机 PWM 驱动
 *          （bsp_motor_dirver）在首次开关动作时完成 PWM 初始化。
 * @note    循迹任务需约每10ms调用一次 Tracker_Update()，与主循环节拍匹配。
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

/* ==================== 状态与常量 ==================== */
#define TRACKER_LINE_LOST 999 // 未检测到黑线（全白）时 Tracker_Get_Position 返回值

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
 * @brief 初始化循迹传感器GPIO（P0.0~P0.4为准双向口）并初始化内部PID
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
 *             返回 TRACKER_LINE_LOST (999) 表示未检测到黑线（全部为白）
 * @note  权重：左起 -64, -32, 0, +32, +64
 */
int Tracker_Get_Position(void);

/**
 * @brief 启动循迹模式（复位PID、清空丢线状态）
 * @note  开启后需周期调用 Tracker_Update() 执行循迹。
 */
void Tracker_Start(void);

/**
 * @brief 停止循迹模式并停止电机
 */
void Tracker_Stop(void);

/**
 * @brief 查询循迹模式是否运行中
 * @return u8 1=运行中，0=已停止
 */
u8 Tracker_Running(void);

/**
 * @brief 切换循迹模式开关（供遥控按键调用）
 */
void Tracker_Toggle(void);

/**
 * @brief 设置循迹基础速度（值越大越快）
 * @param speed 基础速度，范围 20~100
 */
void Tracker_SetSpeed(int speed);

/**
 * @brief 循迹任务：采样→位置→丢线处理→PID→差速→驱动电机
 * @note  需约每10ms调用一次（匹配主循环节拍）。
 *        仅在 Tracker_Start() 开启后生效，未开启时不做任何操作。
 * @note  丢线策略：
 *        - 丢线后 <时间阈值：按最后丢失方向外推保持低速循迹，尝试找回黑线。
 *        - 仍找不到：原地旋转搜索（沿最后丢失方向），检测到黑线自动恢复。
 */
void Tracker_Update(void);

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