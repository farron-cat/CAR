#ifndef BSP_UART_H
#define BSP_UART_H

extern unsigned char UART1_RxFlag;

/**
 * @brief 串口1单字符命令表项（命令分发用）
 * @note  由应用层定义命令表数组，并将数组与个数传入 UART1_ProcessCommands()，
 *        串口驱动只负责"收一帧指令 → 匹配命令 → 回调 + 回传"，不关心具体业务。
 */
typedef struct
{
    char cmd;               // 触发字符，如 'W'
    char alias;             // 别名，如 'w'；无别名填 '\0'
    void (*handler)(void);  // 执行动作（无参回调）
    const char *ack;        // 执行成功后回传的字符串，如 "FORWARD OK\r\n"
} UART1_CmdItem;

void UART1Init(void);

void UART1RxProcess(void);
void UART1SendBuffer(u8 *buf, u8 len);

void UART1_SendString(u8 *str);

/**
 * @brief 串口1命令分发：接收一帧指令并在命令表中匹配执行
 * @param cmdTable 命令结构体数组（const，建议放代码段）
 * @param count    命令表项数
 * @note  需周期性调用（与 UART1RxProcess 配合）。当 UART1_RxFlag 置位表示一帧数据接收完成：
 *        取首个有效字节作为命令，遍历命令表匹配 cmd/alias，命中则调用 handler 并回传 ack；
 *        未命中回传 "UNKNOWN CMD\r\n"。处理完成后复位 UART1_RxFlag 与 COM1.RX_Cnt。
 */
void UART1_ProcessCommands(const UART1_CmdItem *cmdTable, u8 count);

#endif // BSP_UART_H