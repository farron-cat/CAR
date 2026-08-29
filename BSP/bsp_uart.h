#ifndef BSP_UART_H
#define BSP_UART_H

#include "Type_def.h" // u8 类型

extern unsigned char UART1_RxFlag;

/**
 * @brief 串口命令表项（命令分发用，支持字符串命令）
 * @note  由应用层定义命令表数组，并将数组与个数传入 UART1_ProcessCommands()，
 *        串口驱动只负责"收一帧指令 → 字符串匹配 → 回调 + 回传"，
 *        不关心具体业务。cmd 与 alias 均为以 '\0' 结尾的字符串。
 */
typedef struct
{
    const char *cmd;            // 命令行字符串全称，如 "FORWARD"
    const char *alias;          // 字符串别名，如 "FW"；无别名填 0
    void (*handler)(void);      // 执行动作（无参回调）
    const char *ack;            // 执行成功后回传的字符串，如 "FORWARD OK\r\n"
} UART1_CmdItem;

void UART1Init(void);

void UART1RxProcess(void);
void UART1SendBuffer(u8 *buf, u8 len);

void UART1_SendString(u8 *str);

/**
 * @brief 在接收帧中匹配是否命中一个字符串命令
 * @param buf 接收缓冲区（帧数据）
 * @param len 接收到的有效字节数
 * @param cmd 待匹配的命令字符串（以 '\0' 结尾）
 * @return 1 表示命中；0 表示未命中
 * @note  逐字符比较命令串，帧内命令之后剩余部分仅允许空白字符
 *        （'\r'/'\n'/空格/'\t'），可兼容串口终端发送时自带的行尾换行。
 * @note  不能仅是前缀匹配：如 "LEFTX" 不会命中 "LEFT"。
 */
u8 UART_CmdFrameMatch(const u8 *buf, u8 len, const char *cmd);

/**
 * @brief 串口1命令分发：接收一帧指令并在命令表中匹配执行
 * @param cmdTable 命令结构体数组（const，建议放代码段）
 * @param count    命令表项数
 * @note  需周期性调用（与 UART1RxProcess 配合）。当 UART1_RxFlag 置位表示一帧数据接收完成：
 *        在接收帧中做字符串匹配（含 alias），命中则调用 handler 并回传 ack；
 *        未命中回传 "UNKNOWN CMD\r\n"。处理完成后复位 UART1_RxFlag 与 COM1.RX_Cnt。
 */
void UART1_ProcessCommands(const UART1_CmdItem *cmdTable, u8 count);

#endif // BSP_UART_H
