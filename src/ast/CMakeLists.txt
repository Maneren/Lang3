get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

set(NODES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/ast/nodes)
file(GLOB AST_HEADERS LIST_DIRECTORIES FALSE ${NODES_DIR}/*.hpp)

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${GENERATED_DIR}/ast)
set(FORWARD_AST ${GENERATED_DIR}/ast/forward.hpp)

add_custom_command(
  OUTPUT ${FORWARD_AST}
  COMMAND sh -c "\
  echo \$'#pragma once\\nnamespace l3::ast {' \
    \"\$(grep -RhE '^class\\s+\\w+' ${NODES_DIR} | sed -E 's/^class\\s+(\\w*).*/class \\1;/' | sort | uniq)\" \
    '}' > ${FORWARD_AST}"
  DEPENDS ${AST_HEADERS}
  COMMENT "Generating forward AST header"
  VERBATIM
)

auto_create_library(${LIB_NAME}
  HEADER_DEPS utils
  EXTRA_SOURCES ${FORWARD_AST}
)

target_include_directories(${LIB_NAME}_headers INTERFACE ${GENERATED_DIR})
