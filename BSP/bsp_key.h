#ifndef BSP_KEY_H
#define BSP_KEY_H

#define KEY_1 P51
#define KEY_2 P52
#define KEY_3 P53
#define KEY_4 P54

#define KEY_C P32

#define NO_KEY_DOWN -1
#define KEY1_DOWN 1
#define KEY2_DOWN 2
#define KEY3_DOWN 3
#define KEY4_DOWN 4

void KeyInit();
void KeyCInit();
void KeyCINTInit();

#endif // BSP_KEY_H