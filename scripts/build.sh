#!/bin/bash
# PhotoTrick 编译脚本 (Linux)

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# 项目目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

# Qt路径 (根据实际安装位置调整)
QT_PATH="${QT_PATH:-/opt/Qt/6.8.3/gcc_64}"

echo "========================================"
echo "  PhotoTrick 编译脚本"
echo "========================================"
echo

# 检查编译环境
echo -e "${YELLOW}[1/3] 检查编译环境...${NC}"
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}✗ 未找到 CMake${NC}"
    exit 1
fi
echo -e "${GREEN}✓ CMake 可用${NC}"

if ! command -v g++ &> /dev/null; then
    echo -e "${RED}✗ 未找到 g++${NC}"
    exit 1
fi
echo -e "${GREEN}✓ g++ 可用${NC}"

if [ ! -d "$QT_PATH" ]; then
    echo -e "${YELLOW}⚠ Qt路径不存在: $QT_PATH${NC}"
    echo "  请设置 QT_PATH 环境变量或修改脚本"
fi

# 编译
echo
echo -e "${YELLOW}[2/3] 编译项目...${NC}"
if [ ! -d build ]; then
    echo "  创建 build 目录..."
    cmake -B build -DCMAKE_PREFIX_PATH="$QT_PATH"
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ CMake 配置失败${NC}"
        exit 1
    fi
fi

cmake --build build -j$(nproc)
if [ $? -ne 0 ]; then
    echo -e "${RED}✗ 编译失败${NC}"
    exit 1
fi
echo -e "${GREEN}✓ 编译成功${NC}"

# 验证
echo
echo -e "${YELLOW}[3/3] 验证可执行文件...${NC}"
if [ -f build/PhotoTrick ]; then
    echo -e "${GREEN}✓ 可执行文件: build/PhotoTrick${NC}"
else
    echo -e "${RED}✗ 未找到可执行文件${NC}"
    exit 1
fi

echo
echo -e "${GREEN}========================================"
echo "  编译完成: build/PhotoTrick"
echo "========================================${NC}"
