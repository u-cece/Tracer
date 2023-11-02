add_executable(main main.cpp)

target_compile_features(main PRIVATE cxx_std_23)

target_link_libraries(main tracer)

target_include_directories(main PRIVATE ${PROJECT_SOURCE_DIR}/dependencies/stb)

target_compile_definitions(main PRIVATE $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:_CRT_SECURE_NO_WARNINGS>)