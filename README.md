# AServer

从 `ahxt_server` 移植的游戏服务器框架，使用 C++ 与 Lua 脚本，支持 Windows 与 Linux/WSL 编译。第三方依赖由 vcpkg 管理。

## 功能

- 跨平台：Windows (MSVC) 与 Linux/WSL (GCC/Clang)
- 网络层：Linux epoll / BSD kqueue-poll / Windows 多 fd select，逻辑帧驱动
- 多进程模型：gameserver / loginserver / dbserver / bfserver / logserver / robotclient
- Lua 5.1 (LuaJIT) 脚本驱动，消息分发与配置加载
- 日志、MySQL 访问、流量控制、包加密等公共模块

## 依赖

通过 `vcpkg.json` 声明：`curl`、`openssl`、`zlib`、`c-ares`、`libmysql`、`luajit`、`zstd`。

tcmalloc/gperftools 通过 CMake 选项 `ASERVER_ENABLE_TCMALLOC` 可选启用（默认关闭）。

## 构建

前置条件：CMake 3.15+、vcpkg（设置全局环境变量 `VCPKG_ROOT`）、C++11 编译器。

### Windows

```bat
build_windows.bat Release x64-windows
```

脚本会自动定位 vcpkg 工具链并初始化 MSVC 环境。可执行文件输出到 `bin/`。

### Linux / WSL

```bash
bash build_wsl.sh Release x64-linux
```

### 手动配置

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=x64-linux
cmake --build build --parallel
```

## 目录结构

```
AServer/
├── CMakeLists.txt          # 顶层 CMake，统一依赖与平台设置
├── vcpkg.json              # vcpkg 依赖清单
├── build_windows.bat       # Windows 构建脚本
├── build_wsl.sh            # Linux/WSL 构建脚本
├── lib/include/            # 引擎与第三方头文件
├── server_engine/          # 核心引擎（base/misc/network/luawrapper/scene）
├── sceneobj/               # 场景对象静态库
└── server/                 # 6 个服务器可执行文件
```

## 启动

```bash
gameserver -c <配置文件.lua>
```

配置文件为 Lua 脚本，定义 `AppConfig` 表（`Daemon`、`LogicFrameRate` 等）。
