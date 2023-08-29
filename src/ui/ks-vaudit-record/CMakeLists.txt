#project(ks-vaudit-record)

cmake_minimum_required(VERSION 2.8)

include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET_NAME ks-vaudit-record)

find_package(
    Qt5
    COMPONENTS
    Core
    Gui
    Widgets
    DBus
    REQUIRED)

add_subdirectory(src)
add_subdirectory(data)

