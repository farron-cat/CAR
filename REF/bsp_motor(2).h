#ifndef __BSP_MOTOR_H
#define __BSP_MOTOR_H

#include "STC8G_H_GPIO.h"
#include "STC8H_PWM.h"

//方向定义：单电机H桥控制方向
#define DIR_IDLE    0   // 电机空闲，H桥关闭，电机滑行，无扭矩
#define DIR_FWD     1   // 电机正转
#define DIR_REV     2   // 电机反转
#define DIR_BRAKE   3   // H桥短路刹车，电机抱死制动

 
//麦克纳姆轮运动模式
#define MOTOR_FORWARD       1   // 整车向前直行
#define MOTOR_BACKWARD      2   // 整车向后直行
#define MOTOR_LEFT_SHIFT    3   // 整车横向左平移（侧移）
#define MOTOR_RIGHT_SHIFT   4   // 整车横向右平移（侧移）
#define MOTOR_CCW           5   // 原地逆时针旋转（车身左转）
#define MOTOR_CW            6   // 原地顺时针旋转（车身右转）
#define MOTOR_BRAKE         7   // 刹车，H桥短路制动
#define MOTOR_IDLE          8   // 空闲，电机滑行，无输出扭矩


//ch:1=M1左前，2=M2右前，3=M3左后，4=M4右后
void bsp_motor_init(void);
void motor_set(unsigned char ch, unsigned char dir, unsigned char duty);
void motor_run(unsigned char mode, unsigned char speed);

#endif
