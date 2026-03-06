cmake_minimum_required(VERSION 3.15)

project(lua51 C)

set(LUA_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/lapi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lcode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ldebug.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ldo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ldump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lfunc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lgc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/llex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lobject.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lopcodes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lparser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lstate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lstring.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ltable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ltm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lundump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lvm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lzio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lauxlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lbaselib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ldblib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/liolib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lmathlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/loslib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lstrlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ltablib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/linit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/loadlib.c
)

if(WIN32)
    add_definitions(-DLUA_BUILD_AS_DLL)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_library(lua STATIC ${LUA_SOURCES})

set_target_properties(lua PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
