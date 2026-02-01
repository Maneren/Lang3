get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE MODULES CONFIGURE_DEPENDS src/*.cppm)

add_library(${LIB_NAME} STATIC
  ${SOURCES}
)

target_sources(${LIB_NAME} INTERFACE
  FILE_SET HEADERS
  BASE_DIRS include
  FILES include/utils/utils.hpp
)

target_sources(${LIB_NAME} PUBLIC
  FILE_SET cxx_modules TYPE CXX_MODULES
  FILES ${MODULES}
)

set_compiler_and_linker_flags(${LIB_NAME})
