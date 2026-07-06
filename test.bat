@echo off
chcp 65001 >nul
echo ========================================
echo   PhotoTrick 测试脚本
echo ========================================
echo.

echo [1] 检查应用...
if exist "%~dp0build\PhotoTrick.exe" (
    echo     [OK] PhotoTrick.exe 存在
) else (
    echo     [FAIL] PhotoTrick.exe 不存在，请先运行 build.bat
    goto :end
)

echo.
echo [2] 检查 Python 环境...
where python >nul 2>&1
if %errorlevel% neq 0 (
    echo     [WARN] python 不在 PATH，跳过 OCR 服务测试
    goto :start_app
)
echo     [OK] python 可用

set "NEED_INSTALL=0"
python -c "import flask" 2>nul
if %errorlevel% neq 0 (
    echo     [MISS] Flask 未安装
    set "NEED_INSTALL=1"
) else (
    echo     [OK] Flask 已安装
)

python -c "from PIL import Image" 2>nul
if %errorlevel% neq 0 (
    echo     [MISS] Pillow 未安装
    set "NEED_INSTALL=1"
) else (
    echo     [OK] Pillow 已安装
)

python -c "from paddleocr import PaddleOCR" 2>nul
if %errorlevel% neq 0 (
    echo     [MISS] PaddleOCR 未安装
    set "NEED_INSTALL=1"
) else (
    echo     [OK] PaddleOCR 已安装
)

if "%NEED_INSTALL%"=="1" (
    echo.
    echo [3] 安装缺失依赖...
    echo     首选国内源，失败后回退官方源
    python -m pip install flask pillow paddleocr -i https://pypi.tuna.tsinghua.edu.cn/simple --trusted-host pypi.tuna.tsinghua.edu.cn
    if %errorlevel% neq 0 (
        echo     [WARN] 国内源失败，尝试官方源...
        python -m pip install flask pillow paddleocr
    )
) else (
    echo.
    echo [3] 依赖完整，跳过安装
)

echo.
echo [4] 启动 PaddleOCR 服务...
tasklist /FI "WINDOWTITLE eq PaddleOCR Server" 2>nul | findstr /I "cmd.exe" >nul
if %errorlevel% equ 0 (
    echo     [OK] OCR 服务窗口已存在，跳过启动
) else (
    start "PaddleOCR Server" cmd /k "cd /d %~dp0scripts && python paddle_server.py"
    echo     已在新窗口启动 OCR 服务 (端口 5000)
)

echo.
echo [5] 等待服务启动...
timeout /t 5 /nobreak >nul

echo.
echo [6] 测试服务健康状态...
python -c "import urllib.request; print(urllib.request.urlopen('http://localhost:5000/health', timeout=3).read().decode())" 2>nul
if %errorlevel%==0 (
    echo     [OK] OCR 服务运行正常
) else (
    echo     [WARN] OCR 服务未响应，可稍后在应用中重试
)

:start_app
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
