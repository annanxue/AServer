@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo === AServer Windows 编译脚本 (vcpkg) ===

rem ---- 1. 定位 vcpkg ----
rem 当前终端可能仍继承旧值，先校验有效性再决定是否覆盖
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    if exist "D:\vcpkg\vcpkg.exe" set "VCPKG_ROOT=D:\vcpkg"
    if exist "C:\vcpkg\vcpkg.exe" set "VCPKG_ROOT=C:\vcpkg"
    if exist "%USERPROFILE%\vcpkg\vcpkg.exe" set "VCPKG_ROOT=%USERPROFILE%\vcpkg"
)
if "%VCPKG_ROOT%"=="" (
    echo 错误: 未找到 vcpkg。请先安装并设置全局环境变量 VCPKG_ROOT。
    echo   git clone https://github.com/microsoft/vcpkg.git D:\vcpkg
    echo   D:\vcpkg\bootstrap-vcpkg.bat -disableMetrics
    echo   setx VCPKG_ROOT D:\vcpkg
    exit /b 1
)
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo 错误: vcpkg 工具链不存在: %VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
    exit /b 1
)
echo VCPKG_ROOT = %VCPKG_ROOT%
set "PATH=%VCPKG_ROOT%;%PATH%"

rem ---- 2. 初始化 MSVC 环境（Ninja/NMake 需要；VS 生成器可自动发现） ----
where cl >nul 2>nul
if errorlevel 1 (
    set "VSDEV="
    for %%P in (
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
    ) do (
        if exist "%%~P" set "VSDEV=%%~P"
    )
    if not "!VSDEV!"=="" (
        echo 初始化 MSVC 环境: !VSDEV!
        call "!VSDEV!" x64
    )
)

rem ---- 3. 参数 ----
set "BUILD_TYPE=%~1"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Release"
set "TRIPLET=%~2"
if "%TRIPLET%"=="" set "TRIPLET=x64-windows"
set "BUILD_DIR=%~dp0build_win"

echo 编译类型 = %BUILD_TYPE%
echo 目标架构 = %TRIPLET%

rem ---- 4. 配置（manifest 模式会自动安装缺失依赖） ----
cmake -S "%~dp0" -B "%BUILD_DIR%" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DVCPKG_TARGET_TRIPLET=%TRIPLET% ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
if errorlevel 1 (
    echo 错误: CMake 配置失败
    exit /b 1
)

rem ---- 5. 构建 ----
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel
if errorlevel 1 (
    echo 错误: 编译失败
    exit /b 1
)

echo === 编译完成，可执行文件位于 %~dp0bin ===
