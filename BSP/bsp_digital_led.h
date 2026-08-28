#ifndef BSP_DIGITAL_LED_H
#define BSP_DIGITAL_LED_H

#define SER P44
#define RCLK P43
#define SRCLK P42

#define SEG_CLR 0xFF

#define SEG_0 0xC0
#define SEG_1 0xF9
#define SEG_2 0xA4
#define SEG_3 0xB0
#define SEG_4 0x99
#define SEG_5 0x92
#define SEG_6 0x82
#define SEG_7 0xF8
#define SEG_8 0x80
#define SEG_9 0x90

#define SEG_A 0x88
#define SEG_B 0x83
#define SEG_C 0xC6
#define SEG_D 0xA1
#define SEG_E 0x86
#define SEG_F 0x8E

#define SEG_R 0xCE
#define SEG_S 0x92
#define SEG_O 0x8C
#define SEG_N 0xAB
#define SEG_L 0xC7
#define SEG_H 0x89
#define SEG_G 0x82

#define DIG_1 0x01
#define DIG_2 0x02
#define DIG_3 0x04
#define DIG_4 0x08
#define DIG_5 0x10
#define DIG_6 0x20
#define DIG_7 0x40
#define DIG_8 0x80

// 段码表
extern unsigned char code segCodeTable[16];
// 位码表
extern unsigned char code digCodeTable[8];
// 数码管显示缓冲（8位，段码值），由Timer1中断扫描刷新
extern unsigned char displayBuffer[8];

void DigitalLEDInit(void);
void SendByte(unsigned char dat);
void SendCode(unsigned char segCode, unsigned char digCode);
void DisplayNumber(unsigned long int num);

#endif // BSP_DIGITAL_LED_H