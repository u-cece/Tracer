add_library(tracer
            ${PROJECT_SOURCE_DIR}/include/tracer/aabb.h
            ${PROJECT_SOURCE_DIR}/include/tracer/bvh.h
            ${PROJECT_SOURCE_DIR}/include/tracer/camera.h
            ${PROJECT_SOURCE_DIR}/include/tracer/canvas.h
            ${PROJECT_SOURCE_DIR}/include/tracer/emission_profile.h
            ${PROJECT_SOURCE_DIR}/include/tracer/material.h
            ${PROJECT_SOURCE_DIR}/include/tracer/mesh.h
            ${PROJECT_SOURCE_DIR}/include/tracer/object.h
            ${PROJECT_SOURCE_DIR}/include/tracer/octree.h
            ${PROJECT_SOURCE_DIR}/include/tracer/rng.h
            ${PROJECT_SOURCE_DIR}/include/tracer/sampler.h
            ${PROJECT_SOURCE_DIR}/include/tracer/scene.h
            ${PROJECT_SOURCE_DIR}/include/tracer/texture.h
            ${PROJECT_SOURCE_DIR}/include/tracer/tracer.h
            canvas.cpp
            json_helper.h
            material.cpp
            mesh.cpp
            sampler.cpp
            scene.cpp
            thread_pool.h
            texture.cpp
            tracer.cpp
            object.cpp
            util.h)

target_compile_features(tracer PUBLIC cxx_std_23)

target_link_libraries(tracer PUBLIC glm)
target_link_libraries(tracer PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(tracer PRIVATE fmt::fmt)
target_include_directories(tracer PRIVATE ${PROJECT_SOURCE_DIR}/dependencies/stb)

target_compile_definitions(tracer PRIVATE
                           $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:_CRT_SECURE_NO_WARNINGS>
                           GLM_FORCE_INTRINSICS)

target_include_directories(tracer PUBLIC ${PROJECT_SOURCE_DIR}/include)