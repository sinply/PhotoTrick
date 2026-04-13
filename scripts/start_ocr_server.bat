@echo off
REM 启动OCR服务器脚本 (Windows)

setlocal enabledelayedexpansion

REM 获取脚本目录
set SCRIPT_DIR=%~dp0
set PROJECT_DIR=%SCRIPT_DIR%..

cd /d "%PROJECT_DIR%"

echo ==========================================
echo 启动OCR服务器
echo ==========================================

REM 检查是否已运行
tasklist /FI "IMAGENAME eq python.exe" /FI "WINDOWTITLE eq *paddle_server*" 2>nul | find "python.exe" >nul
if %errorlevel% equ 0 (
    echo OCR服务器可能已在运行
    echo 如需重启，请先关闭现有进程
)

REM 检查Python
where python >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误: 未找到Python
    pause
    exit /b 1
)

REM 检查依赖
python -c "import flask" 2>nul
if %errorlevel% neq 0 (
    echo 错误: Flask未安装
    echo 请先运行: scripts\setup_ocr_env.bat
    pause
    exit /b 1
)

echo 启动OCR服务器...
echo 地址: http://127.0.0.1:5000
echo 按 Ctrl+C 停止服务器
echo.

python scripts\paddle_server.py
