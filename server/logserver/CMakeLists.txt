cmake_minimum_required(VERSION 3.15)
project(logserver LANGUAGES CXX)

add_executable(${PROJECT_NAME}
    log_svr_module.cpp
    log_lua_module.cpp
    gm_client_module.cpp
    UnitTest/log_app.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} PRIVATE common aserver_deps)
