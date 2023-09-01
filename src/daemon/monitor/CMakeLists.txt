cmake_minimum_required(VERSION 3.5)
#project(ks-vaudit-monitor)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INCLUDE_DIRS "./include")
set(TARGET_NAME ks-vaudit-monitor)
file(GLOB_RECURSE MONITOR_H_FILES ./*.h)
file(GLOB MONITOR_SRC "./*cpp" "./include/*.h")

find_package(Qt5 COMPONENTS Core Gui DBus REQUIRED)
find_package(PkgConfig REQUIRED)

file(COPY "${CMAKE_SOURCE_DIR}/src/daemon/configure/data/com.kylinsec.Kiran.VauditConfigure.xml" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${CMAKE_SOURCE_DIR}/src/daemon/configure/include/ksvaudit-configure_global.h" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${CMAKE_SOURCE_DIR}/src/common/common-definition.h" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(DBUS_XML_SRC ${CMAKE_CURRENT_BINARY_DIR}/com.kylinsec.Kiran.VauditConfigure.xml)
set_source_files_properties(${DBUS_XML_SRC}
        PROPERTIES
        CLASSNAME ConfigureInterface
        NO_NAMESPACE true)
qt5_add_dbus_interface(CONFIGUREINTERFACE_SRC ${DBUS_XML_SRC} configure_interface)
foreach (interface_item ${CONFIGUREINTERFACE_SRC})
    set_property(SOURCE ${interface_item} PROPERTY SKIP_AUTOGEN ON)
endforeach ()

add_executable(${TARGET_NAME}
    ${MONITOR_SRC}
    ${CONFIGUREINTERFACE_SRC})

target_include_directories(${TARGET_NAME} PRIVATE
        ${INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${TARGET_NAME}
        -lavformat
        -lavutil
        klog-qt5
        pthread
        Qt5::DBus
        Qt5::Core)

set(MONITOR_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
install(TARGETS ${TARGET_NAME} DESTINATION ${MONITOR_BIN_DIR})

add_subdirectory(data)
