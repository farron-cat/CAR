#ifndef BSP_MOTOR_IO_H
#define BSP_MOTOR_IO_H

#define MOTOR_FORWARD 0
#define MOTOR_BACKWARD 1

#define MOTOR_RR_F P20 // 右后轮正转 PWM1P
#define MOTOR_RR_B P21 // 右后轮反转 PWM1N
#define MOTOR_RL_F P22 // 左后轮正转 PWM2P
#define MOTOR_RL_B P23 // 左后轮反转 PWM2N

#define MOTOR_FR_F P14 // 右前轮正转 PWM3P
#define MOTOR_FR_B P15 // 右前轮反转 PWM3N
#define MOTOR_FL_F P16 // 左前轮正转 PWM4P
#define MOTOR_FL_B P17 // 左前轮反转 PWM4N

void Motor_Init(void);

void Motor_SetDirection(unsigned char direction);

void Motor_RunWithDelay(unsigned char direction, unsigned char speed, unsigned int duration_ms);

void Motor_Stop(void);

#endif // BSP_MOTOR_IO_H