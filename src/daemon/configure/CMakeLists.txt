cmake_minimum_required(VERSION 2.8)

#project(configure)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(INCLUDE_DIRS "./include")
set(TARGET_NAME ks-vaudit-configure)

file(GLOB_RECURSE DAEMON_H_FILES ./*.h)

find_package(Qt5 COMPONENTS Core Gui DBus Sql Xml REQUIRED)

set(DBUS_ADAPTOR_SRC "")
qt5_add_dbus_adaptor(DBUS_ADAPTOR_SRC
        data/com.kylinsec.Kiran.VauditConfigure.xml
        vaudit-configure.h
        VauditConfigureDbus
        configure_adaptor
        ConfigureAdaptor)
foreach (adaptor_item ${DBUS_ADAPTOR_SRC})
    set_property(SOURCE ${adaptor_item} PROPERTY SKIP_AUTOGEN ON)
endforeach ()

add_executable(${TARGET_NAME}
    ${DAEMON_H_FILES}
    main.cpp
    general-configure.cpp
    sql-configure.cpp
    vaudit-configure.cpp
    ${DBUS_ADAPTOR_SRC})

target_include_directories(${TARGET_NAME} PRIVATE
    ${INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${TARGET_NAME}
    Qt5::DBus
    Qt5::Core
    Qt5::Sql)

set(CONFIGURE_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
install(TARGETS ${TARGET_NAME} DESTINATION ${CONFIGURE_BIN_DIR})

add_subdirectory(data)
