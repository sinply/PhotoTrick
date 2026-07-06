@echo off
REM PhotoTrick Build Script (Windows)

echo ========================================
echo   PhotoTrick Build Script
echo ========================================
echo.

REM Defaults - override via env vars or edit paths below
if not defined QT_PATH       set "QT_PATH=D:\Qt\6.8.3\mingw_64"
if not defined QT_TOOLS      set "QT_TOOLS=D:\Qt\Tools\mingw1310_64"
if not defined CMAKE_PATH    set "CMAKE_PATH=D:\Qt\Tools\CMake_64\bin"

REM Only prepend paths that exist (graceful fallback to system PATH)
if exist "%CMAKE_PATH%\cmake.exe"   set "PATH=%CMAKE_PATH%;%PATH%"
if exist "%QT_PATH%\bin\Qt6Core.dll" set "PATH=%QT_PATH%\bin;%PATH%"
if exist "%QT_TOOLS%\bin\g++.exe"    set "PATH=%QT_TOOLS%\bin;%PATH%"

REM Change to project directory
cd /d "%~dp0"

echo [1/3] Checking build environment...
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo     [ERROR] CMake not found. Set CMAKE_PATH or add to PATH.
    goto :error
) else (
    echo     [OK] CMake available
)

where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo     [ERROR] MinGW g++ not found. Set QT_TOOLS or install MinGW.
    goto :error
) else (
    echo     [OK] MinGW g++ available
)

REM Detect Qt6 either at QT_PATH or via system PATH
set "QT_BIN=%QT_PATH%\bin"
if not exist "%QT_BIN%\Qt6Core.dll" (
    where Qt6Core.dll >nul 2>&1
    if %errorlevel% neq 0 (
        echo     [ERROR] Qt6 not found. Set QT_PATH or add Qt bin to PATH.
        goto :error
    )
    set "QT_BIN="
) else (
    echo     [OK] Qt available at %QT_BIN%
)

echo.
echo [2/3] Building project...
if not exist build (
    echo     Creating build directory...
    if exist "%QT_BIN%\Qt6Core.dll" (
        cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=%QT_PATH%
    ) else (
        cmake -B build -G "MinGW Makefiles"
    )
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
    if defined QT_BIN (
        "%QT_BIN%\windeployqt.exe" build\PhotoTrick.exe
    ) else (
        where windeployqt >nul 2>&1
        if %errorlevel% equ 0 (
            windeployqt build\PhotoTrick.exe
        ) else (
            echo     [WARN] windeployqt not found, skip deploy
        )
    )
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
