cmake_minimum_required(VERSION 3.15)

set(PROJBASE ${CMAKE_CURRENT_SOURCE_DIR}/../..)

file(GLOB CHIPMUNK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(chipmunk STATIC ${CHIPMUNK_SOURCES})

target_include_directories(chipmunk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJBASE}/lib/include
    ${PROJBASE}/lib/include/chipmunk
)

if(WIN32)
    target_compile_definitions(chipmunk PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

set_target_properties(chipmunk PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${PROJBASE}/lib
)
