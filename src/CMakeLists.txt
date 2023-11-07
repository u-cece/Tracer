add_library(tracer
            ${PROJECT_SOURCE_DIR}/include/tracer/camera.h
            ${PROJECT_SOURCE_DIR}/include/tracer/canvas.h
            ${PROJECT_SOURCE_DIR}/include/tracer/material.h
            ${PROJECT_SOURCE_DIR}/include/tracer/mesh.h
            ${PROJECT_SOURCE_DIR}/include/tracer/object.h
            ${PROJECT_SOURCE_DIR}/include/tracer/rng.h
            ${PROJECT_SOURCE_DIR}/include/tracer/sampler.h
            ${PROJECT_SOURCE_DIR}/include/tracer/scene.h
            ${PROJECT_SOURCE_DIR}/include/tracer/texture.h
            ${PROJECT_SOURCE_DIR}/include/tracer/tracer.h
            canvas.cpp
            material.cpp
            mesh.cpp
            sampler.cpp
            scene.cpp
            thread_pool.h
            texture.cpp
            tracer.cpp
            object.cpp
            util.h)

target_compile_features(tracer PRIVATE cxx_std_23)

target_link_libraries(tracer PUBLIC glm)

target_include_directories(tracer PRIVATE ${PROJECT_SOURCE_DIR}/dependencies/stb)

target_compile_definitions(tracer PRIVATE $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:_CRT_SECURE_NO_WARNINGS>)

target_include_directories(tracer PUBLIC ${PROJECT_SOURCE_DIR}/include)