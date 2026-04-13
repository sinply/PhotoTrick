#!/bin/bash
# 停止OCR服务器

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PID_FILE="$PROJECT_DIR/ocr_server.pid"

echo "停止OCR服务器..."

if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE")
    if ps -p "$PID" > /dev/null 2>&1; then
        kill "$PID" 2>/dev/null
        echo "✓ OCR服务器已停止 (PID: $PID)"
    else
        echo "OCR服务器未运行"
    fi
    rm -f "$PID_FILE"
else
    # 通过进程名查找并停止
    pkill -f "paddle_server.py" 2>/dev/null && echo "✓ OCR服务器已停止" || echo "OCR服务器未运行"
fi
