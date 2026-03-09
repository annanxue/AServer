# AServer vcpkg 编译指南

本文档说明如何在 WSL Ubuntu 环境下使用 vcpkg 管理依赖并编译 AServer 项目。

## 前置要求

1. Windows 10/11 安装 WSL2 + Ubuntu
2. 安装必要的编译工具:
   ```bash
   sudo apt update
   sudo apt install -y build-essential cmake git
   ```

## 安装 vcpkg

```bash
# 克隆 vcpkg
git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg

# 编译 vcpkg
cd ~/vcpkg
./bootstrap-vcpkg.sh

# 设置环境变量（添加到 ~/.bashrc）
echo 'export VCPKG_ROOT=$HOME/vcpkg' >> ~/.bashrc
source ~/.bashrc
```

## 编译项目

### 方法一：使用提供的编译脚本

```bash
# 进入项目目录
cd /mnt/d/AHServer/AServer

# 运行编译脚本
bash build_wsl.sh

# 编译 Debug 版本
bash build_wsl.sh Debug

# 指定目标架构
bash build_wsl.sh Release x64-linux
```

### 方法二：手动编译

```bash
# 进入项目目录
cd /mnt/d/AHServer/AServer

# 安装依赖
$VCPKG_ROOT/vcpkg install

# 创建构建目录
mkdir -p build && cd build

# 运行 CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# 编译
cmake --build . --parallel $(nproc)
```

## vcpkg 依赖清单

项目的依赖在 [vcpkg.json](vcpkg.json) 中定义：

- `curl` - HTTP 客户端库
- `openssl` - SSL/TLS 加密库
- `zlib` - 压缩库
- `c-ares` - 异步 DNS 解析库
- `libmysql` - MySQL 客户端库
- `zstd` - Zstandard 压缩库

## 项目结构

```
AServer/
├── CMakeLists.txt          # 根 CMake 配置
├── vcpkg.json              # vcpkg 依赖清单
├── build_wsl.sh            # WSL 编译脚本
├── server_engine/          # 引擎库
├── sceneobj/               # 场景对象库
└── server/                 # 服务器可执行文件
    ├── bfserver/
    ├── dbserver/
    ├── gameserver/
    ├── loginserver/
    ├── logserver/
    └── robotclient/
```

## 常见问题

### 1. vcpkg 找不到

确保 `VCPKG_ROOT` 环境变量已设置：
```bash
export VCPKG_ROOT=/path/to/vcpkg
```

### 2. MySQL 库链接失败

vcpkg 安装的 MySQL 库可能需要手动指定路径。CMake 会自动查找以下位置：
- `${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib`
- 系统默认路径

### 3. 编译内存不足

减少并行编译任务数：
```bash
cmake --build . --parallel 2
```

## 注意事项

1. 项目使用 C++11 标准
2. 依赖的静态库（如 libffluajit.a、libtcmalloc.a 等）需要预先编译
3. 输出文件位于 `bin/` 和 `lib/` 目录
