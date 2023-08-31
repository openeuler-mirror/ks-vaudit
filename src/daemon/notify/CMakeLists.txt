cmake_minimum_required(VERSION 3.5)
project(ks-vaudit-notify)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INCLUDE_DIRS "./include")
set(TARGET_NAME ks-vaudit-notify)
file(GLOB_RECURSE NOTIFY_H_FILES ./*.h)
file(GLOB NOTIFY_SRC "./*cpp" "./include/*.h")

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

add_executable(${TARGET_NAME}
    ${NOTIFY_SRC})

set(GLIB_INCLUDE_DIRS "/usr/include/glib-2.0" "/usr/lib64/glib-2.0/include")
set(GTK_INCLUDE_DIRS "/usr/include/gtk-2.0" "/usr/include/cairo" "/usr/include/pango-1.0" "/usr/lib64/gtk-2.0/include/"
                     "/usr/include/gdk-pixbuf-2.0/" "/usr/include/atk-1.0/")

target_include_directories(${TARGET_NAME} PRIVATE
        ${INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
	${GTK_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${TARGET_NAME}
        klog-qt5
        Qt5::Core
        Qt5::Gui
	Qt5::Widgets
        glib-2.0
	gobject-2.0
	notify
        pthread
	klog-qt5)

set(NOTIFY_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
install(TARGETS ${TARGET_NAME} DESTINATION ${NOTIFY_BIN_DIR})
