# 编译、核对、烧录、验证

## 1. 新增源文件怎么登记

CubeMX 的 CMake 工程里有两份 CMakeLists，**必须加到顶层那份**：

```cmake
# 顶层 CMakeLists.txt —— 文件头写着 generated only once
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/Src/delay.c
    Core/Src/led.c
)
```

**不要**加到 `cmake/stm32cubemx/CMakeLists.txt` 的 `MX_Application_Src`：那份由 CubeMX 维护，
用户下次在 CubeMX 里改引脚重新 Generate，整份文件会被重建，手写的源文件列表会丢失。

头文件不用登记，`Core/Inc` 已在 CubeMX 生成的 include 路径里。

改完 CMakeLists 要删掉 `build/<preset>` 整个目录重新 configure，否则 CMake 不会重新配置。

## 2. 编译

```bash
rm -rf build/Debug          # 换工具链或改过 CMakeLists 时必须清
cmake --preset Debug
cmake --build --preset Debug
```

成功的标志不只是 `[N/N] Linking`，还要看末尾的 memory usage 输出（链接脚本配了
`-Wl,--print-memory-usage`）。**出现任何 warning 都要处理掉再往下走。**

## 3. 烧录前的静态核对

编译通过 ≠ 程序对。至少核对这三样：

```bash
cd build/Debug
arm-none-eabi-size <name>.elf                                  # 段占用
arm-none-eabi-nm <name>.elf | grep -iE "SysTick_Handler|HAL_IncTick|uwTick|<模块函数名>"
arm-none-eabi-objdump -d <name>.elf --disassemble=main          # 逐条比对
```

| 检查项 | 判据 |
|---|---|
| 延时/定时时基 | 用 `HAL_GetTick` 的方案必须能看到 `SysTick_Handler` + `HAL_IncTick` + `uwTick` 三个符号，缺一则节拍不走 |
| 引脚与端口 | `GPIO_PIN_0` → `movs r1,#1`；`GPIOA` → 立即数 `0x40010800` |
| 时长参数 | `HAL_Delay(1000)` / `Delay_Start(1000)` → `mov.w r0,#1000`（0x3e8） |
| 时钟一致性 | `stm32f1xx_hal_conf.h` 的 `HSI_VALUE`/`HSE_VALUE` 要与 `SystemClock_Config()` 实际时钟源一致，否则延时按比例失准 |
| 链接脚本 | `.ld` 里 FLASH/RAM 的 LENGTH 要匹配真实芯片（C8T6: 64K / 20K） |

## 4. 烧录 + 校验

```bash
openocd -s "$OCDSCRIPTS" -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program <绝对路径>.elf verify reset exit"
```

成功标志三连：`** Programming Finished **` / `** Verified OK **` / `** Resetting Target **`。

常用 target 配置：`stm32f1x.cfg`（F1 全系）、`stm32f4x.cfg`、`stm32g0x.cfg`、`stm32h7x.cfg`。

> grep 时注意：`verify` 匹配不到 `Verified OK`，用 `-E "Verified|Programming|Resetting"`。

## 5. 运行时验证（证明程序真在跑）

烧录成功不代表程序在正常运行。直接读硬件寄存器：

```bash
openocd -s "$OCDSCRIPTS" -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "init" -c "mdw 0x4001080C" -c "sleep 1100" -c "mdw 0x4001080C" \
  -c "sleep 1100" -c "mdw 0x4001080C" -c "exit" 2>&1 | grep "0x4001080c"
```

**关键坑**：`mdw` 和 `sleep` 必须写成**多个独立的 `-c` 参数**。
写成 `-c "init; mdw 0x...; sleep 1100; ..."` 时，后面的命令会静默不执行、拿不到结果。

寄存器地址速查（F1）：

| 寄存器 | 地址 |
|---|---|
| GPIOA_ODR | `0x4001080C` |
| GPIOA_IDR | `0x40010808` |
| GPIOx 基址 | `0x40010000 + 0x400×x`（A=0, B=1, C=2 …） |

解读结果：
- `0x0000a001` → bit0=1（PA0 高电平），bit13/bit15=1 是 PA13/PA15 的 JTAG/SWD 默认状态，正常
- 连读几次，`bit0` 在 0/1 之间交替 = LED 确实在翻转
- `uwTick` 的地址用 `nm` 查，读它在持续增长 = SysTick 节拍正常

## 6. 常见现象解释（别当成错误）

| 现象 | 说明 |
|---|---|
| `flash size = 128 KiB` 但芯片是 C8T6 | 正常。F_SIZE 寄存器在某些批次报 128K，链接脚本按 64K 编即可 |
| `device id = 0x20030410` | F1 Medium-density（`0x410`），对应 C8/CB/R8/RB，正确 |
| `Adding extra erase range` | 擦除按扇区对齐，无害 |
| `Unable to match requested speed 1000 kHz, using 950 kHz` | ST-Link 自适应降速，无害 |
| `DEPRECATED: auto-selecting transport "swd"` | 提示性信息，无害 |
