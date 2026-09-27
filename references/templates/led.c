#include "led.h"

/**
  * @brief LED引脚初始化（PA0推挽输出）
  * @param 无
  * @retval 无
  */
void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	__HAL_RCC_GPIOA_CLK_ENABLE();						//开启GPIOA时钟

	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;		//推挽输出
	GPIO_InitStructure.Pin = GPIO_PIN_0;				//PA0引脚
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;		//低速，仅用于开关LED
	GPIO_InitStructure.Pull = GPIO_NOPULL;				//输出模式，上下拉无作用
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);			//初始化GPIOA

	LED1_W(0);											//初始熄灭
}
