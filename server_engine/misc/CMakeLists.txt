cmake_minimum_required(VERSION 3.15)

set(PROJBASE ${CMAKE_CURRENT_SOURCE_DIR}/../..)

file(GLOB MISC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(misc STATIC ${MISC_SOURCES})

target_include_directories(misc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJBASE}/lib/include
    /usr/include/mariadb
)

target_compile_options(misc PRIVATE
    -Wno-deprecated-declarations
    -fpermissive
    -Wno-error
)

if(WIN32)
    target_compile_definitions(misc PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

set_target_properties(misc PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${PROJBASE}/lib
)
