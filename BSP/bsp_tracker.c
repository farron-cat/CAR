/**
 * @file    bsp_tracker.c
 * @brief   五路循迹传感器驱动 + PID控制器 + 循迹任务
 * @details
 *          - 传感：Tracker_Scan / Tracker_Get_Position（加权平均）。
 *          - PID：位置式，带积分限幅和微分先行。
 *          - 循迹任务 Tracker_Update()（约10ms周期）：
 *              采样 → 位置 → 丢线处理（外推/搜索）→ PID → 左右差速驱动电机。
 * @note    依赖 STC8G_H_GPIO.h（GPIO）、bsp_motor_dirver.h（差速电机）、
 *          bsp_timer.h（tickMs 丢线计时）。
 */

#include "STC8G_H_GPIO.h"
#include "bsp_tracker.h"
#include "bsp_motor_dirver.h" // MotorDriverConfig / MotorDirver_PWM_Config
#include "bsp_timer.h"        // tickMs（丢线计时）

/* ==================== 循迹任务配置 ==================== */
#define TRACKER_BASE_SPEED 20   // 循迹基础速度（最低有效档，电机死区下限）
#define TRACKER_LOST_SPEED 20   // 丢线外推找回时的行驶速度（保持最低有效档）
#define TRACKER_LOST_MS 400     // 丢线后先外推找回的时长（ms），超过则原地搜索
#define TRACKER_SEARCH_SPEED 20 // 丢线搜索旋转速度（最低有效档）
#define TRACKER_POS_MAX 64      // 位置限幅（对应权重两极）

#define TRACKER_KP 1.2f       // 比例系数
#define TRACKER_KI 0.03f      // 积分系数（很小，避免过冲）
#define TRACKER_KD 0.6f       // 微分系数
#define TRACKER_MAX_INT 60.0f // 积分限幅
#define TRACKER_MAX_OUT 60.0f // PID 输出限幅（转向强度）

static int s_baseSpeed = TRACKER_BASE_SPEED; // 循迹基础速度
static u8 s_running = 0;                     // 循迹模式开关
static int s_lastPosition = 0;               // 上次有效位置（丢线外推基准）
static int s_lastDir = 0;                    // 上次有效方向：+1 线偏右 / -1 线偏左
static unsigned int s_lostStartMs = 0;       // 进入丢线状态的时刻 (tickMs)
static u8 s_wasLost = 0;                     // 上个周期是否处于丢线状态
static PID_TypeDef s_pid;                    // 内部 PID 实例

/**
 * @brief 初始化循迹传感器GPIO及内部 PID
 * @note  将P0.0~P0.4设置为准双向口（内部上拉），适合读取数字电平。
 */
void Tracker_Init(void)
{
    GPIO_InitTypeDef gpio_cfg;
    gpio_cfg.Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
    gpio_cfg.Mode = GPIO_PullUp; // 准双向口，输入时默认为高
    GPIO_Inilize(GPIO_P0, &gpio_cfg);

    // 内部 PID 参数：输出为转向强度，正值=左转（线偏左），负值=右转（线偏右）
    PID_Init(&s_pid, TRACKER_KP, TRACKER_KI, TRACKER_KD,
             TRACKER_MAX_INT, TRACKER_MAX_OUT);
    PID_Reset(&s_pid);

    s_running = 0;
    s_lastPosition = 0;
    s_lastDir = 0;
    s_wasLost = 0;
}

/**
 * @brief 扫描所有传感器，将状态存入数组
 * @param states 长度为5的u8数组
 * @note  直接读取引脚电平，存入对应位置。
 */
void Tracker_Scan(u8 states[])
{
    states[0] = TRACK_0; // 最左
    states[1] = TRACK_1; // 左
    states[2] = TRACK_2; // 中
    states[3] = TRACK_3; // 右
    states[4] = TRACK_4; // 最右
}

/**
 * @brief 获取当前黑线位置偏差（加权平均法）
 * @return int 偏差值，范围 -64 ~ +64，999表示未检测到黑线
 * @note  权重设置：左起 -64, -32, 0, +32, +64
 *        只有检测到黑线（值为0）的传感器参与加权平均。
 */
