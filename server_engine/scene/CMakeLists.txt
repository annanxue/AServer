cmake_minimum_required(VERSION 3.15)

set(PROJBASE ${CMAKE_CURRENT_SOURCE_DIR}/../..)

file(GLOB SCENE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(scene STATIC ${SCENE_SOURCES})

target_include_directories(scene PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJBASE}/lib/include
    ${PROJBASE}/lib/include/scene
    ${PROJBASE}/lib/include/detour
    ${PROJBASE}/lib/include/chipmunk
    ${PROJBASE}/sceneobj/object
    ${PROJBASE}/sceneobj/ai
    ${PROJBASE}/sceneobj/action
)

target_link_libraries(scene
    sceneobj
    detour
    chipmunk
)

if(WIN32)
    target_compile_definitions(scene PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

set_target_properties(scene PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${PROJBASE}/lib
)
