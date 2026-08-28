 #include "bsp_motor.h"

#include "STC8G_H_GPIO.h"
#include "STC8H_PWM.h"
#include "STC8G_H_Switch.h"
#include "STC8G_H_NVIC.h"

#define M4_P1 P16   // 左前 PWM4P
#define M4_P2 P17   // 左前 PWM4N
#define M3_P1 P14   // 右前 PWM3P
#define M3_P2 P15   // 右前 PWM3N
#define M2_P1 P22   // 左后 PWM2P
#define M2_P2 P23   // 左后 PWM2N
#define M1_P1 P20   // 右后 PWM1P
#define M1_P2 P21   // 右后 PWM1N



#define MOTOR_MODE  1   // 0:互补输出   1: 单路输出

#define PWM_DUTY_MAX    1200                // PWM比较值最大值
#define PWM_DUTY_MID    PWM_DUTY_MAX>>1     // PWM比较值中位数
#define PWM_DUTY_MIN    0                   // PWM比较值最小值


/**
 * @brief 电机初始化
 * @note  配置PWMA，频率10kHz。
 */
void Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    PWMx_InitDefine PWMx_InitStructure = {0};

    GPIO_InitStruct.Mode = GPIO_OUT_PP;
    GPIO_InitStruct.Pin = GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
    GPIO_Inilize(GPIO_P1, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;
    GPIO_Inilize(GPIO_P2, &GPIO_InitStruct);
    M1_P1 = 0; M1_P2 = 0;
    M2_P1 = 0; M2_P2 = 0;
    M3_P1 = 0; M3_P2 = 0;
    M4_P1 = 0; M4_P2 = 0;

#if MOTOR_MODE == 0
    // 配置PWM通道参数
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1;  // 使用模式1，duty越大，占空比越大
    PWMx_InitStructure.PWM_Duty = 0;
    PWMx_InitStructure.PWM_EnoSelect = ENO1P|ENO1N;
    PWM_Configuration(PWM1, &PWMx_InitStructure);
    PWMx_InitStructure.PWM_EnoSelect = ENO2P|ENO2N;
    PWM_Configuration(PWM2, &PWMx_InitStructure);
    PWMx_InitStructure.PWM_EnoSelect = ENO3P|ENO3N;
    PWM_Configuration(PWM3, &PWMx_InitStructure);
    PWMx_InitStructure.PWM_EnoSelect = ENO4P|ENO4N;
    PWM_Configuration(PWM4, &PWMx_InitStructure);
#elif MOTOR_MODE == 1   // 单通道模式不能同时开两个
    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE2;  // 使用模式2，duty越大，占空比越小
    PWMx_InitStructure.PWM_Duty = 0;
    PWMx_InitStructure.PWM_EnoSelect = ENO1P;
    PWM_Configuration(PWM1, &PWMx_InitStructure);
    PWMx_InitStructure.PWM_EnoSelect = ENO2P;
    PWM_Configuration(PWM2, &PWMx_InitStructure);
    PWMx_InitStructure.PWM_EnoSelect = ENO3P;
    PWM_Configuration(PWM3, &PWMx_InitStructure);
    PWMx_InitStructure.PWM_EnoSelect = ENO4P;
    PWM_Configuration(PWM4, &PWMx_InitStructure);
#endif
    // 配置PWM总参数
    PWMx_InitStructure.PWM_DeadTime = 5;            //死区发生器设置, 0~255
    PWMx_InitStructure.PWM_Period = PWM_DUTY_MAX - 1;         //周期时间，10kHz
    PWMx_InitStructure.PWM_MainOutEnable = ENABLE; //主输出使能
    PWMx_InitStructure.PWM_CEN_Enable = ENABLE;     //使能计数器
    PWM_Configuration(PWMA, &PWMx_InitStructure);

    // PWM引脚映射
    PWM1_SW(PWM1_SW_P20_P21);
    PWM2_SW(PWM2_SW_P22_P23);
    PWM3_SW(PWM3_SW_P14_P15);
    PWM4_SW(PWM4_SW_P16_P17);
    // 关闭PWM中断
    NVIC_PWM_Init(PWMA,DISABLE,Priority_0);

}

#if MOTOR_MODE == 0

/**
 * @brief 设置电机偏移后的占空比比较值
 * @param LeftFront,RightFront,LeftRear,RightRear 偏移后的比较值
 *          -600 ~ 1: 反转
 *          1 ~ 600 : 正转
 */
