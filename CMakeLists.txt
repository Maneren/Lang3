cmake_minimum_required(VERSION 3.31..4.2)

set(PROJECT "Lang3")
project(${PROJECT})

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
set(CMAKE_INSTALL_MESSAGE LAZY)

set(CMAKE_CXX_STANDARD 26)

option(SANITIZERS "Enable sanitizers" OFF)
if(SANITIZERS)
    add_compile_options(-fsanitize=address,undefined,leak)
    add_link_options(-fsanitize=address,undefined,leak)
endif()

option(USE_CCACHE "Use ccache" OFF)
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

if(IS_READABLE doc)
    add_subdirectory(doc EXCLUDE_FROM_ALL)
endif()

if(IS_READABLE test)
    enable_testing()
    add_subdirectory(test)
endif()

# Install configuration
install(TARGETS lang3
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

# Packaging
set(PACKAGE_NAME "${PROJECT}-$<CONFIG>")
set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/package/${PACKAGE_NAME}")

file(GLOB_RECURSE SOURCES
    src/*.cpp src/*.cppm src/*.hpp src/*.h
    src/*.y src/*.l
    apps/*.cpp apps/*.hpp
    cmake/*.cmake
    CMakeLists.txt
)
list(APPEND SOURCES "README.md" "LICENSE")
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/(test|doc|build)/")

set(SOURCE_ARCHIVE "${PACKAGE_DIR}/source.tgz")
add_custom_command(
    OUTPUT "${SOURCE_ARCHIVE}"
    COMMAND ${CMAKE_COMMAND} -E tar cf "${SOURCE_ARCHIVE}" ${SOURCES}
    COMMENT "Creating source archive ${SOURCE_ARCHIVE}"
    DEPENDS ${SOURCES}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VERBATIM
)

file(GLOB EXAMPLES examples/*.l3)

add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/${PACKAGE_NAME}.tgz"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_DIR}/examples"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:lang3>"
        "${SOURCE_ARCHIVE}" "${DOC_PDF}"
        "${PACKAGE_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${EXAMPLES}
        "${PACKAGE_DIR}/examples"
    COMMAND ${CMAKE_COMMAND} -E tar cf
        "${CMAKE_BINARY_DIR}/${PACKAGE_NAME}.tgz"
         "."
    COMMENT "Creating package ${PACKAGE_NAME}.tgz"
    DEPENDS ${SOURCE_ARCHIVE} ${EXAMPLES} ${DOC_PDF}
    BYPRODUCTS ${PACKAGE_DIR}
    WORKING_DIRECTORY "${PACKAGE_DIR}"
    VERBATIM
)

add_custom_target(package
    DEPENDS ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}.tgz
    COMMENT "Packaging ${PACKAGE_NAME}"
    VERBATIM
)
