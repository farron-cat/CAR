#include "STC8H_PWM.h"
#include "STC8G_H_Switch.h"
#include "STC8G_H_NVIC.h"

#include "bsp_motor_dirver.h"

#define PERIOD ((MAIN_Fosc / 1000) - 1)

typedef struct
{
    int RR_speed; // 右后轮速度
    int RL_speed; // 左后轮速度
    int FR_speed; // 右前轮速度
    int FL_speed; // 左前轮速度
} MotorDriverConfig;

int speed2duty(int speed)
{
    // speed > 0 前进 eg.	 60
    // speed < 0 后退 eg. -60
    // speed ==0 停止 eg.   0

    // -100	--------------- 0 --------------- 100			speed
    // B_Max                0                F_Max

    // switch to

    // 100  --------------- 50 -------------- 0			duty
    // B_Max		               0              F_Max

    // (-100 -> 100) / 2  => -50 ->  50
    // -(-50 -> 50)				=> 	50 -> -50
    // (50 -> -50) + 50		=> 100 -> 	0

    return -(speed / 2) + 50;
}

void MotorDirver_PWM_Config(MotorDriverConfig cfg)
{
    PWMx_InitDefine pwmStruct;

    int RR_duty = speed2duty(cfg.RR_speed);
    int RL_duty = speed2duty(cfg.RL_speed);
    int FR_duty = speed2duty(cfg.FR_speed);
    int FL_duty = speed2duty(cfg.FL_speed);

    u8 is_RR_enable = (cfg.RR_speed != 0);
    u8 is_RL_enable = (cfg.RL_speed != 0);
    u8 is_FR_enable = (cfg.FR_speed != 0);
    u8 is_FL_enable = (cfg.FL_speed != 0);

    u8 mainEnable = is_RR_enable || is_RL_enable || is_FR_enable || is_FL_enable;

    // --------------------------------------------------------
    // 具体PWM端口配置

    // 右后轮 RR - pwm1
    pwmStruct.PWM_Mode = CCMRn_PWM_MODE1;                         // 模式, CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
    pwmStruct.PWM_Duty = PERIOD * RR_duty / 100;                  // PWM占空比时间, 0~Period
    pwmStruct.PWM_EnoSelect = is_RR_enable ? (ENO1P | ENO1N) : 0; // 输出通道选择, // ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N,ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM1, &pwmStruct);

    // 左后轮 RL - pwm2
    pwmStruct.PWM_Mode = CCMRn_PWM_MODE1;                         // 模式, CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
    pwmStruct.PWM_Duty = PERIOD * RL_duty / 100;                  // PWM占空比时间, 0~Period
    pwmStruct.PWM_EnoSelect = is_RL_enable ? (ENO2P | ENO2N) : 0; // 输出通道选择, // ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N,ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM2, &pwmStruct);

    // 右前轮 FR - pwm3
    pwmStruct.PWM_Mode = CCMRn_PWM_MODE1;                         // 模式, CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
    pwmStruct.PWM_Duty = PERIOD * FR_duty / 100;                  // PWM占空比时间, 0~Period
    pwmStruct.PWM_EnoSelect = is_FR_enable ? (ENO3P | ENO3N) : 0; // 输出通道选择, // ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N,ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM3, &pwmStruct);

    // 右后轮 FL - pwm4
    pwmStruct.PWM_Mode = CCMRn_PWM_MODE1;                         // 模式, CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
    pwmStruct.PWM_Duty = PERIOD * FL_duty / 100;                  // PWM占空比时间, 0~Period
    pwmStruct.PWM_EnoSelect = is_FL_enable ? (ENO4P | ENO4N) : 0; // 输出通道选择, // ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N,ENO5P,ENO6P,ENO7P,ENO8P
    PWM_Configuration(PWM4, &pwmStruct);

    // 配置PWMA
    pwmStruct.PWM_Period = PERIOD;            // 周期时间,   0~65535
    pwmStruct.PWM_DeadTime = 0;               // 死区发生器设置, 0~255
    pwmStruct.PWM_MainOutEnable = mainEnable; // 主输出使能, ENABLE,DISABLE
    pwmStruct.PWM_CEN_Enable = mainEnable;    // 使能计数器, ENABLE,DISABLE
    PWM_Configuration(PWMA, &pwmStruct);      // 初始化PWM通用寄存器,  PWMA,PWMB

    // 切换PWM通道
    PWM1_SW(PWM1_SW_P20_P21); // PWM1_SW_P10_P11,PWM1_SW_P20_P21,PWM1_SW_P60_P61
    PWM2_SW(PWM2_SW_P22_P23); // PWM2_SW_P12_P13,PWM2_SW_P22_P23,PWM2_SW_P62_P63
    PWM3_SW(PWM3_SW_P14_P15); // PWM3_SW_P14_P15,PWM3_SW_P24_P25,PWM3_SW_P64_P65
    PWM4_SW(PWM4_SW_P16_P17); // PWM4_SW_P16_P17,PWM4_SW_P26_P27,PWM4_SW_P66_P67,PWM4_SW_P34_P33

    // 禁用PWMA的中断
    NVIC_PWM_Init(PWMA, DISABLE, Priority_0);
}

