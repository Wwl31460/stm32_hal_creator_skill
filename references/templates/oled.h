#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>

void OLED_Init(void);										//OLED初始化
void OLED_Clear(void);										//OLED清屏
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);	//显示一个字符
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);	//显示字符串

#endif
