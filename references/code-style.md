# 代码风格规范

规范来源是用户认可的一份 STM32 标准库示例工程（`3-1 LED闪烁`），取其**结构清晰**的优点，
同时改掉了它阻塞式延时的缺点。照做即可，不必每次问。

## 注释模板

**函数头统一三行块，中文，一行说清干什么。**

```c
/**
  * @brief 一句话说清这个函数干什么
  * @param 参数名 说明（无参数写"无"）
  * @retval 返回值说明（无返回写"无"）
  */
void Func(void)
{
    操作;        // 行尾短注释，紧跟代码
}
```

要点：
- 行尾注释用 `//中文`，Tab 缩进，与代码间留出对齐间隔
- 注释写"为什么这么做 / 这行是什么"，**不要复述代码本身**（`i++` 后面写"自增"就是废话）
- 没有解释价值的地方不写，别为了凑注释而注释

## 主程序结构

**一眼望到底是最高原则。** 初始化几行，然后一个 `while(1)`，里面直接写动作。

```c
/**
  * @brief 主函数：LED每隔1秒翻转一次（亮1秒、灭1秒）
  * @param 无
  * @retval 无
  */
int main(void)
{
    HAL_Init();              //HAL库初始化
    SystemClock_Config();    //系统时钟配置

    LED_Init();              //LED初始化
    Delay_Init();            //延时初始化
    Delay_Start(1000);       //启动1秒定时，后台计时中

    while (1)                //主死循环
    {
        if (Delay_IsElapsed())   //检查定时是否到点
        {
            LED1_T();            //翻转LED
            Delay_Start(1000);   //重新启动1秒定时
        }
    }
}
```

对应的标准库写法是 `if (KEY1_R() == 0) { LED1_W(0); } else { LED1_W(1); }`，
结构完全同构：`while(1)` 里一个 `if(条件)`，里面放动作。**保持这种形似度。**

## 精简：该删就删

CubeMX 生成的模板文件里，这些对运行毫无影响，清掉：
- ST 版权 banner 与 `@attention` 段落
- `/* Private typedef ----*/` 这类装饰性分隔线
- 所有**空的** `USER CODE BEGIN/END` 块
- 没有被调用的工具函数（删除前先 grep 确认无外部引用）
- 不必要的返回值检查与 `Error_Handler` 之类的兜底函数（教学示例不需要）

**但要保留有实际内容的 `USER CODE` 注释对**（Includes / PD / PV / 2 / WHILE）。
它们是 CubeMX 识别"哪些代码是用户写的"的锚点，删掉后重新生成工程会冲掉自定义代码。

## 定时必须非阻塞

```c
void    Delay_Start(uint32_t ms);   // 记录起点后立刻返回，后台自行计时
uint8_t Delay_IsElapsed(void);      // 查询是否到点
```

`Delay_ms(1000)` 会卡死整整一秒——期间按键没人管、串口数据丢失。换成查询式之后，
主循环一秒转几十万圈，每圈只花几微秒问一句"到点没"，其余时间 CPU 全是空的。

**例外**：`Delay_us()` 微秒延时可以保留。读写 DHT11 / DS18B20 这类传感器时，
拉低总线后必须等几十到几百微秒才能读数据，这是硬件时序要求，绕不过去，且耗时极短。

实现见 `templates/delay.c`，时基用 HAL 自带的 `HAL_GetTick()`，判断用减法避免溢出回零算错。

## 标准库 → HAL 对照表

给学过标准库的人快速上手用：

| 标准库 | HAL |
|---|---|
| `RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE)` | `__HAL_RCC_GPIOA_CLK_ENABLE()` |
| `GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP` | `GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP` |
| `.GPIO_Pin = GPIO_Pin_0` | `.Pin = GPIO_PIN_0` |
| `.GPIO_Speed = GPIO_Speed_50MHz` | `.Speed = GPIO_SPEED_FREQ_LOW` |
| `GPIO_Init(GPIOA, &GPIO_InitStructure)` | `HAL_GPIO_Init(GPIOA, &GPIO_InitStructure)` |
| `GPIO_WriteBit(GPIOA, GPIO_Pin_0, BitAction)` | `HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PinState)` |
| `GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0)` | `HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0)` |
| `GPIO_SetBits / GPIO_ResetBits` | `HAL_GPIO_WritePin(…, GPIO_PIN_SET / GPIO_PIN_RESET)` |
| `GPIO_Mode_IPU`（上拉输入） | `GPIO_MODE_INPUT` + `.Pull = GPIO_PULLUP` |

规律：HAL 把结构体成员名里的 `GPIO_` 前缀去掉了，枚举值全大写加下划线。

## 引脚操作用宏

和参考风格一致，把硬件连接集中到头文件顶部的宏，换板子只改这几行：

```c
/* 引脚宏定义：LED1→PA0（高电平点亮），修改接线只需改这里 */
#define LED1_W(x)    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, (GPIO_PinState)(x))  //写操作时写1点亮
#define LED1_T()     HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0)                     //翻转电平
```

## 模块的边界怎么划

原则：**能自包含就自包含**，即 `.h` 里定义引脚、`.c` 里完成初始化，别的工程拷过去就能用。

不要做的事：
- 不要让模块反过来依赖 `main.h` 里的 CubeMX 宏
- 不要用回调、任务登记表、动态注册这类机制——除非调用方能明显受益
- 不要为了"看起来专业"多套一层间接调用

可以做的：模块内部保留硬件抽象（引脚宏），对外暴露直白的函数。
