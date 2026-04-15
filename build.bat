@echo off
REM PhotoTrick Build Script (Windows)

echo ========================================
echo   PhotoTrick Build Script
echo ========================================
echo.

REM Set Qt environment
set QT_PATH=D:\Qt\6.8.3\mingw_64
set QT_TOOLS=D:\Qt\Tools\mingw1310_64
set CMAKE_PATH=D:\Qt\Tools\CMake_64\bin
set PATH=%CMAKE_PATH%;%QT_PATH%\bin;%QT_TOOLS%\bin;%PATH%

REM Change to project directory
cd /d "%~dp0"

echo [1/3] Checking build environment...
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo     [ERROR] CMake not found
    goto :error
) else (
    echo     [OK] CMake available
)

where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo     [ERROR] MinGW g++ not found
    goto :error
) else (
    echo     [OK] MinGW g++ available
)

if not exist "%QT_PATH%\bin\Qt6Core.dll" (
    echo     [ERROR] Qt6 not found
    goto :error
) else (
    echo     [OK] Qt 6.8.3 available
)

echo.
echo [2/3] Building project...
if not exist build (
    echo     Creating build directory...
    cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=%QT_PATH%
    if %errorlevel% neq 0 (
        echo     [ERROR] CMake configuration failed
        goto :error
    )
)

cmake --build build -j4
if %errorlevel% neq 0 (
    echo     [ERROR] Build failed
    goto :error
)
echo     [OK] Build successful

echo.
echo [3/3] Deploying Qt dependencies...
if exist build\PhotoTrick.exe (
    %QT_PATH%\bin\windeployqt.exe build\PhotoTrick.exe
    if %errorlevel% neq 0 (
        echo     [WARN] Deploy warning, may need manual deploy
    ) else (
        echo     [OK] Qt dependencies deployed
    )
) else (
    echo     [ERROR] PhotoTrick.exe not found
    goto :error
)

echo.
echo ========================================
echo   Build complete: build\PhotoTrick.exe
echo ========================================
goto :end

:error
echo.
echo ========================================
echo   Build failed
echo ========================================
pause
exit /b 1

:end
pause
