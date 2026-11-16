cmake_minimum_required(VERSION 3.15)

# vcpkg 工具链必须在 project() 之前设置，否则 find_package 找不到 vcpkg 包
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg CMake toolchain")
endif()

project(AServer VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 平台识别
if(WIN32)
    set(ASERVER_WINDOWS TRUE)
elseif(APPLE)
    set(ASERVER_APPLE TRUE)
else()
    set(ASERVER_LINUX TRUE)
endif()

# 所有配置共用输出目录，兼容 VS 多配置生成器
set(ASERVER_BIN_DIR "${CMAKE_SOURCE_DIR}/bin")
set(ASERVER_LIB_DIR "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ASERVER_BIN_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ASERVER_LIB_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ASERVER_LIB_DIR}")
foreach(_cfg Debug Release RelWithDebInfo MinSizeRel)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg} "${ASERVER_BIN_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_cfg} "${ASERVER_LIB_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_cfg} "${ASERVER_LIB_DIR}")
endforeach()

if(WIN32)
    add_compile_definitions(
        _WIN32_WINNT=0x0601
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
        _USE_MATH_DEFINES
    )
endif()

if(MSVC)
    add_compile_options(/utf-8)
endif()

# ---------------- vcpkg 依赖 ----------------
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(c-ares CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(Threads REQUIRED)

# 不同 triplet 下 zstd 的导入目标名不同
if(TARGET zstd::libzstd_static)
    set(ASERVER_ZSTD_TARGET zstd::libzstd_static)
else()
    set(ASERVER_ZSTD_TARGET zstd::libzstd)
endif()

set(_vcpkg_installed "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

# LuaJIT：vcpkg 端口仅提供 pkg-config，手动定位以适配所有平台
find_path(LUAJIT_INCLUDE_DIR NAMES lua.h luajit.h
    PATH_SUFFIXES luajit luajit-2.1
    HINTS "${_vcpkg_installed}" REQUIRED)
find_library(LUAJIT_LIBRARY NAMES luajit lua51
    HINTS "${_vcpkg_installed}" REQUIRED)

# MySQL 客户端（vcpkg port: libmysql）
find_path(MYSQL_INCLUDE_DIR NAMES mysql.h
    PATH_SUFFIXES mysql mariadb
    HINTS "${_vcpkg_installed}" /usr/include /usr/local/include)
find_library(MYSQL_LIBRARY NAMES mysqlclient libmysql mysql mariadb
    HINTS "${_vcpkg_installed}/lib" /usr/lib /usr/local/lib)
if(NOT MYSQL_INCLUDE_DIR OR NOT MYSQL_LIBRARY)
    message(FATAL_ERROR "MySQL client library not found. Install vcpkg port 'libmysql'.")
endif()

# vcpkg 头文件优先于仓库内自带的旧版第三方头文件
include_directories(BEFORE "${LUAJIT_INCLUDE_DIR}")
if(DEFINED VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
    include_directories(BEFORE "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")
endif()
include_directories("${CMAKE_SOURCE_DIR}/lib/include")

add_library(aserver_deps INTERFACE)
target_include_directories(aserver_deps INTERFACE
    "${LUAJIT_INCLUDE_DIR}"
    "${MYSQL_INCLUDE_DIR}")
target_link_libraries(aserver_deps INTERFACE
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    "${ASERVER_ZSTD_TARGET}"
    c-ares::cares
    Threads::Threads
    "${LUAJIT_LIBRARY}"
    "${MYSQL_LIBRARY}")
if(ASERVER_LINUX)
    target_link_libraries(aserver_deps INTERFACE dl rt)
endif()

option(ASERVER_BUILD_SERVERS "Build server executables" ON)
option(ASERVER_ENABLE_TCMALLOC "Enable gperftools tcmalloc/profiler (optional)" OFF)

add_subdirectory(server_engine)
add_subdirectory(sceneobj)

if(ASERVER_BUILD_SERVERS)
    foreach(_srv bfserver dbserver gameserver loginserver logserver robotclient)
        add_subdirectory(server/${_srv})
    endforeach()
endif()
