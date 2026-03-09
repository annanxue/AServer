cmake_minimum_required(VERSION 3.15)

project(AServer CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# vcpkg 集成
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# 查找 vcpkg 包
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(c-ares CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(Threads REQUIRED)

# MySQL
find_path(MYSQL_INCLUDE_DIR
    NAMES mysql.h
    PATHS
        ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/mysql
        /usr/include/mysql
        /usr/include/mariadb
        /usr/local/include/mysql
        /usr/local/include/mariadb
)

find_library(MYSQL_LIBRARY
    NAMES mysqlclient libmysql
    PATHS
        ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib64/mysql
        /usr/lib/mysql
)

if(MYSQL_INCLUDE_DIR)
    include_directories(${MYSQL_INCLUDE_DIR})
    message(STATUS "Found MySQL include directory: ${MYSQL_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "MySQL/MariaDB headers not found.")
endif()

if(MYSQL_LIBRARY)
    message(STATUS "Found MySQL library: ${MYSQL_LIBRARY}")
else()
    message(FATAL_ERROR "MySQL library not found.")
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/lib/include
)

add_subdirectory(server_engine)
add_subdirectory(sceneobj)
add_subdirectory(server/bfserver)
add_subdirectory(server/dbserver)
add_subdirectory(server/gameserver)
add_subdirectory(server/loginserver)
add_subdirectory(server/logserver)
add_subdirectory(server/robotclient)
