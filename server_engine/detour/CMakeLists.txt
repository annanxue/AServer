cmake_minimum_required(VERSION 3.15)

set(PROJBASE ${CMAKE_CURRENT_SOURCE_DIR}/../..)

file(GLOB DETOUR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(detour STATIC ${DETOUR_SOURCES})

target_include_directories(detour PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJBASE}/lib/include
    ${PROJBASE}/lib/include/detour
)

if(WIN32)
    target_compile_definitions(detour PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

set_target_properties(detour PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${PROJBASE}/lib
)