int Tracker_Get_Position(void)
{
    int position = 0;
    int count = 0;
    u8 states[5];
    Tracker_Scan(states);

    // 加权平均：每个传感器的位置权重
    if (states[0] == 0)
    {
        position += -64;
        count++;
    }
    if (states[1] == 0)
    {
        position += -32;
        count++;
    }
    if (states[2] == 0)
    {
        position += 0;
        count++;
    }
    if (states[3] == 0)
    {
        position += 32;
        count++;
    }
    if (states[4] == 0)
    {
        position += 64;
        count++;
    }

    if (count == 0)
    {
        // 没有检测到黑线，返回特殊值
        return TRACKER_LINE_LOST;
    }

    return position / count; // 平均偏差
}

/**
 * @brief 启动循迹模式
 * @note  复位 PID、清空丢线状态。之后周期调用 Tracker_Update() 执行循迹。
 */
void Tracker_Start(void)
{
    PID_Reset(&s_pid);
    s_lastPosition = 0;
    s_lastDir = 0;
    s_wasLost = 0;
    s_baseSpeed = TRACKER_BASE_SPEED;
    s_running = 1;
}

/**
 * @brief 停止循迹模式并停止电机
 */
void Tracker_Stop(void)
{
    s_running = 0;
    Motors_Stop();
}

/**
 * @brief 查询循迹模式是否运行中
 */
u8 Tracker_Running(void)
{
    return s_running;
}

/**
 * @brief 切换循迹模式开关
 */
void Tracker_Toggle(void)
{
    if (s_running)
        Tracker_Stop();
    else
        Tracker_Start();
}

/**
 * @brief 设置循迹基础速度
 * @param speed 基础速度，范围 20~100
 */
void Tracker_SetSpeed(int speed)
{
    if (speed < 20)
        speed = 20;
    if (speed > 100)
        speed = 100;
    s_baseSpeed = speed;
}

/**
 * @brief 将速度值限幅到 [-100, 100]
 */
static int Tracker_ClampSpeed(int v)
{
    if (v > 100)
        v = 100;
    if (v < -100)
        v = -100;
    return v;
}

/**
 * @brief 循迹任务（约10ms周期，配合主循环调用）
 * @note  流程：
 *        1. 未开启循迹则直接返回。
 *        2. 采样并计算加权位置与有效传感器数量。
 *        3. 有有效值：记录位置/方向，若刚从丢线恢复则复位 PID。
 *        4. 丢线：<阈值按最后方向外推低速找回；≥阈值原地旋转搜索。
 *        5. PID 计算转向量（正=左转），映射为左右轮差速并驱动电机。
 */
