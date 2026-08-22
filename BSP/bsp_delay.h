#ifndef BSP_DELAY_H
#define BSP_DELAY_H

void Delay25ms(void);
void Delay50ms(void);
void Delay200ms(void);
void Delay500ms(void);
void Delay600ms(void);
void Delay1000ms(void);
void Delay2000ms(void);
// void delay_ms(unsigned int ms);
void delay_us(unsigned int us);

void Delay956us(void); // Do 半周期延时
void Delay478us(void); // 高音Do 半周期延时

void Delay852us(void); // Re 半周期延时
void Delay426us(void); // 高音Re 半周期延时

void Delay759us(void); // Mi 半周期延时
void Delay383us(void); // 高音Mi 半周期延时

void Delay716us(void); // Fa 半周期延时
void Delay347us(void); // 高音Fa 半周期延时

void Delay638us(void); // So 半周期延时
void Delay311us(void); // 高音So 半周期延时

void Delay568us(void); // La 半周期延时
void Delay276us(void); // 高音La 半周期延时

void Delay506us(void); // Si 半周期延时
void Delay244us(void); // 高音Si 半周期延时

#endif // BSP_DELAY_H