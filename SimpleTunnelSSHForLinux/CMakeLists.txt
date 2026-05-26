cmake_minimum_required(VERSION 3.24)

project(SimpleTunnelSSHForLinux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_subdirectory(src/SimpleTunnelSSH.Core)
add_subdirectory(src/SimpleTunnelSSH.App)

enable_testing()
add_subdirectory(tests/SimpleTunnelSSH.Core.Tests)

set(STS_DESKTOP_EXEC "SimpleTunnelSSH")
configure_file(
	${CMAKE_SOURCE_DIR}/installer/simpletunnelssh.desktop.in
	${CMAKE_BINARY_DIR}/simpletunnelssh.desktop
	@ONLY)

install(TARGETS SimpleTunnelSSH.App RUNTIME DESTINATION bin)
install(FILES ${CMAKE_BINARY_DIR}/simpletunnelssh.desktop DESTINATION share/applications)
install(FILES ${CMAKE_SOURCE_DIR}/src/SimpleTunnelSSH.App/Assets/simpletunnelssh.svg DESTINATION share/icons/hicolor/scalable/apps)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "simpletunnelssh")
set(CPACK_PACKAGE_VENDOR "SimpleTunnelSSH")
set(CPACK_PACKAGE_CONTACT "maintainer@simpletunnelssh.local")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Lightweight SSH tunnel manager for Linux")
set(CPACK_PACKAGE_FILE_NAME "simpletunnelssh-${PROJECT_VERSION}-linux-amd64")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "SimpleTunnelSSH")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "openssh-client, qt6-qpa-plugins")

include(CPack)