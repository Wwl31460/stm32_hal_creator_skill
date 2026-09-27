#ifndef __DELAY_H
#define __DELAY_H

#include <stdint.h>

void Delay_Init(void);			//延时初始化
void Delay_us(uint32_t us);		//微秒延时，用于器件时序

void Delay_Start(uint32_t ms);		//启动一次毫秒定时，后台计时不占用CPU
uint8_t Delay_IsElapsed(void);		//检查定时是否到点

#endif
