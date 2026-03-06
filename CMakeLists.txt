cmake_minimum_required(VERSION 3.15)
project(AServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    add_definitions(-DLUA_USE_WINDOWS)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/core)
include_directories(${CMAKE_SOURCE_DIR}/include/modules)
include_directories(${CMAKE_SOURCE_DIR}/lua51)

find_package(Threads REQUIRED)
find_package(OpenSSL QUIET)
find_package(CURL QUIET)

add_subdirectory(lua51)

set(COMMON_SOURCES
    src/common.cpp
    src/thread.cpp
)

set(CORE_SOURCES
    src/core/app_base.cpp
    src/core/logic_thread.cpp
    src/core/lua_server.cpp
    src/core/net.cpp
)

set(MODULE_SOURCES
    src/modules/log_module.cpp
    src/modules/net_module.cpp
    src/modules/timer_module.cpp
    src/modules/game_svr_module.cpp
    src/modules/game_lua_module.cpp
    src/modules/db_client_module.cpp
    src/modules/bf_client_module.cpp
    src/modules/login_client_module.cpp
    src/modules/log_client_module.cpp
    src/modules/gm_client_module.cpp
    src/modules/gm_svr_module.cpp
)

set(APP_SOURCES
    src/game_app.cpp
)

add_library(AServerCore STATIC
    ${COMMON_SOURCES}
    ${CORE_SOURCES}
    ${MODULE_SOURCES}
)

target_link_libraries(AServerCore
    Threads::Threads
    lua
)

if(OpenSSL_FOUND)
    target_link_libraries(AServerCore OpenSSL::SSL OpenSSL::Crypto)
endif()

if(CURL_FOUND)
    target_link_libraries(AServerCore CURL::libcurl)
endif()

if(WIN32)
    target_link_libraries(AServerCore ws2_32)
endif()

add_executable(AServer ${APP_SOURCES})
target_link_libraries(AServer AServerCore)

if(UNIX)
    set_target_properties(AServer PROPERTIES LINK_FLAGS "-pthread")
endif()

install(TARGETS AServer RUNTIME DESTINATION bin)

message(STATUS "========================================")
message(STATUS "AServer CMake Configuration")
message(STATUS "========================================")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")
