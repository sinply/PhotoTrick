@echo off
chcp 65001 >nul
echo ========================================
echo   PhotoTrick 测试脚本
echo ========================================
echo.

echo [1] 检查应用...
if exist "%~dp0build\PhotoTrick.exe" (
    echo     ✓ PhotoTrick.exe 存在
) else (
    echo     ✗ PhotoTrick.exe 不存在
    goto :end
)

echo.
echo [2] 检查 Python 依赖...
python -c "import flask" 2>nul
if %errorlevel%==0 (
    echo     ✓ Flask 已安装
) else (
    echo     ✗ Flask 未安装
)

python -c "from PIL import Image" 2>nul
if %errorlevel%==0 (
    echo     ✓ Pillow 已安装
) else (
    echo     ✗ Pillow 未安装
)

python -c "from paddleocr import PaddleOCR" 2>nul
if %errorlevel%==0 (
    echo     ✓ PaddleOCR 已安装
) else (
    echo     ✗ PaddleOCR 未安装
)

echo.
echo [3] 安装依赖 (使用清华源)...
python -m pip install --trusted-host pypi.tuna.tsinghua.edu.cn -i https://pypi.tuna.tsinghua.edu.cn/simple flask pillow paddleocr

echo.
echo [4] 启动 PaddleOCR 服务...
start "PaddleOCR Server" cmd /k "cd /d %~dp0scripts && python paddle_server.py"
echo     已在新窗口启动 OCR 服务 (端口 5000)

echo.
echo [5] 等待服务启动...
timeout /t 5 /nobreak >nul

echo.
echo [6] 测试服务健康状态...
python -c "import urllib.request; print(urllib.request.urlopen('http://localhost:5000/health').read().decode())" 2>nul
if %errorlevel%==0 (
    echo     ✓ OCR 服务运行正常
) else (
    echo     ✗ OCR 服务未响应
)

echo.
echo [7] 启动 PhotoTrick 应用...
start "" "%~dp0build\PhotoTrick.exe"
echo     已启动应用

:end
echo.
echo ========================================
echo   测试完成
echo ========================================
pause
