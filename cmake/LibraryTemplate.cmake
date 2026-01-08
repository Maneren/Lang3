include(${CMAKE_CURRENT_LIST_DIR}/CommonUtils.cmake)

function(create_library TARGET SOURCES)
    cmake_parse_arguments(LIB
        "STATIC;SHARED;OBJECT;HEADER_ONLY"
        "VERSION;NAMESPACE;CXX_STD"
        "PRIVATE_DEPS;PUBLIC_DEPS;INTERFACE_DEPS;HEADER_DEPS;EXTRA_SOURCES"
        ${ARGN}
    )

    list(APPEND SOURCES ${LIB_EXTRA_SOURCES})

    set(HEADER_LIBRARY ${TARGET}_headers)

    if (NOT LIB_HEADER_ONLY AND NOT SOURCES)
        message(FATAL_ERROR "Library ${TARGET} has no sources but is not header-only")
    endif()

    if(NOT LIB_CXX_STD)
        set(LIB_CXX_STD ${DEFAULT_CXX_STD})
    endif()

    if(LIB_NAMESPACE)
        set(TARGET "${LIB_NAMESPACE}::${TARGET}")
    endif()

    if(LIB_HEADER_ONLY)
        add_library(${TARGET} INTERFACE)
    elseif(LIB_SHARED)
        add_library(${TARGET} SHARED ${SOURCES})
        set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    elseif(LIB_OBJECT)
        add_library(${TARGET} OBJECT ${SOURCES})
    else()
        add_library(${TARGET} STATIC ${SOURCES})
    endif()

    if(LIB_HEADER_ONLY)
        if (LIB_PRIVATE_DEPS OR LIB_PUBLIC_DEPS)
            message(FATAL_ERROR
                "Header-only library '${TARGET}' can't have private or public dependencies")
        endif()
        target_link_libraries(${TARGET} INTERFACE "${LIB_INTERFACE_DEPS}")
        target_compile_features(${TARGET} INTERFACE cxx_std_${LIB_CXX_STD})
        target_include_directories(${TARGET} INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>)
    else()
        add_library(${HEADER_LIBRARY} INTERFACE)

        target_include_directories(${HEADER_LIBRARY} INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>)
        target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

        target_link_libraries(${HEADER_LIBRARY} INTERFACE "${LIB_HEADER_DEPS}")
        target_link_libraries(${TARGET}
            PUBLIC "${LIB_PUBLIC_DEPS}" "${HEADER_LIBRARY}"
            INTERFACE "${LIB_INTERFACE_DEPS}"
            PRIVATE "${LIB_PRIVATE_DEPS}")

        set_compiler_and_linker_flags(${TARGET} OFF ${LIB_CXX_STD})

        set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${TARGET})
        set_output_directories(${TARGET} LIBRARY)
        set_version_properties(${TARGET} "${LIB_VERSION}")
    endif()
endfunction()

function(auto_create_library TARGET)
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.c src/*.cxx src/*.cc)
    file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.h include/*.hpp include/*.hxx)

    if(NOT SOURCES AND HEADERS)
        create_library(${TARGET} "" HEADER_ONLY ${ARGN})
    else()
        create_library(${TARGET} "${SOURCES}" ${ARGN})
    endif()
endfunction()
