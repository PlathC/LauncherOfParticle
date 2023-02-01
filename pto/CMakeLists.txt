get_cxx_flags(PTO_COMPILATION_FLAGS PTO_COMPILE_DEFINITIONS)

add_executable(            PTOOnline src/main.cpp)
target_link_libraries(     PTOOnline PRIVATE Vazteran)
target_compile_features(   PTOOnline PRIVATE cxx_std_17)
target_compile_options(    PTOOnline PRIVATE ${PTO_COMPILATION_FLAGS})
target_compile_definitions(PTOOnline PRIVATE ${PTO_COMPILE_DEFINITIONS})
add_dependency_folder(     PTOOnline PtoShaders "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "${CMAKE_BINARY_DIR}/bin/shaders")
