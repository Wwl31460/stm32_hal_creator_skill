#include "oled.h"
#include "oled_font.h"
#include "delay.h"
#include "stm32f1xx_hal.h"

/* 引脚配置：SCL→PB8，SDA→PB9，修改接线只需改这里 */
#define OLED_W_SCL(x)	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, (GPIO_PinState)(x))	//SCL时钟线写操作
#define OLED_W_SDA(x)	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, (GPIO_PinState)(x))	//SDA数据线写操作

#define OLED_ADDR		0x78	//OLED的I2C从机地址

/**
  * @brief I2C引脚初始化（PB8、PB9开漏输出）
  * @param 无
  * @retval 无
  * @note  I2C总线要求开漏输出配合外部上拉，不能用推挽
  */
static void OLED_I2C_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	__HAL_RCC_GPIOB_CLK_ENABLE();					//开启GPIOB时钟

	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;	//开漏输出
	GPIO_InitStructure.Pin = GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;	//模拟时序翻转频繁，用高速档
	GPIO_InitStructure.Pull = GPIO_NOPULL;			//外部已有上拉电阻
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

	OLED_W_SCL(1);									//两根线都拉高，总线处于空闲状态
	OLED_W_SDA(1);
}

/**
  * @brief I2C起始信号
  * @param 无
  * @retval 无
  */
static void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_W_SDA(0);		//SCL为高时SDA出现下降沿，即为起始条件
	OLED_W_SCL(0);
}

/**
  * @brief I2C停止信号
  * @param 无
  * @retval 无
  */
static void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);
	OLED_W_SDA(1);		//SCL为高时SDA出现上升沿，即为停止条件
}

/**
  * @brief I2C发送一个字节
  * @param Byte 要发送的字节
  * @retval 无
  */
static void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(!!(Byte & (0x80 >> i)));		//从最高位开始逐位送到SDA
		OLED_W_SCL(1);							//拉高时钟，从机在这一刻采样
		OLED_W_SCL(0);
	}
	OLED_W_SCL(1);		//额外一个时钟用于跳过应答位
	OLED_W_SCL(0);
}

/**
  * @brief OLED写命令
  * @param Command 要写入的命令字
  * @retval 无
  */
static void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(OLED_ADDR);	//从机地址
	OLED_I2C_SendByte(0x00);		//控制字节，00表示接下来是命令
	OLED_I2C_SendByte(Command);
	OLED_I2C_Stop();
}

/**
  * @brief OLED写数据
  * @param Data 要写入的数据
  * @retval 无
  */
static void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(OLED_ADDR);	//从机地址
	OLED_I2C_SendByte(0x40);		//控制字节，40表示接下来是显存数据
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

/**
  * @brief OLED设置光标位置
  * @param Y 页地址，范围：0~7
  * @param X 列地址，范围：0~127
  * @retval 无
  */
static void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//设置页地址
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置列地址高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置列地址低4位
}

/**
  * @brief OLED清屏
  * @param 无
  * @retval 无
  */
void OLED_Clear(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j++)			//128x64的屏共8页，每页8行像素
	{
		OLED_SetCursor(j, 0);
		for (i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);	//写入0即熄灭对应像素
		}
	}
}

/**
  * @brief OLED显示一个字符
  * @param Line 行位置，范围：1~4
  * @param Column 列位置，范围：1~16
  * @param Char 要显示的字符，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//光标移到字符上半部分所在页
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//显示上半部分
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//光标移到下半部分所在页
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分
	}
}

/**
  * @brief OLED显示字符串
  * @param Line 起始行位置，范围：1~4
  * @param Column 起始列位置，范围：1~16
  * @param String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

/**
  * @brief OLED初始化
  * @param 无
  * @retval 无
  */
void OLED_Init(void)
{
	Delay_us(100000);	//上电延时，等待屏幕内部复位完成（此处仅初始化时等待一次）

	OLED_I2C_Init();	//引脚初始化

	OLED_WriteCommand(0xAE);	//关闭显示

	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);

	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);	//64行

	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);

	OLED_WriteCommand(0x40);	//设置显示起始行

	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1为正常方向

	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8为正常方向

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);

	OLED_WriteCommand(0x81);	//设置对比度
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH电平
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//显示内容跟随显存

	OLED_WriteCommand(0xA6);	//正常显示，1为点亮

	OLED_WriteCommand(0x8D);	//开启电荷泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//开启显示

	OLED_Clear();				//清屏
}
