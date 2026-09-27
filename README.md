# STM32 HAL Creator Skill

一个给 AI 编码助手（WorkBuddy / Claude Code 等）用的 Skill：用 HAL 库开发 STM32 程序，
并串起「写代码 → 静态核对 → 编译 → ST-Link 烧录 → 运行时验证」的完整链路。

内容不是在桌面上空想出来的，而是在真实板子上从零跑通过一遍后整理的——
包括工具链从无到有的搭建、踩过的每一个坑、以及反复打磨了几轮的代码风格。

已在 **STM32F103C8T6 + ST-LINK/V2** 上完整验证（LED 每 1 秒翻转，烧录后实测寄存器电平确实在跳变）。

## 它解决什么问题

让 AI 写单片机程序，通常会出现三类毛病：

1. **环境就跑不起来** —— 机器上没工具链，AI 却假设有；
2. **代码风格飘忽** —— 今天长注释明天短注释，看的人抓不住重点；
3. **写完不验证** —— 一句"编译通过，应该没问题"就交差，结果灯根本没亮。

这个 Skill 针对这三点给出明确规则，其中最核心的一条是：
**烧录前必须静态核对（反汇编逐条比对），烧录后必须运行时验证（读硬件寄存器证明程序真在跑）。**

## 目录结构

```
stm32_hal_creator_skill/
├── SKILL.md                          # 主入口：五条铁律 + 七步工作流 + 命令速查
├── README.md                          # 本文件
└── references/
    ├── code-style.md                 # 代码风格规范、标准库↔HAL 对照表
    ├── toolchain-setup.md            # 环境探测、winget 安装包与路径、已知坑
    ├── build-flash-verify.md         # CMake 配置、编译核对、烧录、运行验证
    └── templates/                    # 可直接复制使用的模块
        ├── delay.c / delay.h         # 非阻塞定时 + 微秒延时
        └── led.c / led.h             # LED 驱动
```

`SKILL.md` 保持精简便于加载，细节放在 `references/` 里按需读取。

## 怎么安装

把整个仓库内容放到你的 skills 目录下即可，例如：

```bash
# WorkBuddy
cp -r . ~/.workbuddy/skills/stm32-hal-creator/

# Claude Code
cp -r . ~/.claude/skills/stm32-hal-creator/
```

只要 `SKILL.md` 的 frontmatter 里 `name` 与目录名一致，多数工具都能自动识别。

## 几条你会关心的约定

**◇ 注释规范**
统一用三行块（`/@brief/@param/@retval`）+ 行尾短注释，中文，写"为什么"而不是复述代码。

**◇ 定时一律非阻塞**
禁止 `Delay_ms()` 死等。参考示例里那种阻塞写法会把整个主程序卡死，
按键没人理、串口丢数据。改成查询式：`Delay_Start(1000)` 启动，`Delay_IsElapsed()` 查到点。
只有微秒级器件时序（DHT11 / DS18B20 之类）才允许短暂阻塞，那也是硬件要求躲不开。

**◇ 模块化但不玩概念**
一个模块就是一对 `.c` + `.h`。**刻意不用**回调注册、任务池那套框架——
在只有一两个外设的场景里，那些东西除了让人看不懂，没有任何收益。

**◇ 工具链全用免费开源方案**
Arm GNU Toolchain + CMake + Ninja + OpenOCD，全部 winget 直接装，不需要装几个 G 的官方 IDE。

## 环境要求

- Windows + winget（用于安装工具链）
- ST-Link V2 / V2.1 / V3
- CubeMX 生成的 CMake 工程

工具的具体版本和踩坑记录见 `references/toolchain-setup.md`。
