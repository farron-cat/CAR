#include "STC8G_H_GPIO.h"
#include "STC8G_H_UART.h"
#include "STC8G_H_NVIC.h"
#include "STC8G_H_Switch.h"

#include "bsp_uart.h"
#include "bsp_motor_dirver.h"

unsigned char UART1_RxFlag = 0; // 串口1接收完成标志位

void UART1Init(void)
{
    // 配置结构体定义
    GPIO_InitTypeDef GPIO_UART1_P3_PU;
    COMx_InitDefine UART1_8BIT_BRTX;

    // 配置串口引脚 P3.0 P3.1 为准双向模式
    GPIO_UART1_P3_PU.Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_UART1_P3_PU.Mode = GPIO_PullUp;
    GPIO_Inilize(GPIO_P3, &GPIO_UART1_P3_PU);

    // 配置串口1为8位数据，波特率115600，
    UART1_8BIT_BRTX.UART_Mode = UART_8bit_BRTx;  // 模式：8位数据，波特率可变
    UART1_8BIT_BRTX.UART_BRT_Use = BRT_Timer2;   // 波特率发生器：定时器2（Timer1用于数码管扫描）
    UART1_8BIT_BRTX.UART_BaudRate = 115200;      // 波特率：115200
    UART1_8BIT_BRTX.UART_RxEnable = ENABLE;      // 接收允许：允许
    UART1_8BIT_BRTX.BaudRateDouble = DISABLE;    // 波特率加倍：不使用
    UART_Configuration(UART1, &UART1_8BIT_BRTX); // 初始化UART1

    NVIC_UART1_Init(ENABLE, Priority_1); // 中断使能

    UART1_SW(UART1_SW_P30_P31); // 使用P3.0 P3.1作为串口1的引脚 (默认)
}

// 判断一组数据是否接收完成
void UART1RxProcess(void)
{
    if (COM1.RX_TimeOut > 0)
    {
        if (--COM1.RX_TimeOut == 0)
        {
            UART1_RxFlag = 1;
        }
    }
}

// 将指定缓冲区中的数据依次发送出去
void UART1SendBuffer(u8 *buf, u8 len)
{
    u8 i;

    for (i = 0; i < len; i++)
    {
        TX1_write2buff(buf[i]);
    }
}

/**
 * @brief 串口1发送字符串（以 '\0' 结尾）
 * @param str 待发送的以空字符结尾的字符串
 */
void UART1_SendString(u8 *str)
{
    while (*str)
    {
        TX1_write2buff(*str++); // 逐字节写入发送缓冲
    }
}

/**
 * @brief 串口1命令处理：解析接收到的单字符命令并执行对应动作，处理后回传执行结果
 * @note 需周期性调用（与 UART1RxProcess 配合），当 UART1_RxFlag 置位表示一帧数据接收完成：
 *         - 发送 'W'/'w' → 前进        (四轮全速前进, 持续)
 *         - 发送 'S'/'s' → 后退        (四轮全速后退, 持续)
 *         - 发送 'A'/'a' → 左平移      (向左横向移动, 持续)
 *         - 发送 'D'/'d' → 右平移      (向右横向移动, 持续)
 *         - 发送 'Q'/'q' → 左转        (原地逆时针旋转, 持续)
 *         - 发送 'E'/'e' → 右转        (原地顺时针旋转, 持续)
 *         - 发送 'X'/'x' → 停止        (停止所有电机)
 * @note 取首个有效字节作为命令，执行命令后通过 UART1_SendString 回传状态（如 "BEEP1 OK\r\n"），
 *       处理完成后复位 UART1_RxFlag 与 COM1.RX_Cnt。
 *       字母移动指令调用 dirver 的 PWM 接口，持续运动直到收到新的移动/停止指令（非阻塞）。
 */
void UART1_Command(void)
{
    if (UART1_RxFlag == 0)
        return; // 未收到完整数据帧

    UART1_RxFlag = 0; // 清除接收完成标志

    if (COM1.RX_Cnt == 0)
        return; // 无有效数据

    // 取第一个字节作为命令
    switch (RX1_Buffer[0])
    {
    case 'W': // 前进
    case 'w':
        Motors_Forward(CAR_SPEED);
        UART1_SendString("FORWARD OK\r\n");
        break;

    case 'S': // 后退
    case 's':
        Motors_Backward(CAR_SPEED);
        UART1_SendString("BACKWARD OK\r\n");
        break;

    case 'A': // 左平移
    case 'a':
        Motors_Left(CAR_SPEED, 0);
        UART1_SendString("LEFT OK\r\n");
        break;

    case 'D': // 右平移
    case 'd':
        Motors_Right(CAR_SPEED, 0);
        UART1_SendString("RIGHT OK\r\n");
        break;

    case 'Q': // 左转（原地逆时针旋转）
    case 'q':
        Motors_Around(CAR_SPEED, 0);
        UART1_SendString("TURN-LEFT OK\r\n");
        break;

    case 'E': // 右转（原地顺时针旋转）
    case 'e':
        Motors_Around(CAR_SPEED, 1);
        UART1_SendString("TURN-RIGHT OK\r\n");
        break;

    case 'X': // 停止
    case 'x':
        Motors_Stop();
        UART1_SendString("STOP OK\r\n");
        break;

    default:
        UART1_SendString("UNKNOWN CMD\r\n");
        break; // 忽略未知命令
    }

    COM1.RX_Cnt = 0; // 复位接收计数，准备下一次接收
}