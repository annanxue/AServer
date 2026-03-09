cmake_minimum_required(VERSION 3.15)

set(PROJBASE ${CMAKE_CURRENT_SOURCE_DIR}/../..)

file(GLOB LUAWRAPPER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(luawrapper STATIC ${LUAWRAPPER_SOURCES})

target_include_directories(luawrapper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJBASE}/lib/include
)

target_link_libraries(luawrapper
    misc
    network
)

if(WIN32)
    target_compile_definitions(luawrapper PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

set_target_properties(luawrapper PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${PROJBASE}/lib
)
