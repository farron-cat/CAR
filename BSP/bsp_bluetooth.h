#ifndef __BSP_BLUETOOTH_H__
#define __BSP_BLUETOOTH_H__

#define BT_EN P46    // 蓝牙使能引脚（P4.6），输出
#define BT_STATE P06 // 蓝牙状态引脚（P0.6），输入
#define BT_RXD P10   // UART2 RxD（P1.0）
#define BT_TXD P11   // UART2 TxD（P1.1）

void BT_Init(void); // 初始化蓝牙模块（GPIO + UART2）

#endif