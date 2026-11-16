cmake_minimum_required(VERSION 3.15)
project(dbserver LANGUAGES CXX)

add_executable(${PROJECT_NAME}
    db_svr_module.cpp
    gm_client_module.cpp
    db_lua_module.cpp
    UnitTest/db_app.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} PRIVATE common aserver_deps)
