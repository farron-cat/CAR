#ifndef BSP_ADC_H_
#define BSP_ADC_H_

void ADCInit(void);
float PotReadVoltage(void);
float NTCReadVoltage(void);
float NTCReadResistance(void);
int NTCReadTemperature(void);

#endif // BSP_ADC_H_