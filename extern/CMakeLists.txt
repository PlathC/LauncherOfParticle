if(NOT TARGET Vazteran)
    add_subdirectory(Vazteran)
endif()

if(NOT TARGET EnTT::EnTT)
    add_subdirectory(entt)
endif()

set(PTO_EXTERN_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/stb")
set(PTO_EXTERN_HEADERS ${PTO_EXTERN_HEADERS} PARENT_SCOPE)
