/**
 * @file    bsp_motor_driver.c
 * @brief   四轮直流电机驱动模块（PWM硬件控制）实现
 * @details 硬件初始化与运行时更新分离，保证波形连续无毛刺
 * @note    系统主频 MAIN_Fosc 需在编译器中全局定义（如 24000000UL）
 */

#include "bsp_motor_driver.h"
#include "STC8G_H_Switch.h" // 引脚切换功能
#include "STC8G_H_NVIC.h"   // 中断控制

#define PERIOD ((MAIN_Fosc / 1000) - 1) // PWM频率 = 1kHz

// 死区时间：防止H桥上下管直通（24MHz下约625ns）
#define DEAD_TIME 15

/**
 * @brief 将速度值(-100~100)映射为PWM比较值(占空比)
 * @param speed 速度值
 * @return 映射后的占空比比较值（0 ~ PERIOD）
 * @note  本驱动采用低电平有效PWM：
 *        speed=100 (最大前进) -> Duty=0  (输出常低，满速)
 *        speed=0   (停止)     -> Duty=50 (中间值，但ENO=0时无输出)
 *        speed=-100(最大后退) -> Duty=100(输出常高，满速)
 */
static int speed2duty(int speed)
{
    int duty_percent = 0;
    // 限幅保护
    if (speed > 100)
        speed = 100;
    if (speed < -100)
        speed = -100;

    // 映射公式：duty = -(speed / 2) + 50
    // 乘以 PERIOD/100 换算为寄存器比较值
    duty_percent = -(speed / 2) + 50;
    return (PERIOD * duty_percent) / 100;
}

/**
 * @brief 电机PWM硬件初始化
 */
void Motors_Init(void)
{
    PWMx_InitDefine pwmStruct;

    //  1. 配置PWMA全局寄存器（周期、死区、主输出）
    pwmStruct.PWM_Period = PERIOD;        // 周期
    pwmStruct.PWM_DeadTime = DEAD_TIME;   // 死区时间（防止直通）
    pwmStruct.PWM_MainOutEnable = ENABLE; // 主输出使能（常开）
    pwmStruct.PWM_CEN_Enable = ENABLE;    // 计数器使能（常开）
    pwmStruct.PWM_Mode = CCMRn_PWM_MODE1; // PWM模式1
    PWM_Configuration(PWMA, &pwmStruct);

    // 2. 固定PWM通道引脚映射（上电固定，运行时绝不切换）-----
    PWM1_SW(PWM1_SW_P20_P21); // 右后轮
    PWM2_SW(PWM2_SW_P22_P23); // 左后轮
    PWM3_SW(PWM3_SW_P14_P15); // 右前轮
    PWM4_SW(PWM4_SW_P16_P17); // 左前轮

    // 3. 禁用PWMA中断（不需要中断处理）
    NVIC_PWM_Init(PWMA, DISABLE, Priority_0);

    //  4. 初始化PWM1~PWM4通道（默认占空比0，无输出）
    pwmStruct.PWM_Mode = CCMRn_PWM_MODE1;
    pwmStruct.PWM_Duty = 0;
    pwmStruct.PWM_EnoSelect = 0;

    PWM_Configuration(PWM1, &pwmStruct);
    PWM_Configuration(PWM2, &pwmStruct);
    PWM_Configuration(PWM3, &pwmStruct);
    PWM_Configuration(PWM4, &pwmStruct);

    // 确保全局输出使能寄存器初始为0（所有通道关闭）
    PWMA_ENO = 0;
}

/**
 * @brief 运行时更新四轮速度（轻量级，仅操作CCR和ENO寄存器）
 */
