get_cxx_flags(PTO_COMPILATION_FLAGS PTO_COMPILE_DEFINITIONS)

set(PTO_HEADERS
    include/pto/Math/Sampling.hpp
    
    include/pto/Renderer/Pass/HardwarePathTracing.hpp
    include/pto/Renderer/Pass/UserInterface.hpp
    include/pto/Renderer/Environment.hpp
    include/pto/Renderer/Geometry.hpp
    include/pto/Renderer/Snapshot.hpp
    
    include/pto/System/System.hpp
    include/pto/System/Transform.hpp

    include/pto/Ui/Camera.hpp
    include/pto/Ui/Controller.hpp
    include/pto/Ui/Overlay.hpp
)

set(PTO_SOURCES
    src/Math/Sampling.cpp

    src/Renderer/Pass/HardwarePathTracing.cpp
    src/Renderer/Pass/UserInterface.cpp
    src/Renderer/Environment.cpp
    src/Renderer/Geometry.cpp
    src/Renderer/Snapshot.cpp

    src/Ui/Camera.cpp
    src/Ui/Overlay.cpp

    src/System/Transform.cpp
)

set(PTO_COMPILE_DEFINITIONS ${PTO_COMPILE_DEFINITIONS} VK_NO_PROTOTYPES _CRT_SECURE_NO_WARNINGS)

add_executable(            PTOOnline src/main.cpp ${PTO_SOURCES} ${PTO_EXTERN_SOURCES})
target_link_libraries(     PTOOnline PRIVATE Vazteran EnTT::EnTT)
target_compile_features(   PTOOnline PRIVATE cxx_std_17)
target_compile_options(    PTOOnline PRIVATE ${PTO_COMPILATION_FLAGS})
target_compile_definitions(PTOOnline PRIVATE ${PTO_COMPILE_DEFINITIONS})
target_include_directories(PTOOnline PRIVATE ${PTO_EXTERN_HEADERS} ${PTO_EXTERN_SOURCES} include/)

add_dependency_folder(PTOOnline PTOShaders "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "${CMAKE_BINARY_DIR}/bin/shaders")
