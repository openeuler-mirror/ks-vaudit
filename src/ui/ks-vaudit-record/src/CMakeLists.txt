#project(ks-vaudit-record)

cmake_minimum_required(VERSION 2.8)

FILE(GLOB KS_UI_SRC_LIST_H "./*.h")
FILE(GLOB KS_UI_SRC_LIST_CPP "./*.cpp")
FILE(GLOB KS_UI_SRC_LIST_UI "./*.ui")
FILE(GLOB_RECURSE KS_UI_SRC_LIST_RESOURCES "../data/*.qrc")

file(COPY "${CMAKE_SOURCE_DIR}/src/daemon/configure/data/com.kylinsec.Kiran.VauditConfigure.xml" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(DBUS_XML_SRC ${CMAKE_CURRENT_BINARY_DIR}/com.kylinsec.Kiran.VauditConfigure.xml)
set_source_files_properties(${DBUS_XML_SRC}
        PROPERTIES
        CLASSNAME ConfigureInterface
        NO_NAMESPACE true)
qt5_add_dbus_interface(CONFIGUREINTERFACE_SRC ${DBUS_XML_SRC} configure_interface)
foreach (interface_item ${CONFIGUREINTERFACE_SRC})
    message("INTERFACE_ITEM: " ${interface_item})
    set_property(SOURCE ${interface_item} PROPERTY SKIP_AUTOGEN ON)
endforeach ()

#message("[]...... " ${TARGET_NAME})
#message("[]...... " ${CMAKE_BINARY_DIR})
#message("[]...... " ${CMAKE_CURRENT_BINARY_DIR})


#set(VAUDITCONFIGURE_SRC /home/flw/github/build-ks-vaudit-Desktop_Qt_5_7_1_GCC_64bit-Release/src/configure_interface.cpp;/home/flw/github/build-ks-vaudit-Desktop_Qt_5_7_1_GCC_64bit-Release/src/configure_interface.h)
message("VAUDITCONFIGURE_SRC${CONFIGUREINTERFACE_SRC}")
message("CPP: ${KS_UI_SRC_LIST_CPP}")
message("H: ${KS_UI_SRC_LIST_H}")
message("UI: ${KS_UI_SRC_LIST_UI}")
message("RESOURCE: ${KS_UI_SRC_LIST_RESOURCES}")
ADD_EXECUTABLE(${TARGET_NAME}
    ${KS_UI_SRC_LIST_CPP}
    ${KS_UI_SRC_LIST_H}
    ${KS_UI_SRC_LIST_UI}
    ${KS_UI_SRC_LIST_RESOURCES}
    ${CONFIGUREINTERFACE_SRC})


target_include_directories(${TARGET_NAME} PRIVATE
#    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${TARGET_NAME}
    -lavformat
    -lavcodec
    -lavutil
    Qt5::Gui
    Qt5::Widgets
    Qt5::DBus
    Qt5::Core)


