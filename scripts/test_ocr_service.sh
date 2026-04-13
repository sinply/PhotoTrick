#!/bin/bash
# 测试OCR服务

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=========================================="
echo "OCR服务测试"
echo "=========================================="

# 测试健康端点
echo -e "${YELLOW}[1/3] 测试健康端点...${NC}"
HEALTH=$(curl --noproxy '*' -s http://127.0.0.1:5000/health 2>/dev/null)
if echo "$HEALTH" | grep -q '"status":"ok"'; then
    echo -e "${GREEN}✓ 健康检查通过${NC}"
    echo "  响应: $HEALTH"
else
    echo -e "${RED}✗ 健康检查失败${NC}"
    echo "  请确保OCR服务器已启动: ./scripts/start_ocr_server.sh start"
    exit 1
fi

# 测试OCR识别
echo -e "${YELLOW}[2/3] 测试OCR识别...${NC}"
TEST_RESULT=$(python3 << 'EOF'
import requests
import base64
import io
from PIL import Image, ImageDraw, ImageFont
import json

# 创建测试图片
img = Image.new('RGB', (400, 100), color='white')
draw = ImageDraw.Draw(img)

try:
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 30)
except:
    font = ImageFont.load_default()

draw.text((50, 30), "Test OCR 123", fill='black', font=font)

buffer = io.BytesIO()
img.save(buffer, format='PNG')
img_base64 = base64.b64encode(buffer.getvalue()).decode()

# 发送OCR请求
response = requests.post(
    'http://127.0.0.1:5000/ocr',
    json={'image': img_base64},
    proxies={'http': None, 'https': None},
    timeout=30
)

result = response.json()
print(json.dumps(result))
EOF
)

if echo "$TEST_RESULT" | grep -q '"success"'; then
    TEXT=$(echo "$TEST_RESULT" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('text',''))" 2>/dev/null || echo "解析失败")
    if [ -n "$TEXT" ]; then
        echo -e "${GREEN}✓ OCR识别成功${NC}"
        echo "  识别结果: $TEXT"
    else
        echo -e "${YELLOW}⚠ OCR识别返回空结果${NC}"
    fi
else
    echo -e "${RED}✗ OCR识别失败${NC}"
    echo "  响应: $TEST_RESULT"
    exit 1
fi

# 测试带坐标的OCR
echo -e "${YELLOW}[3/3] 测试带坐标的OCR...${NC}"
TEST_RESULT2=$(python3 << 'EOF'
import requests
import base64
import io
from PIL import Image, ImageDraw, ImageFont
import json

# 创建测试图片
img = Image.new('RGB', (400, 100), color='white')
draw = ImageDraw.Draw(img)

try:
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 30)
except:
    font = ImageFont.load_default()

draw.text((50, 30), "Test OCR 123", fill='black', font=font)

buffer = io.BytesIO()
img.save(buffer, format='PNG')
img_base64 = base64.b64encode(buffer.getvalue()).decode()

# 发送OCR请求
response = requests.post(
    'http://127.0.0.1:5000/ocr',
    json={'image': img_base64, 'options': {'return_boxes': True}},
    proxies={'http': None, 'https': None},
    timeout=30
)

result = response.json()
print(json.dumps(result))
EOF
)

if echo "$TEST_RESULT2" | grep -q '"boxes"'; then
    BOXES=$(echo "$TEST_RESULT2" | python3 -c "import sys,json; d=json.load(sys.stdin); print(len(d.get('boxes',[])))")
    echo -e "${GREEN}✓ 带坐标OCR成功${NC}"
    echo "  检测到 $BOXES 个文本区域"
else
    echo -e "${RED}✗ 带坐标OCR失败${NC}"
    echo "  响应: $TEST_RESULT2"
    exit 1
fi

echo ""
echo -e "${GREEN}=========================================="
echo "所有测试通过!"
echo "==========================================${NC}"
