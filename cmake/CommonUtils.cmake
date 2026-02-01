function(set_compiler_and_linker_flags TARGET)
    cmake_parse_arguments(ARGS
        ""
        "CXX_STD"
        ""
        ${ARGN}
    )

    target_compile_options(${TARGET} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
    )

    if(ARGS_CXX_STD)
        set_target_properties(${TARGET} PROPERTIES CXX_STANDARD ${ARGS_CXX_STD})
    endif()

    set(CXX_GNU_DEBUG_INFO -g3 -gdwarf-5 -fno-omit-frame-pointer)

    target_compile_options(${TARGET} PRIVATE
        $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:GNU,Clang>:-Og>>
        $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:GNU,Clang>:${CXX_GNU_DEBUG_INFO}>>

        $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:GNU,Clang>:-flto>>

        $<$<CONFIG:RelWithDebInfo>:$<$<CXX_COMPILER_ID:GNU,Clang>:${CXX_GNU_DEBUG_INFO}>>
        $<$<CONFIG:RelWithDebInfo>:$<$<CXX_COMPILER_ID:GNU,Clang>:-flto>>

        $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:MSVC>:/Od /Zi>>
        $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:MSVC>:/O2 /DNDEBUG>>
    )
    target_link_options(${TARGET} PRIVATE
        $<$<CONFIG:RelWithDebInfo>:$<$<CXX_COMPILER_ID:GNU,Clang>:-flto>>

        $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:GNU,Clang>:-flto>>
    )
endfunction()

function(add_include_directories TARGET VISIBILITY DIRS)
    if(DIRS)
        target_include_directories(${TARGET} ${VISIBILITY} ${DIRS})
    endif()
endfunction()

function(set_output_directories TARGET TYPE)
    if(TYPE STREQUAL "LIBRARY")
        set_target_properties(${TARGET} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    elseif(TYPE STREQUAL "EXECUTABLE")
        set_target_properties(${TARGET} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    elseif(TYPE STREQUAL "TEST")
        set_target_properties(${TARGET} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/bin
            FOLDER "Tests")
    endif()
endfunction()

function(set_version_properties TARGET VERSION)
    if (VERSION)
        set_target_properties(${TARGET} PROPERTIES
            VERSION ${VERSION}
            SOVERSION ${VERSION})
    endif()
endfunction()
