cmake_minimum_required(VERSION 3.15)

set(PROJBASE ${CMAKE_CURRENT_SOURCE_DIR}/../..)

file(GLOB BASE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(base STATIC ${BASE_SOURCES})

target_include_directories(base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJBASE}/lib/include
)

target_link_libraries(base
    scene
    chipmunk
    detour
    network
    luawrapper
    misc
)

if(UNIX)
    target_link_libraries(base
        pthread
        dl
    )
endif()

if(WIN32)
    target_compile_definitions(base PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

set_target_properties(base PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${PROJBASE}/lib
)
