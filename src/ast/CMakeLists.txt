get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE MODULES CONFIGURE_DEPENDS src/*.cppm)

add_library(${LIB_NAME} STATIC
  ${SOURCES}
)

target_sources(${LIB_NAME} INTERFACE
  FILE_SET HEADERS
  BASE_DIRS include
  FILES include/ast/ast.hpp
)

target_sources(${LIB_NAME} INTERFACE
  PUBLIC
  FILE_SET cxx_modules TYPE CXX_MODULES
  FILES ${MODULES}
)

target_link_libraries(${LIB_NAME} PUBLIC utils)
