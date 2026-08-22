#include "STC8G_H_GPIO.h"
#include "STC8G_H_ADC.h"
#include "STC8G_H_NVIC.h"

// NTC电阻-温度对照表（RT对照表）
// 注意u16最大值是65535，所以所有的阻值保留2位小数X100
unsigned int code temp_table[] = {
    58354, // -55		// [0]
    55464, // -54		// [1]
    52698, // -53
    50048, // -52
    47515, // -51
    45097, // -50
    42789, // -49
    40589, // -48
    38492, // -47
    36496, // -46
    34597, // -45
    32791, // -44
    31075, // -43
    29444, // -42
    27896, // -41
    26427, // -40
    25034, // -39
    23713, // -38
    22460, // -37
    21273, // -36
    20148, // -35
    19083, // -34
    18075, // -33
    17120, // -32
    16216, // -31
    15361, // -30
    14551, // -29
    13785, // -28
    13061, // -27
    12376, // -26
    11728, // -25
    11114, // -24
    10535, // -23
    9986,  // -22
    9468,  // -21
    8977,  // -20
    8513,  // -19
    8075,  // -18
    7660,  // -17
    7267,  // -16
    6896,  // -15
    6545,  // -14
    6212,  // -13
    5898,  // -12
    5601,  // -11
    5319,  // -10
    5053,  // -9
    4801,  // -8
    4562,  // -7
    4336,  // -6
    4122,  // -5
    3920,  // -4
    3728,  // -3
    3546,  // -2
    3374,  // -1
    3211,  // 0
    3057,  // 1
    2910,  // 2
    2771,  // 3
    2639,  // 4
    2515,  // 5
    2396,  // 6
    2284,  // 7
    2177,  // 8
    2076,  // 9
    1978,  // 10
    1889,  // 11
    1802,  // 12
    1720,  // 13
    1642,  // 14
    1568,  // 15
    1497,  // 16
    1430,  // 17
    1366,  // 18
    1306,  // 19
    1248,  // 20
    1193,  // 21
    1141,  // 22
    1092,  // 23
    1044,  // 24
    1000,  // 25
    957,   // 26
    916,   // 27
    877,   // 28
    840,   // 29
    805,   // 30
    771,   // 31
    739,   // 32
    709,   // 33
    679,   // 34
    652,   // 35
    625,   // 36
    600,   // 37
    576,   // 38
    552,   // 39
    530,   // 40
    509,   // 41
    489,   // 42
    470,   // 43
    452,   // 44
    434,   // 45
    417,   // 46
    401,   // 47
    386,   // 48
    371,   // 49
    358,   // 50
    344,   // 51
    331,   // 52
    318,   // 53
    306,   // 54
    295,   // 55
    284,   // 56
    274,   // 57
    264,   // 58
    254,   // 59
    245,   // 60
    236,   // 61
    228,   // 62
    220,   // 63
    212,   // 64
    205,   // 65
    198,   // 66
    191,   // 67
    184,   // 68
    178,   // 69
    172,   // 70
    166,   // 71
    160,   // 72
    155,   // 73
    150,   // 74
    145,   // 75
    140,   // 76
    135,   // 77
    131,   // 78
    126,   // 79
    122,   // 80
    118,   // 81
    115,   // 82
    111,   // 83
    107,   // 84
    104,   // 85
    101,   // 86
    97,    // 87
    94,    // 88
    91,    // 89
    89,    // 90
    86,    // 91
    83,    // 92
    81,    // 93
    78,    // 94
    76,    // 95
    74,    // 96
    71,    // 97
    69,    // 98
    67,    // 99
    65,    // 100
    63,    // 101
    61,    // 102
    60,    // 103
    58,    // 104
    56,    // 105
    55,    // 106
    53,    // 107
    52,    // 108
    50,    // 109
    49,    // 110
    47,    // 111
    46,    // 112
    45,    // 113
    43,    // 114
    42,    // 115
    41,    // 116
    40,    // 117
    39,    // 118
    38,    // 119
    37,    // 120
    36,    // 121
    35,    // 122
    34,    // 123
    33,    // 124
    32,    // 125
};

