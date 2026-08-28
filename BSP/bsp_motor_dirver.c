/**
 * @file    bsp_motor_dirver.c
 * @brief   电机驱动（PWM 硬件控制）模块
 * @details 本模块基于 STC8H 的 PWMA 定时器/比较器，通过硬件 PWM 驱动四路直流电机：
 *          - 使用 PWM1 ~ PWM4 四路输出，分别对应右后、左后、右前、左前四轮。
 *          - 每个电机由正转（P）与反转（N）两路 PWM 组成，占空比切换实现方向与调速。
 *          通过速度值（-100~100）映射为占空比（0~100），对外提供前进、后退、平移、旋转等运动接口。
 * @note    依赖 STC8H_PWM.h、STC8G_H_Switch.h（通道切换）、STC8G_H_NVIC.h（中断控制）。
 * @note    使用前需先完成 PWM 引脚初始化与系统时钟（MAIN_Fosc）配置。
 * @note    PWM 周期由宏 PERIOD 决定：PERIOD = (MAIN_Fosc / 1000) - 1，即 1kHz。
 */

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

/**
 * @brief 将速度值映射为 PWM 占空比
 * @param speed 速度值，范围 -100 ~ 100：
 *          - speed > 0  前进（如 60）
 *          - speed < 0  后退（如 -60）
 *          - speed == 0 停止（如   0）
 * @return 映射后的占空比，范围 0 ~ 100：
 *          - speed=-100（最大后退）=> 占空比 100（B_Max）
 *          - speed=0              => 占空比  50
 *          - speed=100（最大前进）=> 占空比   0（F_Max）
 * @note  映射公式：duty = -(speed / 2) + 50。
 */
int speed2duty(int speed)
{
    return -(speed / 2) + 50;
}

/**
 * @brief 根据四项速度配置 PWM1 ~ PWM4 的占空比与输出通道
 * @param cfg 各轮速度配置 MotorDriverConfig（RR/RL/FR/FL_speed，范围 -100~100）
 * @note  通过 speed2duty() 将每轮速度换算成占空比，写入各通道。
 * @note  速度为 0 的通道被禁用（PWM_EnoSelect = 0），任一通道使能则开启 PWMA 主输出与计数器。
 * @note  配置 PWM1~PWM4 通道引脚映射（P20/P21、P22/P23、P14/P15、P16/P17），并禁用 PWMA 中断。
 */
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

/**
 * @brief 四轮全速前进
 * @param speed 前进速度，范围 0~100（值越大越快）。
 * @note  四轮同步前进，速度越高占空比越小（见 speed2duty() 映射）。
 */
void Motors_Forward(int speed)
{
    MotorDriverConfig cfg = {0, 0, 0, 0};
    cfg.RR_speed = speed;
    cfg.RL_speed = speed;
    cfg.FR_speed = speed;
    cfg.FL_speed = speed;
    MotorDirver_PWM_Config(cfg);
}

/**
 * @brief 四轮全速后退
 * @param speed 后退速度，范围 0~100（值越大越快）。
 * @note  向每轮配置负速度，对应反转方向。
 */
void Motors_Backward(int speed)
{
    MotorDriverConfig cfg = {0, 0, 0, 0};
    cfg.RR_speed = -speed;
    cfg.RL_speed = -speed;
    cfg.FR_speed = -speed;
    cfg.FL_speed = -speed;
    MotorDirver_PWM_Config(cfg);
}

/**
 * @brief 小车左平移（横向移动）
 * @param speed 平移速度，范围 0~100。
 * @param dir   平移方向控制：
 *          - dir == 1  右侧轮子向外（右后轮向后、右前轮向前）
 *          - dir == -1 左侧轮子向内（左前轮向后、左后轮向前）
 *          - dir == 0  两端（向外 + 向内）同时生效，实现整体左移
 * @note  通过左右两侧轮子反向转动实现横向平移。
 */
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

/**
 * @brief 小车右平移（横向移动）
 * @param speed 平移速度，范围 0~100。
 * @param dir   平移方向控制：
 *          - dir == 1  右侧轮子向内（右后轮向前、右前轮向后）
 *          - dir == -1 左侧轮子向外（左前轮向前、左后轮向后）
 *          - dir == 0  两端（向内 + 向外）同时生效，实现整体右移
 * @note  通过左右两侧轮子反向转动实现横向平移。
 */
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

/**
 * @brief 小车原地旋转（打转）
 * @param speed 旋转速度，范围 0~100。
 * @param dir   旋转方向：
 *          - dir == 0             逆时针（左侧轮向后、右侧轮向前）
 *          - dir 其它非零值       顺时针（左侧轮向前、右侧轮向后）
 * @note  通过左右两侧轮子反向转动、速度相同实现原地打转。
 */
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

/**
 * @brief 停止所有电机
 * @note 将四轮速度全部置 0，通过 MotorDirver_PWM_Config() 关闭各通道输出。
 */
void Motors_Stop()
{
    MotorDriverConfig cfg = {0, 0, 0, 0};
    cfg.RR_speed = 0;
    cfg.RL_speed = 0;
    cfg.FR_speed = 0;
    cfg.FL_speed = 0;
    MotorDirver_PWM_Config(cfg);
}