cmake_minimum_required(VERSION 3.15)
project(bfserver LANGUAGES CXX)

set(PROJBASE "${CMAKE_CURRENT_SOURCE_DIR}/../..")

add_executable(${PROJECT_NAME}
    game_svr_module.cpp
    bf_svr_module.cpp
    log_client_module.cpp
    gm_client_module.cpp
    bf_lua_module.cpp
    UnitTest/bf_app.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJBASE}/lib/include/scene"
    "${PROJBASE}/lib/include/detour"
)

target_link_libraries(${PROJECT_NAME} PRIVATE sceneobj common aserver_deps)
