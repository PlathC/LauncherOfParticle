get_cxx_flags(PTO_COMPILATION_FLAGS PTO_COMPILE_DEFINITIONS)

set(PTO_HEADERS
    include/pto/Renderer/View/HardwarePathTracing.hpp
    include/pto/Renderer/Geometry.hpp
    
    include/pto/System/System.hpp
    include/pto/System/Transform.hpp

    include/pto/Ui/Camera.hpp
    include/pto/Ui/Controller.hpp
)

set(PTO_SOURCES
    src/Renderer/View/HardwarePathTracing.cpp
    src/Renderer/Geometry.cpp

    src/Ui/Camera.cpp

    src/System/Transform.cpp
)

add_executable(            PTOOnline src/main.cpp ${PTO_SOURCES})
target_link_libraries(     PTOOnline PRIVATE Vazteran EnTT::EnTT)
target_compile_features(   PTOOnline PRIVATE cxx_std_17)
target_compile_options(    PTOOnline PRIVATE ${PTO_COMPILATION_FLAGS})
target_compile_definitions(PTOOnline PRIVATE ${PTO_COMPILE_DEFINITIONS})
target_include_directories(PTOOnline PRIVATE include/)

add_dependency_folder(     PTOOnline PtoShaders "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "${CMAKE_BINARY_DIR}/bin/shaders")
