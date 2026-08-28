#ifndef BSP_UART_H
#define BSP_UART_H

// 小车移动速度（0~100，越大越快），供串口单字母指令使用
#define CAR_SPEED 70

extern unsigned char UART1_RxFlag;

void UART1Init(void);

void UART1RxProcess(void);
void UART1SendBuffer(u8 *buf, u8 len);

void UART1_SendString(u8 *str);

void UART1_Command(void);

#endif // BSP_UART_H