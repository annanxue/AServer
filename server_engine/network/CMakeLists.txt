cmake_minimum_required(VERSION 3.15)

set(PROJBASE ${CMAKE_CURRENT_SOURCE_DIR}/../..)

file(GLOB NETWORK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(network STATIC ${NETWORK_SOURCES})

target_include_directories(network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJBASE}/lib/include
)

if(WIN32)
    target_compile_definitions(network PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

set_target_properties(network PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${PROJBASE}/lib
)
