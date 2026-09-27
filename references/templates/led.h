#ifndef __LED_H
#define __LED_H

#include "stm32f1xx_hal.h"

/* 引脚宏定义：LED1→PA0（高电平点亮），修改接线只需改这里 */
#define LED1_W(x)	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, (GPIO_PinState)(x))	//LED1写操作，写1点亮
#define LED1_T()	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0)					//LED1电平翻转

void LED_Init(void);	//LED引脚初始化（PA0推挽输出）

#endif
