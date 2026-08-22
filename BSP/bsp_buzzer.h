#ifndef BSP_BUZZER_H
#define BSP_BUZZER_H

#define BUZZER P00

void BuzzerInit(void);

// ============ 非阻塞播放（推荐使用） =============
// 使用Timer2中断产生音符方波，不阻塞主循环，按键等任务可正常响应
void Buzzer_ToneStart(unsigned int halfPeriodUs); // 以指定半周期(us)开始发声
void Buzzer_ToneOff(void);                        // 停止发声
void Buzzer_ToneIsr(void);                        // Timer2中断服务（翻转引脚）
void LittleStar_Start(void);                      // 开始播放小星星（非阻塞）
void LittleStar_Task(void);                       // 主循环周期性调用推进播放
unsigned char LittleStar_Playing(void);           // 查看是否正在播放

void Do(void); // 播放Do音（400Hz）
void Re(void); // 播放Re音（400ms）
void Mi(void); // 播放Mi音（400ms）
void Fa(void); // 播放Fa音（400ms）
void So(void); // 播放So音（400ms）
void La(void); // 播放La音 (400ms)
void Si(void); // 播放Si音 (400ms)

void Do_h(void); // 播放Do高音（400ms）
void Re_h(void); // 播放Re高音（400ms）
void Mi_h(void); // 播放Mi高音（400ms）
void Fa_h(void); // 播放Fa高音（400ms）
void So_h(void); // 播放So高音（400ms）
void La_h(void); // 播放La高音 (400ms)
void Si_h(void); // 播放Si高音 (400ms)

void Buzzer_Off(void);

void LittleStar();

#endif // BSP_BUZZER_H