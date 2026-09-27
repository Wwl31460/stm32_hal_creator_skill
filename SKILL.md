---
name: stm32-hal-creator
description: 用 HAL 库开发 STM32 程序（CubeMX + CMake 工程），覆盖「写代码 → 静态核对 → 编译 → ST-Link 烧录 → 运行时验证」全流程。内含经实战确认的代码风格规范（中文注释模板、对标标准库教程的结构、非阻塞定时优先）、可直接复用的外设驱动模板，以及 winget 工具链搭建与排障记录。当用户要求写 STM32/HAL 程序、新建或修改外设模块、编译烧录固件、排查 STM32 工程环境或下载问题时使用。
agent_created: true
---

# STM32 HAL 程序开发流程

适用对象：CubeMX 生成 + CMake/Ninja 构建 + ST-Link 下载的 STM32 工程（已在 STM32F103C8T6 上完整验证，换型号只需换 target 芯片型号与 OpenOCD 配置文件）。

## 五条铁律

动手前先看这五条，它们是踩过坑之后定下来的。

1. **注释一律中文**，用统一模板（见 `references/code-style.md`），写在关键处，不写长篇散文。
2. **应用层定时一律非阻塞**，禁止 `Delay_ms()` 之类的死等；只有微秒级器件时序允许短暂阻塞。
3. **模块化但不玩概念**：一个模块就是一对 `.c` + `.h`，函数直白地把事情做完。**不要用回调注册、任务池、句柄抽象**——这类框架在只有一两个外设的场合纯属负担。
4. **新增源文件加到顶层 `CMakeLists.txt`**，不要加到 CubeMX 维护的子 CMakeLists（重新生成工程会被覆盖）。
5. **烧录前静态核对，烧录后运行时验证**。只说"编译通过"是不够的。

## 工作流

| 步骤 | 关键动作 | 详细资料 |
|---|---|---|
| 1. 探环境 | 检查工具链是否齐全、ST-Link 是否连上 | `references/toolchain-setup.md` |
| 2. 写代码 | 按风格规范与现有模块模板写 | `references/code-style.md` |
| 3. 配构建 | 新增源文件登记到顶层 CMakeLists | `references/build-flash-verify.md` |
| 4. 编译 | `cmake --build --preset Debug`，必须零警告 | 同上 |
| 5. 静态核对 | `objdump` 看反汇编、`nm` 查符号 | 同上 |
| 6. 烧录 | OpenOCD `program ... verify reset exit` | 同上 |
| 7. 运行时验证 | 读 ODR / uwTick 寄存器，证明程序真在跑 | 同上 |

## 快速命令卡

已经配好环境的机器上，日常只需要这几条：

```bash
# 编译（注意换工具链后要先删 build/<preset> 再 configure）
cmake --build --preset Debug

# 烧录 + 校验 + 复位
openocd -s "$OCDSCRIPTS" -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/Debug/<工程名>.elf verify reset exit"

# 运行时采样（看 LED 有没有在翻转）
openocd -s "$OCDSCRIPTS" -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "init" -c "mdw 0x4001080C" -c "sleep 1100" -c "mdw 0x4001080C" -c "exit"
```

工具链路径变量的定义见 `references/toolchain-setup.md`。

## 参考资料

- `references/code-style.md` —— 代码风格规范、标准库↔HAL 对照表、模块模板
- `references/toolchain-setup.md` —— 环境探测、winget 安装包与路径、已知坑
- `references/build-flash-verify.md` —— CMake 配置、编译检查、烧录、运行验证
- `references/templates/` —— 可直接复制使用的模块：
  - `delay.c/h`：非阻塞定时 + 微秒延时
  - `led.c/h`：GPIO 驱动（推挽输出）
  - `oled.c/h` + `oled_font.h`：0.96 寸 SSD1306 显示屏驱动（软件模拟 I2C + 8x16 字库）

## 外设模块实战记录

**OLED（SSD1306，0.96 寸，4 脚 I2C）**
- 接线：SCL→PB8，SDA→PB9（正好是 I2C1 的硬件引脚，但代码用软件模拟，换引脚只需改头文件里的宏）
- 从机地址 `0x78`；写命令发 `0x00`，写数据发 `0x40`
- **GPIO 必须配成开漏输出 `GPIO_MODE_OUTPUT_OD`**，不能用推挽——I2C 靠线与仲裁，推挽会烧端口
- 初始化顺序有讲究：屏幕要先上电延时（约 100ms）再发初始化命令，否则不亮
- 软件模拟优先于硬件 I2C：STM32F1 的硬件 I2C 在 HAL 库里有名的不稳定，模拟实现更省心且便于排查
- 需要显示数字/十六进制/二进制时，参考原版驱动补 `OLED_Pow()` 和对应函数即可

写新外设模块的通用步骤：先找该外设的初始化命令序列与关键时序（从已有驱动或数据手册），
再把 GPIO/通信部分换成 HAL 的写法，其余逻辑照搬。
