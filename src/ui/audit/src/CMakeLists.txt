#project(ks-vaudit-audit)

cmake_minimum_required(VERSION 2.8)

FILE(GLOB KS_AUDIT_UI_SRC_LIST_H "./*.h")
FILE(GLOB KS_AUDIT_UI_SRC_LIST_CPP "./*.cpp")
FILE(GLOB KS_AUDIT_UI_SRC_LIST_UI "./*.ui")
FILE(GLOB_RECURSE KS_AUDIT_UI_SRC_LIST_RESOURCES "../data/*.qrc")
FILE(GLOB_RECURSE KS_AUDIT_UI_SRC_LIST_QSS "../data/*.qss")

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

ADD_EXECUTABLE(${TARGET_NAME}
    ${KS_AUDIT_UI_SRC_LIST_CPP}
    ${KS_AUDIT_UI_SRC_LIST_H}
    ${KS_AUDIT_UI_SRC_LIST_UI}
    ${KS_AUDIT_UI_SRC_LIST_RESOURCES}
    ${CONFIGUREINTERFACE_SRC}
    ${KS_AUDIT_UI_SRC_LIST_QSS}
    ${LICENSE_MANAGER_GENERATED_CPP_FILES}
    ${LICENSE_OBJECT_GENERATED_CPP_FILES})

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${KYLIN_LICENSE_INCLUDE_DIRS})

target_link_libraries(${TARGET_NAME}
    -lavformat
    -lavcodec
    -lavutil
    -lklog-qt5
    -lqrencode
    Qt5::Gui
    Qt5::Widgets
    Qt5::DBus
    Qt5::Core)

target_compile_definitions(${TARGET_NAME} PRIVATE
    -DRECORD_SYSTEM_DIR="${CMAKE_INSTALL_FULL_DATADIR}/ks-vaudit"
    )

set(AUDIT_UI_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
install(TARGETS ${TARGET_NAME} DESTINATION ${AUDIT_UI_BIN_DIR}/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
