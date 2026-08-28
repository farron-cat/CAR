#include "bsp_motor.h"
#include "STC8G_H_GPIO.h"
#include "STC8G_H_NVIC.h"
#include "STC8G_H_Switch.h"

/*
硬件映射表
ch 1~4
port_num: GPIO_P1 / GPIO_P2（库编号）
pinP/pinN: GPIO_Pin_x 掩码
pwm_ch:PWM通道号
【修改硬件只改本表，其余代码不动】
*/
typedef struct
{
    unsigned char pwm_ch;
    unsigned char port_num;
    unsigned char pinP;
    unsigned char pinN;
}MotorChDef;

static const MotorChDef motor_tbl[5] =
{
    {0,  0,          0,      0},      //0号占位，不用
    {4,  GPIO_P1, GPIO_Pin_6, GPIO_Pin_7}, //ch1 M1左前 PWM4 P1.6 P1.7
    {3,  GPIO_P1, GPIO_Pin_4, GPIO_Pin_5}, //ch2 M2右前 PWM3 P1.4 P1.5
    {2,  GPIO_P2, GPIO_Pin_2, GPIO_Pin_3}, //ch3 M3左后 PWM2 P2.2 P2.3
    {1,  GPIO_P2, GPIO_Pin_0, GPIO_Pin_1}, //ch4 M4右后 PWM1 P2.0 P2.1
};

//保存通道当前方向，避免重复开关ENO，减少波形断裂
static unsigned char cur_dir[5] = {DIR_IDLE,DIR_IDLE,DIR_IDLE,DIR_IDLE,DIR_IDLE};

/**
 * @brief 内部工具：设置IO口电平，适配STC原厂库
 * @param port_num GPIO_P1 / GPIO_P2
 * @param pin_mask GPIO_Pin_x
 * @param level 0=低，1=高
 */
static void motor_gpio_set_bit(unsigned char port_num, unsigned char pin_mask, unsigned char level)
{
    if(port_num == GPIO_P1)
    {
        if(level)   P1 |= pin_mask;
        else        P1 &= ~pin_mask;
    }
    else if(port_num == GPIO_P2)
    {
        if(level)   P2 |= pin_mask;
        else        P2 &= ~pin_mask;
    }
}

static void motor_pwm_init(void)
{
    PWMx_InitDefine PWMx_InitStructure;

    // 时钟预分频 24 → 1MHz计数时钟，Period=255 → PWM周期≈255us≈3.9kHz（无啸叫）
    PWMA_Prescaler(24 - 1);

    PWMx_InitStructure.PWM_Period = 255;
    PWMx_InitStructure.PWM_DeadTime = 0;
    PWMx_InitStructure.PWM_MainOutEnable = ENABLE;
    PWMx_InitStructure.PWM_CEN_Enable = ENABLE;
    PWM_Configuration(PWMA, &PWMx_InitStructure);

    PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1;
    PWMx_InitStructure.PWM_Duty = 0;

    // M4 右后 → PWM1 → P2.0(P)/P2.1(N)
    PWM1_SW(PWM1_SW_P20_P21);
    PWMx_InitStructure.PWM_EnoSelect = ENO1P | ENO1N;
    PWM_Configuration(PWM1, &PWMx_InitStructure);

    // M3 左后 → PWM2 → P2.2(P)/P2.3(N)
    PWM2_SW(PWM2_SW_P22_P23);
    PWMx_InitStructure.PWM_EnoSelect = ENO2P | ENO2N;
    PWM_Configuration(PWM2, &PWMx_InitStructure);

    // M2 右前 → PWM3 → P1.4(P)/P1.5(N)
    PWM3_SW(PWM3_SW_P14_P15);
    PWMx_InitStructure.PWM_EnoSelect = ENO3P | ENO3N;
    PWM_Configuration(PWM3, &PWMx_InitStructure);

    // M1 左前 → PWM4 → P1.6(P)/P1.7(N)
    PWM4_SW(PWM4_SW_P16_P17);
    PWMx_InitStructure.PWM_EnoSelect = ENO4P | ENO4N;
    PWM_Configuration(PWM4, &PWMx_InitStructure);

    NVIC_PWM_Init(PWMA, DISABLE, Priority_0);
}


/**
 * @brief 底层单电机控制函数
 * @param ch 1~4
 * @param dir DIR_IDLE/DIR_FWD/DIR_REV/DIR_BRAKE
 * @param duty 0~255
 */
