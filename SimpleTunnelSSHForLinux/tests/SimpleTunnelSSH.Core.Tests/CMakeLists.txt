add_executable(SimpleTunnelSSH.Core.Tests
    PortConflictDetectorTests.cpp
    SshCommandBuilderTests.cpp
    main.cpp
)

target_link_libraries(SimpleTunnelSSH.Core.Tests
    PRIVATE
        Qt6::Core
        SimpleTunnelSSH.Core
)

target_include_directories(SimpleTunnelSSH.Core.Tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/SimpleTunnelSSH.Core
)

add_test(NAME SimpleTunnelSSH.Core.Tests COMMAND SimpleTunnelSSH.Core.Tests)