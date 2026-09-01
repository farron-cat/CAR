/**
 * @file    bsp_motor_dirver.h
 * @brief   电机驱动（PWM 硬件控制）模块接口
 * @details 基于 STC8H 硬件 PWM 的四路直流电机驱动接口：
 *          - 引脚定义：MOTOR_XX_F/B（正转/反转），分布于 P1、P2 口。
 *          - 运动接口：前进、后退、左移、右移、原地旋转、停止。
 *          通过速度值（-100~100）结合各运动模式配置 PWM 工作。
 * @note    PWM 通道与引脚：右后轮(PWM1)、左后轮(PWM2)、右前轮(PWM3)、左前轮(PWM4)。
 */
#ifndef BSP__MOTOR_DRIVER_H
#define BSP__MOTOR_DRIVER_H

#define MOTOR_RR_F P20 // 右后轮正转 PWM1P
#define MOTOR_RR_B P21 // 右后轮反转 PWM1N
#define MOTOR_RL_F P22 // 左后轮正转 PWM2P
#define MOTOR_RL_B P23 // 左后轮反转 PWM2N

#define MOTOR_FR_F P14 // 右前轮正转 PWM3P
#define MOTOR_FR_B P15 // 右前轮反转 PWM3N
#define MOTOR_FL_F P16 // 左前轮正转 PWM4P
#define MOTOR_FL_B P17 // 左前轮反转 PWM4N

/**
 * @brief 四轮速度配置结构体
 * @note  速度范围 -100 ~ 100：正数前进，负数后退，0 停止。
 *        RR_speed 右后轮、RL_speed 左后轮、FR_speed 右前轮、FL_speed 左前轮。
 */
typedef struct
{
    int RR_speed; // 右后轮速度
    int RL_speed; // 左后轮速度
    int FR_speed; // 右前轮速度
    int FL_speed; // 左前轮速度
} MotorDriverConfig;

/**
 * @brief 根据四项速度配置 PWM 输出（底层接口，高级运动接口与差速循迹均调用）
 * @param cfg 各轮速度配置 MotorDriverConfig（RR/RL/FR/FL_speed，范围 -100~100）
 * @note  高级运动接口（Motors_Forward 等）最终都调用本函数完成 PWM 配置。
 */
void MotorDirver_PWM_Config(MotorDriverConfig cfg);

/**
 * @brief 四轮全速前进
 * @param speed 前进速度，范围 0~100（值越大越快）。
 */
void Motors_Forward(int speed); // 前进

/**
 * @brief 四轮全速后退
 * @param speed 后退速度，范围 0~100（值越大越快）。
 */
void Motors_Backward(int speed); // 后退

/**
 * @brief 小车左平移
 * @param speed 平移速度，范围 0~100。
 * @param dir   平移方向（0：整体左移；1：右侧轮向外；-1：左侧轮向内）。
 */
void Motors_Left(int speed, int dir); // 左移

/**
 * @brief 小车右平移
 * @param speed 平移速度，范围 0~100。
 * @param dir   平移方向（0：整体右移；1：右侧轮向内；-1：左侧轮向外）。
 */
void Motors_Right(int speed, int dir); // 右移

/**
 * @brief 小车原地旋转
 * @param speed 旋转速度，范围 0~100。
 * @param dir   旋转方向（0：逆时针；其它非零值：顺时针）。
 */
void Motors_Around(int speed, int dir); // 原地打转

/**
 * @brief 停止所有电机
 */
void Motors_Stop(); // 停止

/**
 * @brief 麦克纳姆轮全向移动（摇杆控制）
 * @param x 摇杆横向分量（-100~100）：负=左移，正=右移
 * @param y 摇杆纵向分量（-100~100）：负=后退，正=前进
 * @note  通过四轮差速实现前进/后退/平移/转向的平滑组合，
 *        内部按 30% 比例缩放速度（避免全速）。
 */
void Motors_move(char x, char y); // 麦克纳姆全向摇杆控制

#endif // BSP__MOTOR_DRIVER_H