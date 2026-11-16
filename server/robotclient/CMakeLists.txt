cmake_minimum_required(VERSION 3.15)
project(robotclient LANGUAGES CXX)

set(PROJBASE "${CMAKE_CURRENT_SOURCE_DIR}/../..")

add_executable(${PROJECT_NAME}
    robot_lua_module.cpp
    robot_client_module.cpp
    UnitTest/robot_app.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJBASE}/lib/include/scene"
    "${PROJBASE}/lib/include/detour"
)

target_link_libraries(${PROJECT_NAME} PRIVATE sceneobj common aserver_deps)