void Motors_Forward(int speed)
{
    //   0  --------------- 50 -------------- 100			duty
    // F_Max---------------  0 --------------B_Max

    // -100	--------------- 0 --------------- 100			speed
    // B_Max                0                F_Max

    // Backward  - duty 100			50			0
    // Forward 	 - duty	 0			50		100

    MotorDriverConfig cfg = {0, 0, 0, 0};
    cfg.RR_speed = speed;
    cfg.RL_speed = speed;
    cfg.FR_speed = speed;
    cfg.FL_speed = speed;
    MotorDirver_PWM_Config(cfg);
}

void Motors_Backward(int speed)
{
    MotorDriverConfig cfg = {0, 0, 0, 0};
    cfg.RR_speed = -speed;
    cfg.RL_speed = -speed;
    cfg.FR_speed = -speed;
    cfg.FL_speed = -speed;
    MotorDirver_PWM_Config(cfg);
}

// 左平移，dir控制平移方向
void Motors_Left(int speed, int dir)
{
    MotorDriverConfig cfg = {0, 0, 0, 0};
    // 右侧轮子向外
    if (dir == 0 | dir == 1)
    {
        cfg.RR_speed = -speed; // 右后轮向后
        cfg.FR_speed = speed;  // 右前轮向前
    }

    // 左侧轮子向内
    if (dir == 0 | dir == -1)
    {
        cfg.FL_speed = -speed; // 左前轮向后
        cfg.RL_speed = speed;  // 左后轮向前
    }

    MotorDirver_PWM_Config(cfg);
}

// 右平移
void Motors_Right(int speed, int dir)
{
    MotorDriverConfig cfg = {0, 0, 0, 0};
    // 右侧轮子向内
    if (dir == 0 | dir == 1)
    {
        cfg.RR_speed = speed;  // 右后轮向前
        cfg.FR_speed = -speed; // 右前轮向后
    }

    // 左侧轮子向外
    if (dir == 0 | dir == -1)
    {
        cfg.FL_speed = speed;  // 左前轮向前
        cfg.RL_speed = -speed; // 左后轮向后
    }

    MotorDirver_PWM_Config(cfg);
}

// 旋转 dir控制旋转方向 顺时针，逆时针
void Motors_Around(int speed, int dir)
{
    MotorDriverConfig cfg = {0, 0, 0, 0};
    // 逆时针
    if (dir == 0)
    {
        cfg.RR_speed = speed;
        cfg.FR_speed = speed;
        cfg.RL_speed = -speed;
        cfg.FL_speed = -speed;
    }

    // 顺时针
    else
    {
        cfg.RR_speed = -speed;
        cfg.FR_speed = -speed;
        cfg.RL_speed = speed;
        cfg.FL_speed = speed;
    }
    MotorDirver_PWM_Config(cfg);
}

// 停止
void Motors_Stop()
{
    MotorDriverConfig cfg = {0, 0, 0, 0};
    cfg.RR_speed = 0;
    cfg.RL_speed = 0;
    cfg.FR_speed = 0;
    cfg.FL_speed = 0;
    MotorDirver_PWM_Config(cfg);
}