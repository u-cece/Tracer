cmake_minimum_required(VERSION 3.20)

project(Tracer)

add_subdirectory(dependencies/glm)
add_subdirectory(dependencies/json)
add_subdirectory(dependencies/fmt)

add_compile_definitions($<$<COMPILE_LANG_AND_ID:CXX,MSVC>:USING_MSVC>)

add_subdirectory(src)

set(TRACER_BUILD_TEST true CACHE BOOL "whether to build executable")

if(TRACER_BUILD_TEST)
    add_subdirectory(test)
endif()