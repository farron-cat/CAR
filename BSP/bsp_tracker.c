/**
 * @file    bsp_tracker.c
 * @brief   五路循迹传感器驱动 + PID控制器实现
 * @version 2.0
 * @date    2026-08-31
 * @details 提供传感器扫描、位置计算（加权平均）及PID控制算法。
 *          PID采用位置式，带积分限幅和微分先行（仅对误差微分）。
 * @note    依赖 STC8G_H_GPIO.h 进行GPIO初始化。
 */

#include "STC8G_H_GPIO.h"
#include "bsp_tracker.h"

/**
 * @brief 初始化循迹传感器GPIO
 * @note  将P0.0~P0.4设置为准双向口（内部上拉），适合读取数字电平。
 */
void Tracker_Init(void)
{
    GPIO_InitTypeDef gpio_cfg;
    gpio_cfg.Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
    gpio_cfg.Mode = GPIO_PullUp; // 准双向口，输入时默认为高
    GPIO_Inilize(GPIO_P0, &gpio_cfg);
}

/**
 * @brief 扫描所有传感器，将状态存入数组
 * @param states 长度为5的u8数组
 * @note  直接读取引脚电平，存入对应位置。
 */
void Tracker_Scan(u8 states[])
{
    states[0] = TRACK_0; // 最左
    states[1] = TRACK_1; // 左
    states[2] = TRACK_2; // 中
    states[3] = TRACK_3; // 右
    states[4] = TRACK_4; // 最右
}

/**
 * @brief 获取当前黑线位置偏差（加权平均法）
 * @return int 偏差值，范围 -64 ~ +64，999表示未检测到黑线
 * @note  权重设置：左起 -64, -32, 0, +32, +64
 *        只有检测到黑线（值为0）的传感器参与加权平均。
 */
int Tracker_Get_Position(void)
{
    int position = 0;
    int count = 0;
    u8 states[5];
    Tracker_Scan(states);

    // 加权平均：每个传感器的位置权重
    if (states[0] == 0)
    {
        position += -64;
        count++;
    }
    if (states[1] == 0)
    {
        position += -32;
        count++;
    }
    if (states[2] == 0)
    {
        position += 0;
        count++;
    }
    if (states[3] == 0)
    {
        position += 32;
        count++;
    }
    if (states[4] == 0)
    {
        position += 64;
        count++;
    }

    if (count == 0)
    {
        // 没有检测到黑线，返回特殊值
        return 999;
    }

    return position / count; // 平均偏差
}

/* ==================== PID控制器实现 ==================== */

/**
 * @brief 初始化PID参数
 * @param pid      PID结构体指针
 * @param kp       比例系数
 * @param ki       积分系数
 * @param kd       微分系数
 * @param maxInt   积分限幅（绝对值）
 * @param maxOut   输出限幅（绝对值）
 */
void PID_Init(PID_TypeDef *pid,
              float kp, float ki, float kd,
              float maxInt, float maxOut)
{
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->MaxIntegral = maxInt;
    pid->MaxOutput = maxOut;
    PID_Reset(pid); // 清空历史状态
}

/**
 * @brief 重置PID积分项和上次误差
 * @param pid PID结构体指针
 */
void PID_Reset(PID_TypeDef *pid)
{
    pid->Integral = 0.0f;
    pid->LastError = 0.0f;
}

/**
 * @brief 执行PID计算（位置式）
 * @param pid   PID结构体指针
 * @param error 当前偏差（目标值 - 测量值）
 * @return float 控制量（已限幅）
 * @note  公式：输出 = Kp*error + Ki*∫error + Kd*(error - lastError)
 *        积分限幅防止饱和，微分项仅对误差微分（微分先行可选）。
 *        当误差符号改变时，积分项自动清零，加速响应。
 */
float PID_Calc(PID_TypeDef *pid, float error)
{
    float output;
    float P_term, I_term, D_term;

    // 比例项
    P_term = pid->Kp * error;

    // 积分项（带限幅）
    // 若误差符号改变，清零积分以防过冲
    if ((error > 0 && pid->LastError < 0) || (error < 0 && pid->LastError > 0))
    {
        pid->Integral = 0.0f;
    }

    pid->Integral += error;
    // 积分限幅
    if (pid->Integral > pid->MaxIntegral)
        pid->Integral = pid->MaxIntegral;
    else if (pid->Integral < -pid->MaxIntegral)
        pid->Integral = -pid->MaxIntegral;

    I_term = pid->Ki * pid->Integral;

    // 微分项
    D_term = pid->Kd * (error - pid->LastError);

    // 总输出
    output = P_term + I_term + D_term;

    // 输出限幅
    if (output > pid->MaxOutput)
        output = pid->MaxOutput;
    else if (output < -pid->MaxOutput)
        output = -pid->MaxOutput;

    // 更新上次误差
    pid->LastError = error;

    return output;
}