void motor_set(unsigned char ch, unsigned char dir, unsigned char duty)
{
    const MotorChDef *p;   
    unsigned char spd_val;

    if(ch < 1 || ch > 4) return;
    p = &motor_tbl[ch];

    spd_val = duty;
    if(spd_val > 255) spd_val = 255;

    //设置占空比
    switch(p->pwm_ch)
    {
        case 1: PWMA_Duty1(spd_val); break;
        case 2: PWMA_Duty2(spd_val); break;
        case 3: PWMA_Duty3(spd_val); break;
        case 4: PWMA_Duty4(spd_val); break;
    }

    //方向没有变化，只更新占空比，不操作ENO，保护PWM波形
    if(cur_dir[ch] == dir)
    {
        return;
    }
    cur_dir[ch] = dir;

    switch(dir)
    {
        case DIR_FWD:
            //P输出PWM，N关闭
            switch(p->pwm_ch)
            {
                case 1: PWM1P_OUT_EN();  PWM1N_OUT_DIS(); break;
                case 2: PWM2P_OUT_EN();  PWM2N_OUT_DIS(); break;
                case 3: PWM3P_OUT_EN();  PWM3N_OUT_DIS(); break;
                case 4: PWM4P_OUT_EN();  PWM4N_OUT_DIS(); break;
            }
            break;

        case DIR_REV:
            //N输出PWM，P关闭
            switch(p->pwm_ch)
            {
                case 1: PWM1P_OUT_DIS(); PWM1N_OUT_EN(); break;
                case 2: PWM2P_OUT_DIS(); PWM2N_OUT_EN(); break;
                case 3: PWM3P_OUT_DIS(); PWM3N_OUT_EN(); break;
                case 4: PWM4P_OUT_DIS(); PWM4N_OUT_EN(); break;
            }
            break;

        case DIR_BRAKE:
            //关闭PWM外设输出，交还GPIO，双引脚拉低实现H桥短路刹车
            switch(p->pwm_ch)
            {
                case 1: PWM1P_OUT_DIS(); PWM1N_OUT_DIS(); break;
                case 2: PWM2P_OUT_DIS(); PWM2N_OUT_DIS(); break;
                case 3: PWM3P_OUT_DIS(); PWM3N_OUT_DIS(); break;
                case 4: PWM4P_OUT_DIS(); PWM4N_OUT_DIS(); break;
            }
            motor_gpio_set_bit(p->port_num, p->pinP, 0);
            motor_gpio_set_bit(p->port_num, p->pinN, 0);
            break;

        case DIR_IDLE:
        default:
            //关闭PWM外设输出，交还GPIO
            switch(p->pwm_ch)
            {
                case 1: PWM1P_OUT_DIS(); PWM1N_OUT_DIS(); break;
                case 2: PWM2P_OUT_DIS(); PWM2N_OUT_DIS(); break;
                case 3: PWM3P_OUT_DIS(); PWM3N_OUT_DIS(); break;
                case 4: PWM4P_OUT_DIS(); PWM4N_OUT_DIS(); break;
            }
            /*
            !!!重要提醒!!!
            TB6612：INA INB = 0 1 才是滑行高阻；0 0为刹车
            L298N：0 0即为滑行
            如果你的板子是TB6612，请改成：
            motor_gpio_set_bit(p->port_num, p->pinP, 0);
            motor_gpio_set_bit(p->port_num, p->pinN, 1);
            */
            motor_gpio_set_bit(p->port_num, p->pinP, 0);
            motor_gpio_set_bit(p->port_num, p->pinN, 0);
            break;
    }
}


/**
 * @brief 初始化GPIO、PWM，上电默认停止滑行
 */
void bsp_motor_init(void)
{
    GPIO_InitTypeDef gpio_cfg;

    //P1.4~P1.7推挽输出
    gpio_cfg.Mode = GPIO_OUT_PP;
    gpio_cfg.Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Inilize(GPIO_P1, &gpio_cfg);

    //P2.0~P2.3推挽输出
    gpio_cfg.Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_Inilize(GPIO_P2, &gpio_cfg);

    motor_pwm_init();

    //全部电机进入空闲状态
    motor_set(1,DIR_IDLE,0);
    motor_set(2,DIR_IDLE,0);
    motor_set(3,DIR_IDLE,0);
    motor_set(4,DIR_IDLE,0);
}

/**
 * @brief 运动模式函数，麦克纳姆轮成套运动
 * @param mode MOTOR_FORWARD/MOTOR_BACKWARD/MOTOR_LEFT_SHIFT...
 * @param speed 0~255
 */
void motor_run(unsigned char mode, unsigned char speed)
{
    unsigned char spd = speed > 255 ? 255 : speed;
    switch(mode)
    {
        case MOTOR_FORWARD:
            motor_set(1,DIR_FWD,spd);
            motor_set(2,DIR_FWD,spd);
            motor_set(3,DIR_FWD,spd);
            motor_set(4,DIR_FWD,spd);
            break;
        case MOTOR_BACKWARD:
            motor_set(1,DIR_REV,spd);
            motor_set(2,DIR_REV,spd);
            motor_set(3,DIR_REV,spd);
            motor_set(4,DIR_REV,spd);
            break;
        case MOTOR_LEFT_SHIFT:
            motor_set(1,DIR_REV,spd);
            motor_set(2,DIR_FWD,spd);
            motor_set(3,DIR_FWD,spd);
            motor_set(4,DIR_REV,spd);
            break;
        case MOTOR_RIGHT_SHIFT:
            motor_set(1,DIR_FWD,spd);
            motor_set(2,DIR_REV,spd);
            motor_set(3,DIR_REV,spd);
            motor_set(4,DIR_FWD,spd);
            break;
        case MOTOR_CCW:
            motor_set(1,DIR_REV,spd);
            motor_set(2,DIR_FWD,spd);
            motor_set(3,DIR_REV,spd);
            motor_set(4,DIR_FWD,spd);
            break;
        case MOTOR_CW:
            motor_set(1,DIR_FWD,spd);
            motor_set(2,DIR_REV,spd);
            motor_set(3,DIR_FWD,spd);
            motor_set(4,DIR_REV,spd);
            break;
        case MOTOR_BRAKE:
            motor_set(1,DIR_BRAKE,0);
            motor_set(2,DIR_BRAKE,0);
            motor_set(3,DIR_BRAKE,0);
            motor_set(4,DIR_BRAKE,0);
            break;
        case MOTOR_IDLE:
        default:
            motor_set(1,DIR_IDLE,0);
            motor_set(2,DIR_IDLE,0);
            motor_set(3,DIR_IDLE,0);
            motor_set(4,DIR_IDLE,0);
            break;
    }
}
