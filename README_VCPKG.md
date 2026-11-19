# AServer vcpkg 编译指南

第三方库统一由 vcpkg 管理，清单见 [vcpkg.json](vcpkg.json)。

## 1. 安装 vcpkg（Windows）

如果本机尚未安装：

```bat
git clone --depth 1 https://github.com/microsoft/vcpkg.git D:\vcpkg
D:\vcpkg\bootstrap-vcpkg.bat -disableMetrics
setx VCPKG_ROOT D:\vcpkg
```

`setx` 设置的是用户级全局环境变量，重新打开终端后生效。CMake 会在 `project()` 之前读取 `VCPKG_ROOT` 并设置 `CMAKE_TOOLCHAIN_FILE`。

## 2. 编译（Windows）

以管理员身份打开 “Developer Command Prompt for VS 2022”（或普通终端，脚本会自动调用 `vcvarsall.bat`）：

```bat
cd /d e:\C++\AHServer\AServer
build_windows.bat Release x64-windows
```

产物位于 `bin/`。

## 3. Windows 核心协议冒烟测试

在构建完成且同级目录存在 `ahxt_server` 运行时数据后执行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File E:/C++/AHServer/AServer/tests/robot_smoke/run_core_smoke.ps1 -KeepTemp
```

脚本会在 `%TEMP%` 创建隔离运行时，启动 `logserver`、`loginserver`、`gameserver` 和 `robotclient`，记录端口/进程/应用日志并输出 `result.json`。成功返回 `0`，代码或协议失败返回 `1`，明确外部依赖阻塞返回 `2`。当前 Windows PowerShell 5.1 环境没有 `pwsh` 时使用上面的 `powershell.exe` 命令；失败时保留临时目录并打印路径。

## 4. 编译（Linux / WSL）

```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg && ./bootstrap-vcpkg.sh && export VCPKG_ROOT=$HOME/vcpkg

cd /mnt/e/C++/AHServer/AServer
bash build_wsl.sh Release x64-linux
```

## 5. 依赖清单

| 包 | 用途 |
|---|---|
| `curl` | HTTP 鉴权/请求 |
| `openssl` | 加密与 HTTPS |
| `zlib` / `zstd` | 压缩 |
| `c-ares` | 异步 DNS |
| `libmysql` | MySQL 客户端 |
| `luajit` | Lua 5.1 运行时 |

gperftools (tcmalloc/profiler) 为可选项，默认关闭。如需启用：

```bat
cmake -S . -B build_win -DASERVER_ENABLE_TCMALLOC=ON ...
```

## 6. 常见问题

- **找不到 vcpkg**：确认 `VCPKG_ROOT` 指向包含 `scripts/buildsystems/vcpkg.cmake` 的目录。
- **MySQL 头文件缺失**：vcpkg 的 `libmysql` 头文件位于 `include/mysql/`，CMake 会自动加入搜索路径。
- **C4819 / C4828 编码警告**：源码含中文注释，构建脚本已使用 `/utf-8`，不影响编译结果。
- **libmysql 编译慢**：Windows 下 `libmysql` 依赖 Boost，首次编译耗时较长，后续会命中 vcpkg 二进制缓存。
