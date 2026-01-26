cmake_minimum_required(VERSION 3.31)

set(PROJECT "Lang3")
project(${PROJECT})

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(DEFAULT_CXX_STD 26)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
set(CMAKE_INSTALL_MESSAGE LAZY)

option(SANITIZERS "Enable sanitizers" OFF)
if(SANITIZERS)
    add_compile_options(-fsanitize=address,undefined,leak)
    add_link_options(-fsanitize=address,undefined,leak)
endif()

option(USE_CCACHE "Use ccache" ON)
if (USE_CCACHE)
    find_program(CCACHE ccache)
    find_program(SCCACHE sccache)
    if (CCACHE)
        message(STATUS "Found ccache: ${CCACHE}")
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
    elseif (SCCACHE)
        message(STATUS "Found sccache: ${SCCACHE}")
        set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE})
    else()
        message(WARNING "ccache not found")
    endif()
endif()

include(cmake/ExecutableTemplate.cmake)
include(cmake/LibraryTemplate.cmake)

add_subdirectory(src)
add_subdirectory(apps)

enable_testing()
add_subdirectory(test)
