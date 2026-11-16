cmake_minimum_required(VERSION 3.15)
project(loginserver LANGUAGES CXX)

add_executable(${PROJECT_NAME}
    game_login_svr_module.cpp
    player_login_svr_module.cpp
    login_lua_module.cpp
    log_client_module.cpp
    gm_client_module.cpp
    UnitTest/login_app.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} PRIVATE common aserver_deps)
