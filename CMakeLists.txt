cmake_minimum_required(VERSION 3.16)
project(KDEP6Dock VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(BUILD_STANDALONE_DOCK "Build legacy standalone Qt Widgets dock executable" OFF)

find_package(Qt6 REQUIRED COMPONENTS Core)

if(BUILD_STANDALONE_DOCK)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Gui)
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

    if(X11_FOUND)
        target_link_libraries(kdep6dock PRIVATE X11::X11)
    endif()

    install(TARGETS kdep6dock RUNTIME DESTINATION bin)
endif()

# Plasma-integrated dock package (primary target)
install(DIRECTORY plasmoid/package/
        DESTINATION share/plasma/plasmoids/org.kdep6dock.magnifyingdock)

install(FILES config/sample-config.json DESTINATION share/kdep6dock)
