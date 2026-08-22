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

void MotorDirver_Config();

void Motors_Forward(int speed);         // 前进
void Motors_Backward(int speed);        // 后退
void Motors_Left(int speed, int dir);   // 左移
void Motors_Right(int speed, int dir);  // 右移
void Motors_Around(int speed, int dir); // 原地打转
void Motors_Stop();                     // 停止

#endif // BSP__MOTOR_DRIVER_H