# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称和版本
project(libcommon 
        VERSION 1.0 
        LANGUAGES C CXX)

# 设置项目根目录（对应原Makefile的PROJBASE）
set(PROJBASE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# 设置输出目录（对应原Makefile的lib目录）
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJBASE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJBASE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJBASE}/lib)

# 为Debug和Release模式分别设置输出后缀（对应原Makefile的DEBUG_SUFFIX/RELEASE_SUFFIX）
set(CMAKE_DEBUG_POSTFIX "_debug")
set(CMAKE_RELEASE_POSTFIX "_release")

# 定义子目录列表（对应原Makefile的SUBDIR）
set(SUBDIRS 
    base
    misc
    luajitlib
    luawrapper
    network
    chipmunk
    detour
    scene
)

# 定义不需要管理的目录（对应原Makefile的NOT_MANAGED_DIR）
set(NOT_MANAGED_DIR luajitlib)

# 链接需要的系统库（对应原Makefile的SHARED_LIB）
set(SHARED_LIB -lrt)

# 创建静态库目标
add_library(common STATIC)

# 添加全局头文件路径
target_include_directories(common PRIVATE 
    ${PROJBASE}/lib/include
    ${PROJBASE}/lib/include/chipmunk
    ${PROJBASE}/lib/include/curl
    ${PROJBASE}/lib/include/detour
    ${PROJBASE}/lib/include/gperftools
    ${PROJBASE}/lib/include/openssl
    ${PROJBASE}/lib/include/scene
    ${PROJBASE}/sceneobj/object
    ${PROJBASE}/sceneobj/sceneobj_utils
    ${PROJBASE}/sceneobj/action
    ${PROJBASE}/sceneobj/ai
    /usr/include/mysql
    /usr/include/mariadb
)

# 遍历所有子目录，添加源文件
foreach(SUBDIR ${SUBDIRS})
    # 跳过不需要管理的目录
    if(NOT ${SUBDIR} STREQUAL ${NOT_MANAGED_DIR})
        # 添加子目录的源文件（排除UnitTest目录）
        file(GLOB_RECURSE SUBDIR_SOURCES 
             ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/*.c 
             ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/*.cpp 
             ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/*.cc)
        
        # 过滤掉UnitTest目录中的文件
        list(FILTER SUBDIR_SOURCES EXCLUDE REGEX ".*UnitTest.*")
        
        # 过滤掉mallochook.c（glibc 2.34+移除了__malloc_hook）
        list(FILTER SUBDIR_SOURCES EXCLUDE REGEX ".*mallochook.*")
        
        # 将源文件添加到库目标
        target_sources(common PRIVATE ${SUBDIR_SOURCES})
        
        # 添加头文件目录
        target_include_directories(common PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR})
    endif()
endforeach()

# 链接系统库
target_link_libraries(common PRIVATE ${SHARED_LIB})

# 设置编译选项（模拟原Makefile的gcc编译选项）
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    # 添加基础编译选项
    target_compile_options(common PRIVATE 
        -Wall 
        -Wextra 
        -fPIC
        -fpermissive
    )
    
    # Debug模式编译选项
    target_compile_options(common PRIVATE 
        $<$<CONFIG:Debug>:-g -O0>
    )
    
    # Release模式编译选项
    target_compile_options(common PRIVATE 
        $<$<CONFIG:Release>:-O2 -DNDEBUG>
    )
endif()

# 设置库的输出名称（对应原Makefile的TARGET）
set_target_properties(common PROPERTIES OUTPUT_NAME "common")

# 自定义gperftools目标（对应原Makefile的gperftools规则）
add_custom_target(gperftools
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/libunwind-0.99-beta/ && make -f Makefile.server_engine
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/gperftools-2.1/ && make -f Makefile.server_engine
    COMMENT "Building gperftools..."
)

# 自定义test目标
add_custom_target(test
    COMMAND ${CMAKE_MAKE_PROGRAM} all
    COMMENT "Running tests..."
)

# 自定义rtest目标（release模式测试）
add_custom_target(rtest
    COMMAND ${CMAKE_MAKE_PROGRAM} all
    COMMENT "Running release tests..."
)

# 帮助信息
message(STATUS "libcommon build configuration:")
message(STATUS "  Project base: ${PROJBASE}")
message(STATUS "  Output directory: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")