void Motors_Update(Motor_Config_t cfg)
{
    // 1. 计算四轮占空比比较值（CCR）
    int RR_duty = speed2duty(cfg.RR_speed);
    int RL_duty = speed2duty(cfg.RL_speed);
    int FR_duty = speed2duty(cfg.FR_speed);
    int FL_duty = speed2duty(cfg.FL_speed);

    // 2. 直接写入硬件比较寄存器（影子寄存器，周期结束更新，平滑无毛刺）
    PWMA_CCR1 = RR_duty;
    PWMA_CCR2 = RL_duty;
    PWMA_CCR3 = FR_duty;
    PWMA_CCR4 = FL_duty;

    // 3. 更新通道使能：必须同时操作 CCER（比较输出/互补使能）与 ENO（引脚输出使能）。
    //    仅写 ENO 而 CCER 仍为 0 时，比较输出被禁用，引脚无 PWM 波形，电机不转。
    PWMA_ENO = 0;
    PWMA_CCER1_Disable(); // 关闭 PWM1/PWM2 比较输出（CC1E/CC1NE、CC2E/CC2NE）
    PWMA_CCER2_Disable(); // 关闭 PWM3/PWM4 比较输出（CC3E/CC3NE、CC4E/CC4NE）

    // 右后轮 PWM1
    if (cfg.RR_speed != 0)
    {
        PWMA_CC1E_Enable();    // 开启 PWM1 主通道比较输出
        PWMA_CC1NE_Enable();   // 开启 PWM1 互补通道比较输出
        PWMA_ENO |= (ENO1P | ENO1N); // 引脚输出使能
    }
    // 左后轮 PWM2
    if (cfg.RL_speed != 0)
    {
        PWMA_CC2E_Enable();    // 开启 PWM2 主通道比较输出
        PWMA_CC2NE_Enable();   // 开启 PWM2 互补通道比较输出
        PWMA_ENO |= (ENO2P | ENO2N);
    }
    // 右前轮 PWM3
    if (cfg.FR_speed != 0)
    {
        PWMA_CC3E_Enable();    // 开启 PWM3 主通道比较输出
        PWMA_CC3NE_Enable();   // 开启 PWM3 互补通道比较输出
        PWMA_ENO |= (ENO3P | ENO3N);
    }
    // 左前轮 PWM4
    if (cfg.FL_speed != 0)
    {
        PWMA_CC4E_Enable();    // 开启 PWM4 主通道比较输出
        PWMA_CC4NE_Enable();   // 开启 PWM4 互补通道比较输出
        PWMA_ENO |= (ENO4P | ENO4N);
    }
}

void Motors_Forward(int speed)
{
    Motor_Config_t cfg = {0};
    cfg.RR_speed = speed;
    cfg.RL_speed = speed;
    cfg.FR_speed = speed;
    cfg.FL_speed = speed;
    Motors_Update(cfg);
}

void Motors_Backward(int speed)
{
    Motor_Config_t cfg = {0};
    cfg.RR_speed = -speed;
    cfg.RL_speed = -speed;
    cfg.FR_speed = -speed;
    cfg.FL_speed = -speed;
    Motors_Update(cfg);
}

void Motors_Left(int speed, int dir)
{
    Motor_Config_t cfg = {0};

    // 右侧轮子向外翻（右后向后，右前向前）
    if (dir == 0 || dir == 1)
    {
        cfg.RR_speed = -speed;
        cfg.FR_speed = speed;
    }
    // 左侧轮子向内翻（左前向后，左后向前）
    if (dir == 0 || dir == -1)
    {
        cfg.FL_speed = -speed;
        cfg.RL_speed = speed;
    }
    Motors_Update(cfg);
}

void Motors_Right(int speed, int dir)
{
    Motor_Config_t cfg = {0};

    // 右侧轮子向内翻（右后向前，右前向后）
    if (dir == 0 || dir == 1)
    {
        cfg.RR_speed = speed;
        cfg.FR_speed = -speed;
    }
    // 左侧轮子向外翻（左前向前，左后向后）
    if (dir == 0 || dir == -1)
    {
        cfg.FL_speed = speed;
        cfg.RL_speed = -speed;
    }
    Motors_Update(cfg);
}

void Motors_Around(int speed, int dir)
{
    Motor_Config_t cfg = {0};

    if (dir == 0)
    { // 逆时针
        cfg.RR_speed = speed;
        cfg.FR_speed = speed;
        cfg.RL_speed = -speed;
        cfg.FL_speed = -speed;
    }
    else
    { // 顺时针
        cfg.RR_speed = -speed;
        cfg.FR_speed = -speed;
        cfg.RL_speed = speed;
        cfg.FL_speed = speed;
    }
    Motors_Update(cfg);
}

void Motors_Stop(void)
{
    Motor_Config_t cfg = {0}; // 全部置零
    Motors_Update(cfg);
}
