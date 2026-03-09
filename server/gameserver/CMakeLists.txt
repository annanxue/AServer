cmake_minimum_required(VERSION 3.10)
project(gameserver)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wno-narrowing)

set(PROJBASE ${CMAKE_CURRENT_SOURCE_DIR}/../../)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJBASE}/lib/include
    ${PROJBASE}/lib/include/chipmunk
    ${PROJBASE}/lib/include/scene
    ${PROJBASE}/sceneobj/object
    ${PROJBASE}/sceneobj/sceneobj_utils
    ${PROJBASE}/sceneobj/action
    ${PROJBASE}/sceneobj/ai
    ${PROJBASE}/lib/include/detour
)

set(SOURCES
    bf_client_module.cpp
    game_svr_module.cpp
    login_client_module.cpp
    log_client_module.cpp
    gm_svr_module.cpp
    gm_client_module.cpp
    game_lua_module.cpp
    db_client_module.cpp
    UnitTest/game_app.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    ${PROJBASE}/lib/libsceneobj.a
    ${PROJBASE}/lib/libcommon_release.a
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
