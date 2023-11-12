cmake_minimum_required(VERSION 3.10)

project(Tracer)

add_subdirectory(dependencies/glm)
add_subdirectory(dependencies/json)

add_subdirectory(src)

set(TRACER_BUILD_TEST true CACHE BOOL "whether to build executable")

if (TRACER_BUILD_TEST)
    add_subdirectory(test)
endif()