void Motor_SetAll(int LeftFront, int RightFront, int LeftRear, int RightRear)
{
    uint16 tmp = 0;

    // 左前
    LeftFront = (LeftFront>(-PWM_DUTY_MID)?LeftFront:(-PWM_DUTY_MID))<PWM_DUTY_MID?LeftFront:PWM_DUTY_MID;  // 限幅-600~600
    tmp = (uint16)(LeftFront + 600);
    if (tmp == PWM_DUTY_MID) {
        PWMA_CC4E_Disable();
        PWMA_CC4NE_Disable();
    } else if (tmp <= PWM_DUTY_MAX) {
        PWMA_CC4E_Enable();
        PWMA_CC4NE_Enable();
        PWMA_Duty4(tmp);       // 反转
    }

    // 右前
    RightFront = (RightFront>(-PWM_DUTY_MID)?RightFront:(-PWM_DUTY_MID))<PWM_DUTY_MID?RightFront:PWM_DUTY_MID;  // 限幅-600~600
    tmp = (uint16)(RightFront + 600);
    if (tmp == PWM_DUTY_MID) {
        PWMA_CC3E_Disable();
        PWMA_CC3NE_Disable();
    } else if (tmp <= PWM_DUTY_MAX) {
        PWMA_CC3E_Enable();
        PWMA_CC3NE_Enable();
        PWMA_Duty3(tmp);       // 反转
    }

    // 左后
    LeftRear = (LeftRear>(-PWM_DUTY_MID)?LeftRear:(-PWM_DUTY_MID))<PWM_DUTY_MID?LeftRear:PWM_DUTY_MID;  // 限幅-600~600
    tmp = (uint16)(LeftRear + 600);
    if (tmp == PWM_DUTY_MID) {
        PWMA_CC2E_Disable();
        PWMA_CC2NE_Disable();
    } else if (tmp <= PWM_DUTY_MAX) {
        PWMA_CC2E_Enable();
        PWMA_CC2NE_Enable();
        PWMA_Duty2(tmp);       // 反转
    }

    // 右后
    RightRear = (RightRear>(-PWM_DUTY_MID)?RightRear:(-PWM_DUTY_MID))<PWM_DUTY_MID?RightRear:PWM_DUTY_MID;  // 限幅-600~600
    tmp = (uint16)(RightRear + 600);
    if (tmp == PWM_DUTY_MID) {
        PWMA_CC1E_Disable();
        PWMA_CC1NE_Disable();
    } else if (tmp <= PWM_DUTY_MAX) {
        PWMA_CC1E_Enable();
        PWMA_CC1NE_Enable();
        PWMA_Duty1(tmp);       // 反转
    }
}

#elif MOTOR_MODE == 1

/**
 * @brief 设置速度
 * @param LeftFront,RightFront,LeftRear,RightRear 比较值
 *          200 ~ 1200
 */
