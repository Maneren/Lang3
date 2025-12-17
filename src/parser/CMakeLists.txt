get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

set(LEXER_CPP ${CMAKE_CURRENT_BINARY_DIR}/src/lexer.yy.cpp)

flex_target(Lexer
  src/lexer.l
  ${LEXER_CPP}
  OPTIONS --align --full --ecs
)

set(PARSER_CPP ${CMAKE_CURRENT_BINARY_DIR}/src/parser.tab.cpp)
set(PARSER_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/parser/parser.tab.h)

bison_target(Parser
  src/parser.y
  ${PARSER_CPP}
  DEFINES_FILE ${PARSER_HEADER}
  OPTIONS -Wcounterexamples
)

add_flex_bison_dependency(Lexer Parser)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.c src/*.cxx src/*.cc)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.h include/*.hpp include/*.hxx)

set(SOURCES
  ${SOURCES}
  ${LEXER_CPP}
  ${PARSER_CPP}
)

create_library(${LIB_NAME} "${SOURCES}" "")

target_include_directories(${LIB_NAME}
  PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/include/parser
)
