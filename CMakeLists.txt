cmake_minimum_required(VERSION 3.16)
project(KDEP6Dock VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)
find_package(X11 QUIET)

add_executable(kdep6dock
    src/main.cpp
    src/Config.cpp
    src/DockModel.cpp
    src/DesktopEntry.cpp
    src/DockItemWidget.cpp
    src/DockWindow.cpp
)

target_include_directories(kdep6dock PRIVATE src)
target_link_libraries(kdep6dock PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core)

install(TARGETS kdep6dock RUNTIME DESTINATION bin)
install(FILES config/sample-config.json DESTINATION share/kdep6dock)

if(X11_FOUND)
    target_link_libraries(kdep6dock PRIVATE X11::X11)
endif()
