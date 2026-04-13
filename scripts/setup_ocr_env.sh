#!/bin/bash
# OCR环境配置脚本
# 自动安装RapidOCR并启动OCR服务器

set -e

echo "=========================================="
echo "PhotoTrick OCR环境配置脚本"
echo "=========================================="

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 检查Python
echo -e "${YELLOW}[1/4] 检查Python环境...${NC}"
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}错误: 未找到Python3，请先安装Python3${NC}"
    exit 1
fi
PYTHON_VERSION=$(python3 --version)
echo -e "${GREEN}✓ 找到 $PYTHON_VERSION${NC}"

# 检查pip
echo -e "${YELLOW}[2/4] 检查pip...${NC}"
if ! command -v pip &> /dev/null && ! python3 -m pip --version &> /dev/null; then
    echo -e "${RED}错误: 未找到pip，请先安装pip${NC}"
    exit 1
fi
echo -e "${GREEN}✓ pip可用${NC}"

# 安装依赖
echo -e "${YELLOW}[3/4] 安装OCR依赖...${NC}"
pip install rapidocr-onnxruntime flask pillow numpy --quiet 2>/dev/null || \
python3 -m pip install rapidocr-onnxruntime flask pillow numpy --quiet

echo -e "${GREEN}✓ 依赖安装完成${NC}"

# 验证安装
echo -e "${YELLOW}[4/4] 验证安装...${NC}"
if python3 -c "from rapidocr_onnxruntime import RapidOCR; print('RapidOCR OK')" 2>/dev/null; then
    echo -e "${GREEN}✓ RapidOCR安装成功${NC}"
else
    echo -e "${RED}错误: RapidOCR安装失败${NC}"
    exit 1
fi

if python3 -c "from flask import Flask; print('Flask OK')" 2>/dev/null; then
    echo -e "${GREEN}✓ Flask安装成功${NC}"
else
    echo -e "${RED}错误: Flask安装失败${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}=========================================="
echo "OCR环境配置完成!"
echo "==========================================${NC}"
echo ""
echo "启动OCR服务器:"
echo "  python3 scripts/paddle_server.py"
echo ""
echo "或使用后台模式:"
echo "  nohup python3 scripts/paddle_server.py > ocr_server.log 2>&1 &"
echo ""
echo "测试OCR服务:"
echo "  curl --noproxy '*' http://127.0.0.1:5000/health"
echo ""
