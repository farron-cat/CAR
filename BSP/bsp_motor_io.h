/**
 * @file    bsp_motor_io.h
 * @brief   电机 I/O 控制驱动模块接口
 * @details 提供四路小车直流电机的控制引脚定义与操控接口：
 *          - 引脚定义：MOTOR_XX_F/B（正转/反转），分布于 P1、P2 口。
 *          - 方向定义：MOTOR_FORWARD（正转）与 MOTOR_BACKWARD（反转）。
 *          对外暴露初始化、方向设置、软件 PWM 变速及停止等接口。
 * @note    使用时需在调用方向/速度控制前先调用 Motor_Init() 完成引脚配置。
 */
#ifndef BSP_MOTOR_IO_H
#define BSP_MOTOR_IO_H

// 电机转向定义
#define MOTOR_FORWARD 0  // 正转
#define MOTOR_BACKWARD 1 // 反转

// 右后轮
#define MOTOR_RR_F P20 // 右后轮正转 PWM1P
#define MOTOR_RR_B P21 // 右后轮反转 PWM1N
// 左后轮
#define MOTOR_RL_F P22 // 左后轮正转 PWM2P
#define MOTOR_RL_B P23 // 左后轮反转 PWM2N

// 右前轮
#define MOTOR_FR_F P14 // 右前轮正转 PWM3P
#define MOTOR_FR_B P15 // 右前轮反转 PWM3N
// 左前轮
#define MOTOR_FL_F P16 // 左前轮正转 PWM4P
#define MOTOR_FL_B P17 // 左前轮反转 PWM4N

/**
 * @brief 初始化所有电机控制引脚为推挽输出
 * @note 配置 P1.4 ~ P1.7（前轮）与 P2.0 ~ P2.3(后轮) 为推挽输出模式。
 */
void Motor_Init(void);

/**
 * @brief 设置所有电机转向
 * @param direction MOTOR_FORWARD（正转）或 MOTOR_BACKWARD（反转）。
 * @note  调用前需先完成 Motor_Init() 引脚初始化。
 */
void Motor_SetDirection(unsigned char direction);

/**
 * @brief 使用软件 PWM 控制电机速度（阻塞式）
 * @param direction  方向：MOTOR_FORWARD / MOTOR_BACKWARD。
 * @param speed      速度等级：0~100（0 停止，100 全速）。
 * @param duration_ms 持续运行的总时间（单位 ms），达到后停止。
 * @note 函数会阻塞执行，期间无法响应其它任务。
 */
void Motor_RunWithDelay(unsigned char direction, unsigned char speed, unsigned int duration_ms);

/**
 * @brief 停止所有电机（将所有控制引脚置低）
 */
void Motor_Stop(void);

#endif // BSP_MOTOR_IO_H