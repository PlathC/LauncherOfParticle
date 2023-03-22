if(NOT TARGET Vazteran)
    add_subdirectory(Vazteran)
endif()

if(NOT TARGET EnTT::EnTT)
    add_subdirectory(entt)
endif()

if(NOT TARGET portable_file_dialogs)
    add_subdirectory(portable-file-dialogs)
endif()

set(IMGUI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imconfig.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_internal.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdl2.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdl2.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.h"
)

set(PTO_EXTERN_SOURCES 
    ${PTO_EXTERN_SOURCES}
    
    ${IMGUI_SOURCES}
)

set(PTO_EXTERN_HEADERS 
    ${PTO_EXTERN_HEADERS}
    
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/stb"
)

set(PTO_EXTERN_LIBRARIES
    ${PTO_EXTERN_LIBRARIES}

    EnTT::EnTT
    portable_file_dialogs
    Vazteran 
)

# Forward to parent's scope
set(PTO_EXTERN_HEADERS   ${PTO_EXTERN_HEADERS}   PARENT_SCOPE)
set(PTO_EXTERN_SOURCES   ${PTO_EXTERN_SOURCES}   PARENT_SCOPE)
set(PTO_EXTERN_LIBRARIES ${PTO_EXTERN_LIBRARIES} PARENT_SCOPE)
