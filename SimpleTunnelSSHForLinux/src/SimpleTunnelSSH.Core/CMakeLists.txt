set(SIMPLE_TUNNEL_SSH_CORE_SOURCES
    Infrastructure/AppPaths.cpp
    Services/JsonConfigurationStore.cpp
    Services/PortConflictDetector.cpp
    Services/SshCommandBuilder.cpp
    Services/TunnelRuntimeService.cpp
)

add_library(SimpleTunnelSSH.Core STATIC ${SIMPLE_TUNNEL_SSH_CORE_SOURCES})

target_link_libraries(SimpleTunnelSSH.Core
    PUBLIC
        Qt6::Core
)

target_include_directories(SimpleTunnelSSH.Core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)