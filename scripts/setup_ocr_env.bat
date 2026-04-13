@echo off
REM OCR环境配置脚本 (Windows)
REM 自动安装PaddleOCR/RapidOCR依赖

echo ==========================================
echo PhotoTrick OCR环境配置脚本
echo ==========================================

REM 检查Python
echo [1/4] 检查Python环境...
where python >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误: 未找到Python，请先安装Python 3.8+
    echo 下载地址: https://www.python.org/downloads/
    pause
    exit /b 1
)
for /f "tokens=*" %%i in ('python --version') do set PYTHON_VERSION=%%i
echo 找到 %PYTHON_VERSION%

REM 检查pip
echo [2/4] 检查pip...
python -m pip --version >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误: pip不可用
    pause
    exit /b 1
)
echo pip可用

REM 安装依赖
echo [3/4] 安装OCR依赖...
echo 选择要安装的OCR引擎:
echo   1. PaddleOCR (功能完整，体积较大)
echo   2. RapidOCR (轻量级，推荐)
set /p OCR_CHOICE="请输入选择 [1/2, 默认2]: "

if "%OCR_CHOICE%"=="1" (
    echo 安装PaddleOCR...
    python -m pip install paddleocr flask pillow-heif pdf2image python-docx openpyxl -q
) else (
    echo 安装RapidOCR...
    python -m pip install rapidocr-onnxruntime flask pillow numpy pdf2image python-docx openpyxl -q
)

echo 依赖安装完成

REM 验证安装
echo [4/4] 验证安装...
python -c "from flask import Flask; print('Flask OK')" 2>nul
if %errorlevel% neq 0 (
    echo 错误: Flask安装失败
    pause
    exit /b 1
)
echo Flask安装成功

if "%OCR_CHOICE%"=="1" (
    python -c "from paddleocr import PaddleOCR; print('PaddleOCR OK')" 2>nul
) else (
    python -c "from rapidocr_onnxruntime import RapidOCR; print('RapidOCR OK')" 2>nul
)
if %errorlevel% neq 0 (
    echo 警告: OCR库安装可能有问题
) else (
    echo OCR库安装成功
)

echo.
echo ==========================================
echo OCR环境配置完成!
echo ==========================================
echo.
echo 启动OCR服务器:
echo   python scripts\paddle_server.py
echo.
echo 测试OCR服务:
echo   curl http://127.0.0.1:5000/health
echo.
pause
