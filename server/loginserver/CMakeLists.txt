cmake_minimum_required(VERSION 3.10)
project(loginserver)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wno-narrowing)

set(PROJBASE ${CMAKE_CURRENT_SOURCE_DIR}/../../)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJBASE}/lib/include
)

set(SOURCES
    game_login_svr_module.cpp
    player_login_svr_module.cpp
    login_lua_module.cpp
    log_client_module.cpp
    gm_client_module.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    ${MYSQL_LIBRARY}
    ${PROJBASE}/lib/libffluajit.a
    ${PROJBASE}/lib/libtcmalloc.a
    ${PROJBASE}/lib/libprofiler.a
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    zstd::libzstd_static
    c-ares::cares
    Threads::Threads
    dl
    rt
)
