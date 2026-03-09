cmake_minimum_required(VERSION 3.10)
project(logserver)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wno-narrowing)

set(PROJBASE ${CMAKE_CURRENT_SOURCE_DIR}/../../)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJBASE}/lib/include
)

set(SOURCES
    log_svr_module.cpp
    log_lua_module.cpp
    gm_client_module.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    /usr/lib/x86_64-linux-gnu/libmysqlclient.a
    ${PROJBASE}/lib/libffluajit.a
    ${PROJBASE}/lib/libtcmalloc.a
    ${PROJBASE}/lib/libprofiler.a
    ${PROJBASE}/lib/libcurl.a
    ${PROJBASE}/lib/libssl.a
    ${PROJBASE}/lib/libcrypto.a
    ${PROJBASE}/lib/libz.a
    ${PROJBASE}/lib/libcares.a
    pthread
    dl
    rt
)
