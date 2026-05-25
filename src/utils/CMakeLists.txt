get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

auto_create_library(${LIB_NAME})

target_precompile_headers(${LIB_NAME}
  PUBLIC "include/utils/visit.h" "include/utils/accessor.h"
)
