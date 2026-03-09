#!/bin/bash

# AServer WSL Ubuntu 编译脚本
# 使用 vcpkg 管理依赖

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== AServer 编译脚本 ===${NC}"

# 检查是否在 WSL 环境中
if ! grep -qE "(Microsoft|WSL)" /proc/version 2>/dev/null; then
    echo -e "${YELLOW}警告: 未检测到 WSL 环境${NC}"
fi

# 检查 vcpkg 是否在 Windows 磁盘上（/mnt/）
if [[ "$VCPKG_ROOT" == /mnt/* ]]; then
    echo -e "${YELLOW}警告: 检测到 vcpkg 位于 Windows 磁盘 ($VCPKG_ROOT)${NC}"
    echo -e "${YELLOW}这可能导致权限问题，建议将 vcpkg 安装到 WSL Linux 文件系统中${NC}"
    echo ""
    echo "解决方案:"
    echo "  1. 在 WSL 中重新安装 vcpkg:"
    echo "     cd ~ && git clone https://github.com/Microsoft/vcpkg.git"
    echo "     cd vcpkg && ./bootstrap-vcpkg.sh"
    echo "     echo 'export VCPKG_ROOT=\$HOME/vcpkg' >> ~/.bashrc"
    echo ""
    echo "  2. 或者设置环境变量使用现有的 Linux vcpkg:"
    echo "     export VCPKG_ROOT=/path/to/linux/vcpkg"
    echo ""
    read -p "是否继续尝试编译? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# 设置 vcpkg 路径
if [ -z "$VCPKG_ROOT" ]; then
    # 尝试常见路径
    if [ -d "$HOME/vcpkg" ]; then
        export VCPKG_ROOT="$HOME/vcpkg"
    elif [ -d "/usr/local/vcpkg" ]; then
        export VCPKG_ROOT="/usr/local/vcpkg"
    elif [ -d "/opt/vcpkg" ]; then
        export VCPKG_ROOT="/opt/vcpkg"
    else
        echo -e "${RED}错误: 未找到 vcpkg，请设置 VCPKG_ROOT 环境变量${NC}"
        echo "例如: export VCPKG_ROOT=/path/to/vcpkg"
        exit 1
    fi
fi

echo -e "${GREEN}vcpkg 路径: $VCPKG_ROOT${NC}"

# 检查 vcpkg 是否存在
if [ ! -f "$VCPKG_ROOT/vcpkg" ]; then
    echo -e "${RED}错误: vcpkg 可执行文件不存在${NC}"
    echo "请先安装 vcpkg:"
    echo "  git clone https://github.com/Microsoft/vcpkg.git \$VCPKG_ROOT"
    echo "  cd \$VCPKG_ROOT && ./bootstrap-vcpkg.sh"
    exit 1
fi

# 获取项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

echo -e "${GREEN}项目路径: $PROJECT_ROOT${NC}"

# 设置编译类型
BUILD_TYPE=${1:-Release}
echo -e "${GREEN}编译类型: $BUILD_TYPE${NC}"

# 设置目标架构
VCPKG_TRIPLET=${2:-x64-linux}
echo -e "${GREEN}目标架构: $VCPKG_TRIPLET${NC}"

# 安装依赖
echo -e "${YELLOW}=== 安装 vcpkg 依赖 ===${NC}"
$VCPKG_ROOT/vcpkg install --triplet=$VCPKG_TRIPLET

# 创建构建目录
BUILD_DIR="$PROJECT_ROOT/build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 运行 CMake
echo -e "${YELLOW}=== 运行 CMake ===${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=$VCPKG_TRIPLET

# 编译
echo -e "${YELLOW}=== 开始编译 ===${NC}"
cmake --build . --parallel $(nproc)

echo -e "${GREEN}=== 编译完成 ===${NC}"
echo -e "可执行文件位于: $PROJECT_ROOT/bin/"
