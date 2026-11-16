cmake_minimum_required(VERSION 3.15)
project(aserver_engine LANGUAGES C CXX)

set(PROJBASE "${CMAKE_CURRENT_SOURCE_DIR}/..")

# 核心引擎源码目录；LuaJIT 由 vcpkg 提供，不再源码编译
set(ENGINE_SUBDIRS base misc luawrapper network chipmunk detour scene)

add_library(common STATIC)

target_include_directories(common PUBLIC
    "${PROJBASE}/lib/include"
    "${PROJBASE}/lib/include/chipmunk"
    "${PROJBASE}/lib/include/detour"
    "${PROJBASE}/lib/include/gperftools"
    "${PROJBASE}/lib/include/scene"
    "${PROJBASE}/sceneobj/object"
    "${PROJBASE}/sceneobj/sceneobj_utils"
    "${PROJBASE}/sceneobj/action"
    "${PROJBASE}/sceneobj/ai"
)

foreach(_subdir IN LISTS ENGINE_SUBDIRS)
    file(GLOB_RECURSE _sources CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}/*.cc")
    list(FILTER _sources EXCLUDE REGEX ".*UnitTest.*")
    list(FILTER _sources EXCLUDE REGEX ".*mallochook.*")
    target_sources(common PRIVATE ${_sources})
    target_include_directories(common PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}")
endforeach()

# vcpkg 的 LuaJIT 不含原 vendored 版本内置的 cjson/cmsgpack，单独编入
target_sources(common PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/luajitlib/src/lua_cjson.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/luajitlib/src/lua_cmsgpack.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/luajitlib/src/fpconv.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/luajitlib/src/strbuf.c"
)
target_include_directories(common PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/luajitlib/src")

# 收集所有依赖的头文件与库（Lua/OpenSSL/curl/MySQL 等）
target_link_libraries(common PUBLIC aserver_deps)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(common PRIVATE
        -Wall -Wextra -fPIC -fpermissive -Wno-unused-parameter)
endif()

if(ASERVER_ENABLE_TCMALLOC)
    target_compile_definitions(common PUBLIC ASERVER_ENABLE_TCMALLOC)
endif()

set_target_properties(common PROPERTIES OUTPUT_NAME "common")