void Tracker_Update(void)
{
    int position;
    int count = 0; // 检测到黑线的传感器数量
    int turn;
    int leftSpeed;
    int rightSpeed;
    int i;
    u8 states[5];
    MotorDriverConfig cfg;

    if (!s_running)
        return; // 循迹未开启，不做任何操作

    // 1. 采样并计算位置与有效数
    Tracker_Scan(states);
    position = 0;
    for (i = 0; i < 5; i++)
    {
        if (states[i] == 0)
        {
            switch (i)
            {
            case 0:
                position += -64;
                break;
            case 1:
                position += -32;
                break;
            case 2:
                position += 0;
                break;
            case 3:
                position += 32;
                break;
            default:
                position += 64;
                break;
            }
            count++;
        }
    }

    if (count == 0)
    {
        // ---------- 丢线处理 ----------
        if (!s_wasLost)
        {
            s_wasLost = 1;
            s_lostStartMs = tickMs;
            PID_Reset(&s_pid); // 丢线瞬间复位积分，避免残留
        }

        if ((unsigned int)(tickMs - s_lostStartMs) < TRACKER_LOST_MS)
        {
            // 阶段1：按最后丢失方向外推，低速缓慢寻找
            position = s_lastPosition + s_lastDir * 8;
            if (position > TRACKER_POS_MAX)
                position = TRACKER_POS_MAX;
            if (position < -TRACKER_POS_MAX)
                position = -TRACKER_POS_MAX;
            s_baseSpeed = TRACKER_LOST_SPEED; // 丢线外推时保持最低有效档，便于找回
        }
        else
        {
            // 阶段2：原地旋转搜索（沿最后丢失方向）
            // s_lastDir>=0：线最后在右侧 → 顺时针右转搜索；否则逆时针左转
            if (s_lastDir >= 0)
                Motors_Around(TRACKER_SEARCH_SPEED, 1);
            else
                Motors_Around(TRACKER_SEARCH_SPEED, 0);
            return;
        }
    }
    else
    {
        // ---------- 在线：记录位置与方向 ----------
        if (s_wasLost)
        {
            // 刚从丢线恢复，复位 PID 并恢复基础速度
            PID_Reset(&s_pid);
            s_wasLost = 0;
            s_baseSpeed = TRACKER_BASE_SPEED;
        }
        position /= count; // 加权平均
        s_lastPosition = position;
        if (position > 4)
            s_lastDir = 1; // 线偏右
        else if (position < -4)
            s_lastDir = -1; // 线偏左
    }

    // 2. PID 计算：error = 目标(0) - 当前位置 → 正=线偏左(左转)，负=线偏右(右转)
    turn = (int)PID_Calc(&s_pid, (float)(-position));

    // 3. 差速映射：转向量正值→左轮慢右轮快（左转），负值→右轮慢左轮快（右转）
    leftSpeed = Tracker_ClampSpeed(s_baseSpeed - turn);
    rightSpeed = Tracker_ClampSpeed(s_baseSpeed + turn);

    // 4. 驱动电机（四轮差速：同侧一致）
    cfg.RR_speed = rightSpeed; // 右后
    cfg.RL_speed = leftSpeed;  // 左后
    cfg.FR_speed = rightSpeed; // 右前
    cfg.FL_speed = leftSpeed;  // 左前
    MotorDirver_PWM_Config(cfg);
}

/* ==================== PID控制器实现 ==================== */

/**
 * @brief 初始化PID参数
 * @param pid      PID结构体指针
 * @param kp       比例系数
 * @param ki       积分系数
 * @param kd       微分系数
 * @param maxInt   积分限幅（绝对值）
 * @param maxOut   输出限幅（绝对值）
 */
void PID_Init(PID_TypeDef *pid,
              float kp, float ki, float kd,
              float maxInt, float maxOut)
{
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->MaxIntegral = maxInt;
    pid->MaxOutput = maxOut;
    PID_Reset(pid); // 清空历史状态
}

/**
 * @brief 重置PID积分项和上次误差
 * @param pid PID结构体指针
 */
void PID_Reset(PID_TypeDef *pid)
{
    pid->Integral = 0.0f;
    pid->LastError = 0.0f;
}

/**
 * @brief 执行PID计算（位置式）
 * @param pid   PID结构体指针
 * @param error 当前偏差（目标值 - 测量值）
 * @return float 控制量（已限幅）
 * @note  公式：输出 = Kp*error + Ki*∫error + Kd*(error - lastError)
 *        积分限幅防止饱和，微分项仅对误差微分（微分先行可选）。
 *        当误差符号改变时，积分项自动清零，加速响应。
 */
float PID_Calc(PID_TypeDef *pid, float error)
{
    float output;
    float P_term, I_term, D_term;

    // 比例项
    P_term = pid->Kp * error;

    // 积分项（带限幅）
    // 若误差符号改变，清零积分以防过冲
    if ((error > 0 && pid->LastError < 0) || (error < 0 && pid->LastError > 0))
    {
        pid->Integral = 0.0f;
    }

    pid->Integral += error;
    // 积分限幅
    if (pid->Integral > pid->MaxIntegral)
        pid->Integral = pid->MaxIntegral;
    else if (pid->Integral < -pid->MaxIntegral)
        pid->Integral = -pid->MaxIntegral;

    I_term = pid->Ki * pid->Integral;

    // 微分项
    D_term = pid->Kd * (error - pid->LastError);

    // 总输出
    output = P_term + I_term + D_term;

    // 输出限幅
    if (output > pid->MaxOutput)
        output = pid->MaxOutput;
    else if (output < -pid->MaxOutput)
        output = -pid->MaxOutput;

    // 更新上次误差
    pid->LastError = error;

    return output;
}