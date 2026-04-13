#!/bin/bash
# 启动OCR服务器脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
LOG_FILE="$PROJECT_DIR/ocr_server.log"
PID_FILE="$PROJECT_DIR/ocr_server.pid"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

start_server() {
    echo -e "${YELLOW}启动OCR服务器...${NC}"

    # 检查是否已运行
    if [ -f "$PID_FILE" ]; then
        OLD_PID=$(cat "$PID_FILE")
        if ps -p "$OLD_PID" > /dev/null 2>&1; then
            echo -e "${YELLOW}OCR服务器已在运行 (PID: $OLD_PID)${NC}"
            echo "停止服务器请运行: ./scripts/stop_ocr_server.sh"
            return 0
        fi
    fi

    # 启动服务器
    cd "$PROJECT_DIR"
    nohup python3 scripts/paddle_server.py > "$LOG_FILE" 2>&1 &
    PID=$!
    echo $PID > "$PID_FILE"

    # 等待启动
    sleep 5

    # 验证
    if curl --noproxy '*' -s http://127.0.0.1:5000/health | grep -q '"status":"ok"'; then
        echo -e "${GREEN}✓ OCR服务器启动成功 (PID: $PID)${NC}"
        echo "  地址: http://127.0.0.1:5000"
        echo "  日志: $LOG_FILE"
    else
        echo -e "${RED}✗ OCR服务器启动失败，请检查日志:${NC}"
        echo "  cat $LOG_FILE"
        rm -f "$PID_FILE"
        return 1
    fi
}

stop_server() {
    echo -e "${YELLOW}停止OCR服务器...${NC}"

    if [ -f "$PID_FILE" ]; then
        PID=$(cat "$PID_FILE")
        if ps -p "$PID" > /dev/null 2>&1; then
            kill "$PID" 2>/dev/null
            echo -e "${GREEN}✓ OCR服务器已停止${NC}"
        else
            echo -e "${YELLOW}OCR服务器未运行${NC}"
        fi
        rm -f "$PID_FILE"
    else
        # 尝试通过进程名查找
        PID=$(pgrep -f "paddle_server.py" 2>/dev/null || true)
        if [ -n "$PID" ]; then
            kill "$PID" 2>/dev/null
            echo -e "${GREEN}✓ OCR服务器已停止 (PID: $PID)${NC}"
        else
            echo -e "${YELLOW}OCR服务器未运行${NC}"
        fi
    fi
}

status_server() {
    if [ -f "$PID_FILE" ]; then
        PID=$(cat "$PID_FILE")
        if ps -p "$PID" > /dev/null 2>&1; then
            echo -e "${GREEN}OCR服务器运行中 (PID: $PID)${NC}"

            # 测试健康端点
            HEALTH=$(curl --noproxy '*' -s http://127.0.0.1:5000/health 2>/dev/null || echo '{"status":"error"}')
            echo "  健康状态: $HEALTH"
            return 0
        fi
    fi

    # 检查进程
    PID=$(pgrep -f "paddle_server.py" 2>/dev/null || true)
    if [ -n "$PID" ]; then
        echo -e "${GREEN}OCR服务器运行中 (PID: $PID)${NC}"
        return 0
    fi

    echo -e "${YELLOW}OCR服务器未运行${NC}"
    return 1
}

case "${1:-start}" in
    start)
        start_server
        ;;
    stop)
        stop_server
        ;;
    restart)
        stop_server
        sleep 2
        start_server
        ;;
    status)
        status_server
        ;;
    *)
        echo "用法: $0 {start|stop|restart|status}"
        exit 1
        ;;
esac
