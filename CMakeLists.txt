cmake_minimum_required(VERSION 3.15)

project(AServer CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/lib/include
)

if(UNIX)
    find_package(Threads REQUIRED)
    set(EXTRA_LIBS Threads::Threads dl)
    
    find_path(MYSQL_INCLUDE_DIR
        NAMES mysql.h
        PATHS
            /usr/include/mysql
            /usr/include/mariadb
            /usr/local/include/mysql
            /usr/local/include/mariadb
        NO_DEFAULT_PATH
    )
    
    if(MYSQL_INCLUDE_DIR)
        include_directories(${MYSQL_INCLUDE_DIR})
        message(STATUS "Found MySQL include directory: ${MYSQL_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "MySQL/MariaDB headers not found. Please install mysql-devel or mariadb-devel")
    endif()
endif()

add_subdirectory(server_engine)
add_subdirectory(sceneobj)
add_subdirectory(server/bfserver)
add_subdirectory(server/dbserver)
add_subdirectory(server/gameserver)
add_subdirectory(server/loginserver)
add_subdirectory(server/logserver)
add_subdirectory(server/robotclient)
