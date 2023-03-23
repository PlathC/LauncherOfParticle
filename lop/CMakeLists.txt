get_cxx_flags(LOP_COMPILATION_FLAGS LOP_COMPILE_DEFINITIONS)

set(LOP_HEADERS
    include/lop/Math/Sampling.hpp
    
    include/lop/Renderer/Pass/HardwarePathTracing.hpp
    include/lop/Renderer/Pass/UserInterface.hpp
    include/lop/Renderer/Environment.hpp
    include/lop/Renderer/Geometry.hpp
    include/lop/Renderer/Snapshot.hpp
    
    include/lop/System/System.hpp
    include/lop/System/Transform.hpp

    include/lop/Ui/Controller/Camera.hpp
    include/lop/Ui/Controller/Controller.hpp
    include/lop/Ui/Window/Overlay.hpp
)

set(LOP_SOURCES
    src/Math/Sampling.cpp

    src/Renderer/Pass/HardwarePathTracing.cpp
    src/Renderer/Pass/UserInterface.cpp
    src/Renderer/Environment.cpp
    src/Renderer/Geometry.cpp
    src/Renderer/Snapshot.cpp

    src/Ui/Controller/Camera.cpp
    src/Ui/Window/Overlay.cpp

    src/System/Transform.cpp
)

set(LOP_COMPILE_DEFINITIONS ${LOP_COMPILE_DEFINITIONS} VK_NO_PROTOTYPES _CRT_SECURE_NO_WARNINGS)

add_executable(            LOPOnline src/main.cpp ${LOP_SOURCES} ${LOP_EXTERN_SOURCES})
target_link_libraries(     LOPOnline PRIVATE ${LOP_EXTERN_LIBRARIES})
target_compile_features(   LOPOnline PRIVATE cxx_std_17)
target_compile_options(    LOPOnline PRIVATE ${LOP_COMPILATION_FLAGS})
target_compile_definitions(LOPOnline PRIVATE ${LOP_COMPILE_DEFINITIONS})
target_include_directories(LOPOnline PRIVATE ${LOP_EXTERN_HEADERS} ${LOP_EXTERN_SOURCES} include/)

add_dependency_folder(LOPShaders "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "${CMAKE_BINARY_DIR}/bin/shaders")
add_dependencies(LOPOnline LOPShaders)