// ADC初始化，包含电位器电压和NTC热敏电阻对应的GPIO初始化
void ADCInit(void)
{
    // 定义初始化结构体
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    // P0.5 设置为高阻输入 电位器 channel 13
    // P0.4 设置为高阻输入 NTC热敏电阻 channel 12
    GPIO_InitStructure.Pin = GPIO_Pin_5 | GPIO_Pin_4;
    GPIO_InitStructure.Mode = GPIO_HighZ;
    GPIO_Inilize(GPIO_P0, &GPIO_InitStructure);

    // 初始化ADC
    ADC_InitStructure.ADC_SMPduty = 31;                    // ADC 模拟信号采样时间控制, 0~31（注意： SMPDUTY 一定不能设置小于 10）
    ADC_InitStructure.ADC_CsSetup = 0;                     // ADC 通道选择时间控制 0(默认),1
    ADC_InitStructure.ADC_CsHold = 1;                      // ADC 通道选择保持时间控制 0,1(默认),2,3
    ADC_InitStructure.ADC_Speed = ADC_SPEED_2X16T;         // 设置 ADC 工作时钟频率	ADC_SPEED_2X1T~ADC_SPEED_2X16T
    ADC_InitStructure.ADC_AdjResult = ADC_RIGHT_JUSTIFIED; // ADC结果调整,	ADC_LEFT_JUSTIFIED,ADC_RIGHT_JUSTIFIED
    ADC_Inilize(&ADC_InitStructure);                       // 初始化
    ADC_PowerControl(ENABLE);                              // ADC电源开关, ENABLE或DISABLE
    NVIC_ADC_Init(DISABLE, Priority_0);                    // 中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
}

// 读取电位器电压 1.7V~2.5V
float PotReadVoltage(void)
{
    unsigned int adcCode = 0;
    float voltage = 0.0f;

    adcCode = Get_ADCResult(ADC_CH13);     // 注意通道选择 channel 13 P0.5 (查表得到)
    voltage = (float)adcCode * 2.5 / 4096; // 2.5V是参考电压，4096是12位分辨率

    return voltage;
}

// 读取NTC热敏电阻电压
float NTCReadVoltage(void)
{
    unsigned int adcCode = 0;
    float voltage = 0;

    adcCode = Get_ADCResult(ADC_CH12);     // 注意通道选择 channel 12 P0.4 (查表得到)
    voltage = (float)adcCode * 2.5 / 4096; // 2.5V是参考电压，4096是12位分辨率

    return voltage;
}

// 读取NTC热敏电阻阻值,单位kΩ
float NTCReadResistance(void)
{
    float voltage = 0;
    float resistance = 0;

    voltage = NTCReadVoltage();
    resistance = 10 * voltage / (3.3 - voltage); // 10kΩ是NTC热敏电阻分压电阻

    return resistance;
}

// 求绝对值
static float absoluteValue(float num)
{
    if (num < 0)
    {
        return -num;
    }

    return num;
}

// 查找温度表
static int search_temp(float res)
{
    int i;
    int index = 0;
    int len;
    float min;  // 最小差值
    float diff; // 差值

    len = sizeof(temp_table) / sizeof(temp_table[0]); // 数组长度
    min = absoluteValue(res - temp_table[0]);

    for (i = 1; i < len; i++)
    {
        diff = absoluteValue(res - temp_table[i]);

        if (diff < min)
        {
            min = diff;
            index = i;
        }
    }

    return index;
}

// 读取NTC热敏电阻温度,单位℃
int NTCReadTemperature(void)
{
    int temp = 0;
    float resistance = NTCReadResistance();
    temp = search_temp(resistance * 100) - 55;
    return temp;
}