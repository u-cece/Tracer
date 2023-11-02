add_library(tracer
            ${PROJECT_SOURCE_DIR}/include/tracer/camera.h
            ${PROJECT_SOURCE_DIR}/include/tracer/canvas.h
            ${PROJECT_SOURCE_DIR}/include/tracer/material.h
            ${PROJECT_SOURCE_DIR}/include/tracer/mesh.h
            ${PROJECT_SOURCE_DIR}/include/tracer/object.h
            ${PROJECT_SOURCE_DIR}/include/tracer/rng.h
            ${PROJECT_SOURCE_DIR}/include/tracer/sampler.h
            ${PROJECT_SOURCE_DIR}/include/tracer/scene.h
            ${PROJECT_SOURCE_DIR}/include/tracer/tracer.h
            material.cpp
            sampler.cpp
            scene.cpp
            thread_pool.h
            tracer.cpp
            object.cpp
            util.h)

target_compile_features(tracer PRIVATE cxx_std_23)

target_link_libraries(tracer PUBLIC glm)

target_include_directories(tracer PUBLIC ${PROJECT_SOURCE_DIR}/include)