#ifndef BSP_UART_H
#define BSP_UART_H

extern unsigned char UART1_RxFlag;

void UART1Init(void);

void UART1RxProcess(void);
void UART1SendBuffer(u8 *buf, u8 len);

#endif // BSP_UART_H