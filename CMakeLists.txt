cmake_minimum_required(VERSION 3.25)

set(PROJECT "cmake-project-template")
project(${PROJECT})

set(CMAKE_CXX_EXTENSIONS OFF)
set(DEFAULT_CXX_STD 26)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
set(CMAKE_INSTALL_MESSAGE LAZY)

option(SANITIZERS "Enable sanitizers" OFF)
if(SANITIZERS)
    add_compile_options(-fsanitize=address,undefined,leak)
    add_link_options(-fsanitize=address,undefined,leak)
endif()

find_program(SCCACHE sccache)

if (SCCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE})
endif()

include(cmake/ExecutableTemplate.cmake)
include(cmake/LibraryTemplate.cmake)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

add_subdirectory(src)
add_subdirectory(apps)

option(BUILD_TESTS "Build test executables" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
