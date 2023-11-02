cmake_minimum_required(VERSION 3.10)

project(Tracer)

add_subdirectory(dependencies/glm)

add_subdirectory(src)

option(TRACER_BUILD_TEST ON)

if (TRACER_BUILD_TEST)
    add_subdirectory(test)
endif()