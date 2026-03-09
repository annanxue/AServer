@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: AServer Windows 编译脚本
:: 使用 vcpkg 管理依赖

echo === AServer Windows 编译脚本 ===

:: 检查 Visual Studio 环境
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo 正在初始化 Visual Studio 环境...
    
    :: 尝试查找 VS2022
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    if not exist "!VS_PATH!" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    )
    if not exist "!VS_PATH!" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    )
    
    :: 尝试查找 VS2019
    if not exist "!VS_PATH!" (
        set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
    )
    if not exist "!VS_PATH!" (
        set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    )
    if not exist "!VS_PATH!" (
        set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    )
    
    if not exist "!VS_PATH!" (
        echo 错误: 未找到 Visual Studio
        echo 请安装 Visual Studio 2019 或 2022，并安装 "使用 C++ 的桌面开发" 工作负载
        pause
        exit /b 1
    )
    
    echo 找到 Visual Studio: !VS_PATH!
    call "!VS_PATH!" x64
    
    :: 验证环境是否初始化成功
    where cl >nul 2>nul
    if !errorlevel! neq 0 (
        echo 错误: 初始化 Visual Studio 环境失败
        pause
        exit /b 1
    )
    echo Visual Studio 环境初始化成功
)

:: 设置 vcpkg 路径
if "%VCPKG_ROOT%"=="" (
    :: 尝试常见路径
    if exist "C:\vcpkg\vcpkg.exe" (
        set "VCPKG_ROOT=C:\vcpkg"
    ) else if exist "D:\vcpkg\vcpkg.exe" (
        set "VCPKG_ROOT=D:\vcpkg"
    ) else if exist "%USERPROFILE%\vcpkg\vcpkg.exe" (
        set "VCPKG_ROOT=%USERPROFILE%\vcpkg"
    ) else (
        echo 错误: 未找到 vcpkg，请设置 VCPKG_ROOT 环境变量
        echo 例如: set VCPKG_ROOT=C:\vcpkg
        pause
        exit /b 1
    )
)

echo vcpkg 路径: %VCPKG_ROOT%

:: 检查 vcpkg 是否存在
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo 错误: vcpkg 可执行文件不存在
    echo 请先安装 vcpkg:
    echo   git clone https://github.com/Microsoft/vcpkg.git %%VCPKG_ROOT%%
    echo   cd %%VCPKG_ROOT%% ^&^& bootstrap-vcpkg.bat
    pause
    exit /b 1
)

:: 获取项目根目录
set "PROJECT_ROOT=%~dp0"
cd /d "%PROJECT_ROOT%"

echo 项目路径: %PROJECT_ROOT%

:: 设置编译类型
set "BUILD_TYPE=%~1"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Release"
echo 编译类型: %BUILD_TYPE%

:: 设置目标架构
set "VCPKG_TRIPLET=%~2"
if "%VCPKG_TRIPLET%"=="" set "VCPKG_TRIPLET=x64-windows"
echo 目标架构: %VCPKG_TRIPLET%

:: 安装依赖
echo === 安装 vcpkg 依赖 ===
"%VCPKG_ROOT%\vcpkg.exe" install --triplet=%VCPKG_TRIPLET%
if %errorlevel% neq 0 (
    echo 错误: 安装依赖失败
    pause
    exit /b 1
)

:: 创建构建目录
set "BUILD_DIR=%PROJECT_ROOT%build"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

:: 运行 CMake
echo === 运行 CMake ===
cmake .. ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=%VCPKG_TRIPLET% ^
    -G "Visual Studio 17 2022" ^
    -A x64

if %errorlevel% neq 0 (
    :: 尝试 VS2019
    cmake .. ^
        -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
        -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
        -DVCPKG_TARGET_TRIPLET=%VCPKG_TRIPLET% ^
        -G "Visual Studio 16 2019" ^
        -A x64
    
    if %errorlevel% neq 0 (
        echo 错误: CMake 配置失败
        pause
        exit /b 1
    )
)

:: 编译
echo === 开始编译 ===
cmake --build . --config %BUILD_TYPE% --parallel

if %errorlevel% neq 0 (
    echo 错误: 编译失败
    pause
    exit /b 1
)

echo === 编译完成 ===
echo 可执行文件位于: %PROJECT_ROOT%bin\%BUILD_TYPE%\

pause
