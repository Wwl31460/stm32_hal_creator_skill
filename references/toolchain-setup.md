# 工具链环境

一台新机器上大概率什么都没有。先探测，再决定要不要装。

## 1. 探测

```bash
# 1) 工具链
for c in cmake ninja arm-none-eabi-gcc openocd STM32_Programmer_CLI; do
  printf "%-22s %s\n" "$c" "$(command -v $c || echo 'MISSING')"
done

# 2) 是否有官方套装
ls -d "/c/ST" "/c/Program Files/STMicroelectronics" 2>/dev/null
```

全 MISSING 且 `C:\ST` 不存在 → 这台机器没有任何 ARM 工具链，直接告诉用户，别硬试。

> 注意：`reg.exe` 在本机安全策略黑名单里，查已装软件不要用 `reg query`。
> PowerShell 工具的 stdout 有时返回空，优先用 Bash 配 `cmd //c`。

## 2. 检查 ST-Link 是否插好

```bash
cmd //c "pnputil /enum-devices /connected" 2>&1 | iconv -f GBK -t UTF-8 | grep -aiE "0483|ST-Link|STMicro"
```

必须转码（`iconv -f GBK`），否则 Windows 的中文输出会让 grep 误判为二进制文件而拒搜。

| USB PID | 型号 |
|---|---|
| 0483:3748 | ST-LINK/V2 |
| 0483:374B | ST-LINK/V2.1（常见于 Nucleo 板载） |
| 0483:374E / 3753 | ST-LINK/V3 |

## 3. 安装（winget，全部免费，不弹 UAC）

```bash
winget install -e --id Arm.GnuArmEmbeddedToolchain  --accept-package-agreements --accept-source-agreements --disable-interactivity
winget install -e --id Kitware.CMake                --accept-package-agreements --accept-source-agreements --disable-interactivity
winget install -e --id Ninja-build.Ninja            --accept-package-agreements --accept-source-agreements --disable-interactivity
winget install -e --id xpack-dev-tools.openocd-xpack --accept-package-agreements --accept-source-agreements --disable-interactivity
```

搜索包名用 `winget search "<关键词>"`；`--id` 精确。winget search 偶有超时，多试或加参数。

## 4. 已知坑（逐个踩过）

**坑 1｜ARM 编译器包名大小写刁钻**
是 `Arm.GnuArmEmbeddedToolchain`（`GNU` 写成 `Gnu`）。
写成 `Arm.GNU.ArmEmbeddedToolchain` 或含 `-dev-tools` 的变体都搜不到。用 `winget search "GNU Arm Embedded"` 确认。

**坑 2｜Ninja 的 winget 别名不可执行**
winget 会在 `%LOCALAPPDATA%\Microsoft\WinGet\Links\ninja.exe` 放一个 App Exec 别名，
CMake 调用它时报 `inappropriate file type or format`。**必须用 Packages 目录下的真实文件。**

**坑 3｜装完 PATH 不生效**
winget 修改了用户 PATH，但当前 shell 不会更新。用绝对路径，或者说明"重启 VSCode/终端"。

**坑 4｜reg.exe 被禁**
本机安全策略把 reg.exe 列入黑名单，查询注册表会直接被拦截，不要尝试绕过。

**坑 5｜VSCode 的 STM32Cube 扩展 bundle 是空的**
扩展目录里 `resources/binaries/win32` 为空说明工具没下载。
若 `.vscode/settings.json` 里写着 `"cmake.cmakePath": "cube-cmake"`，构建必然失败。
解决办法：改成指向本机已装工具链的绝对路径，原文件备份成 `.bak`。

## 5. 路径速查（本次实测）

```bash
export PATH="/c/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin:$PATH"
export PATH="/c/Program Files/CMake/bin:$PATH"
export NINJA_DIR="/c/Users/$USERNAME/AppData/Local/Microsoft/WinGet/Packages/Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe"
export PATH="$NINJA_DIR:$PATH"
export OCD="/c/Users/$USERNAME/AppData/Local/Microsoft/WinGet/Packages/xpack-dev-tools.openocd-xpack_Microsoft.Winget.Source_8wekyb3d8bbwe/xpack-openocd-0.12.0-7/bin/openocd.exe"
export OCDSCRIPTS="/c/Users/$USERNAME/AppData/Local/Microsoft/WinGet/Packages/xpack-dev-tools.openocd-xpack_Microsoft.Winget.Source_8wekyb3d8bbwe/xpack-openocd-0.12.0-7/openocd/scripts"
```

版本号会变，装完用 find 确认：

```bash
find "/c/Program Files" "/c/Program Files (x86)" "/c/Users/$USERNAME/AppData/Local/Microsoft/WinGet/Packages" \
  -maxdepth 6 -iname "*.exe" 2>/dev/null | grep -iE "gcc|cmake|ninja|openocd"
```