void Motor_SetAll(int LeftFront, int RightFront, int LeftRear, int RightRear)
{
    uint16 tmp = 0;

    if(LeftFront == 0) {
        // 刹车
        PWMA_CC4E_Disable();    // 关闭PWM4P通道,GPIO置高
        PWMA_ENO &= ~ENO4P;
        M4_P1 = 1;
        PWMA_CC4NE_Disable();    // 关闭PWM4N通道,GPIO置高
        PWMA_ENO &= ~ENO4N;
        M4_P2 = 1;
    } else if(LeftFront > 0) {
        // 左前正转
        tmp = (uint16)(LeftFront<PWM_DUTY_MAX?LeftFront:PWM_DUTY_MAX);   // 限幅
        PWMA_CC4E_Disable();    // 关闭PWM4P通道,GPIO置高
        PWMA_ENO &= ~ENO4P;
        M4_P1 = 1;
        PWMA_CC4NE_Enable();    // 开启PWM4N通道，配置PWM占空比
        PWMA_ENO |= ENO4N;
        PWMA_Duty4(tmp);
    } else if(LeftFront < 0) {
        // 左前倒转
        tmp = (uint16)((-LeftFront)<PWM_DUTY_MAX?(-LeftFront):PWM_DUTY_MAX);   // 取反限幅
        PWMA_CC4NE_Disable();    // 关闭PWM4N通道,GPIO置高
        PWMA_ENO &= ~ENO4N;
        M4_P2 = 1;
        PWMA_CC4E_Enable();    // 开启PWM4N通道，配置PWM占空比
        PWMA_ENO |= ENO4P;
        PWMA_Duty4(tmp);
    }

    if(RightFront == 0) {
        // 刹车
        PWMA_CC3E_Disable();    // 关闭PWM4P通道,GPIO置高
        PWMA_ENO &= ~ENO3P;
        M3_P1 = 1;
        PWMA_CC3NE_Disable();    // 关闭PWM4N通道,GPIO置高
        PWMA_ENO &= ~ENO3N;
        M3_P2 = 1;
    } else if(RightFront > 0) {
        // 右前正转
        tmp = (uint16)(RightFront<PWM_DUTY_MAX?RightFront:PWM_DUTY_MAX);   // 限幅
        PWMA_CC3E_Disable();    // 关闭PWM4P通道,GPIO置高
        PWMA_ENO &= ~ENO3P;
        M3_P1 = 1;
        PWMA_CC3NE_Enable();    // 开启PWM4N通道，配置PWM占空比
        PWMA_ENO |= ENO3N;
        PWMA_Duty3(tmp);
    } else if(RightFront < 0) {
        // 右前倒转
        tmp = (uint16)((-RightFront)<PWM_DUTY_MAX?(-RightFront):PWM_DUTY_MAX);   // 取反限幅
        PWMA_CC3NE_Disable();    // 关闭PWM4N通道,GPIO置高
        PWMA_ENO &= ~ENO3N;
        M3_P2 = 1;
        PWMA_CC3E_Enable();    // 开启PWM4N通道，配置PWM占空比
        PWMA_ENO |= ENO3P;
        PWMA_Duty3(tmp);
    }


    if(LeftRear == 0) {
        // 左后刹车
        PWMA_CC2E_Disable();    // 关闭PWM4P通道,GPIO置高
        PWMA_ENO &= ~ENO2P;
        M2_P1 = 1;
        PWMA_CC2NE_Disable();    // 关闭PWM4N通道,GPIO置高
        PWMA_ENO &= ~ENO2N;
        M2_P2 = 1;
    } else if(LeftRear > 0) {
        // 左后正转
        tmp = (uint16)(LeftRear<PWM_DUTY_MAX?LeftRear:PWM_DUTY_MAX);   // 限幅
        PWMA_CC2E_Disable();    // 关闭PWM4P通道,GPIO置高
        PWMA_ENO &= ~ENO2P;
        M2_P1 = 1;
        PWMA_CC2NE_Enable();    // 开启PWM4N通道，配置PWM占空比
        PWMA_ENO |= ENO2N;
        PWMA_Duty2(tmp);
    } else if(LeftRear < 0) {
        // 左后倒转
        tmp = (uint16)((-LeftRear)<PWM_DUTY_MAX?(-LeftRear):PWM_DUTY_MAX);   // 取反限幅
        PWMA_CC2NE_Disable();    // 关闭PWM4N通道,GPIO置高
        PWMA_ENO &= ~ENO2N;
        M2_P2 = 1;
        PWMA_CC2E_Enable();    // 开启PWM4N通道，配置PWM占空比
        PWMA_ENO |= ENO2P;
        PWMA_Duty2(tmp);
    }

    if(RightRear == 0) {
        // 右后刹车
        PWMA_CC1E_Disable();    // 关闭PWM4P通道,GPIO置高
        PWMA_ENO &= ~ENO1P;
        M1_P1 = 1;
        PWMA_CC1NE_Disable();    // 关闭PWM4N通道,GPIO置高
        PWMA_ENO &= ~ENO1N;
        M1_P2 = 1;
    } else if(RightRear > 0) {
        // 右后正转
        tmp = (uint16)(RightRear<PWM_DUTY_MAX?RightRear:PWM_DUTY_MAX);   // 限幅
        PWMA_CC1E_Disable();    // 关闭PWM4P通道,GPIO置高
        PWMA_ENO &= ~ENO1P;
        M1_P1 = 1;
        PWMA_CC1NE_Enable();    // 开启PWM4N通道，配置PWM占空比
        PWMA_ENO |= ENO1N;
        PWMA_Duty1(tmp);
    } else if(RightRear < 0) {
        // 右后倒转
        tmp = (uint16)((-RightRear)<PWM_DUTY_MAX?(-RightRear):PWM_DUTY_MAX);   // 取反限幅
        PWMA_CC1NE_Disable();    // 关闭PWM4N通道,GPIO置高
        PWMA_ENO &= ~ENO1N;
        M1_P2 = 1;
        PWMA_CC1E_Enable();    // 开启PWM4N通道，配置PWM占空比
        PWMA_ENO |= ENO1P;
        PWMA_Duty1(tmp);
    }
}

#endif
