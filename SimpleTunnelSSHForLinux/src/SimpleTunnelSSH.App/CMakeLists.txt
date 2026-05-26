set(SIMPLE_TUNNEL_SSH_APP_SOURCES
    AppIconFactory.cpp
    AppTheme.cpp
    AskPassPromptForm.cpp
    AskPassPromptHost.cpp
    ConnectionEditorForm.cpp
    ConnectionPickerForm.cpp
    GroupEditorForm.cpp
    LinuxStartupManager.cpp
    LogViewerForm.cpp
    MainForm.cpp
    PortEditorForm.cpp
    Program.cpp
    SettingsForm.cpp
    TunnelApplicationContext.cpp
)

add_executable(SimpleTunnelSSH.App ${SIMPLE_TUNNEL_SSH_APP_SOURCES})

target_link_libraries(SimpleTunnelSSH.App
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        SimpleTunnelSSH.Core
)

target_include_directories(SimpleTunnelSSH.App
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/SimpleTunnelSSH.Core
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(SimpleTunnelSSH.App PROPERTIES OUTPUT_NAME "SimpleTunnelSSH")