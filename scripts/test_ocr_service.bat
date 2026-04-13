@echo off
REM 测试OCR服务 (Windows)

echo ==========================================
echo OCR服务测试
echo ==========================================

REM 检查curl
where curl >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误: 未找到curl命令
    echo 请使用PowerShell测试:
    echo   Invoke-WebRequest -Uri http://127.0.0.1:5000/health
    pause
    exit /b 1
)

echo [1/2] 测试健康端点...
curl -s http://127.0.0.1:5000/health
if %errorlevel% neq 0 (
    echo.
    echo 错误: 无法连接到OCR服务器
    echo 请确保服务器已启动: scripts\start_ocr_server.bat
    pause
    exit /b 1
)
echo.

echo [2/2] 测试OCR识别...
REM 创建临时Python脚本测试
python -c "import requests; import base64; from PIL import Image, ImageDraw, ImageFont; import io; img = Image.new('RGB', (400, 100), 'white'); d = ImageDraw.Draw(img); d.text((50, 30), 'Test OCR 123', fill='black'); b = io.BytesIO(); img.save(b, 'PNG'); r = requests.post('http://127.0.0.1:5000/ocr', json={'image': base64.b64encode(b.getvalue()).decode()}, timeout=30); print('Result:', r.json().get('text', 'empty'))" 2>nul
if %errorlevel% neq 0 (
    echo 警告: OCR测试失败，请检查依赖是否安装完整
)

echo.
echo ==========================================
echo 测试完成
echo ==========================================
pause
