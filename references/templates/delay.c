#include "delay.h"
#include "stm32f1xx_hal.h"

static uint32_t s_lastTick;		//上次启动定时的时刻
static uint32_t s_periodMs;		//定时时长

/**
  * @brief 延时初始化，开启内核的时钟周期计数器
  * @param 无
  * @retval 无
  */
void Delay_Init(void)
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;	//允许使用DWT
	DWT->CYCCNT = 0;								//计数器清零
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;			//启动计数，此后每1个时钟周期加1
}

/**
  * @brief 微秒延时
  * @param us 延时时长，单位微秒
  * @retval 无
  * @note  等待期间不放开CPU，仅用于读写传感器等器件时序，长时间等待请用下一组非阻塞定时
  */
void Delay_us(uint32_t us)
{
	uint32_t start = DWT->CYCCNT;							//记录起始计数值
	uint32_t cnt = us * (SystemCoreClock / 1000000U);		//换算成需要的时钟周期数

	//减法写法，可避免计数器溢出回零时算错
	while ((uint32_t)(DWT->CYCCNT - start) < cnt);
}

/**
  * @brief 启动一次毫秒定时
  * @param ms 定时时长，单位毫秒
  * @retval 无
  * @note  调用后立即返回，之后由后台自行计时，期间CPU可处理其他事情
  */
void Delay_Start(uint32_t ms)
{
	s_lastTick = HAL_GetTick();		//记录启动时刻
	s_periodMs = ms;				//保存定时时长
}

/**
  * @brief 检查定时是否到点
  * @param 无
  * @retval 1：已到点
  * @retval 0：未到点
  * @note  到点后需再次调用Delay_Start，才能开始下一轮定时
  */
uint8_t Delay_IsElapsed(void)
{
	//用减法而非比较大小，可避免节拍计数溢出回零时算错
	if ((uint32_t)(HAL_GetTick() - s_lastTick) >= s_periodMs)
	{
		return 1;
	}
	return 0;
}
