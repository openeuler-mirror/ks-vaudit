cmake_minimum_required(VERSION 2.8)

set(DATA_DIR "${PROJECT_SOURCE_DIR}/src/daemon/configure/data")
#set(DATA_DIR "${PROJECT_SOURCE_DIR}/data")

install(FILES ${DATA_DIR}/com.kylinsec.Kiran.VauditConfigure.conf
              ${DATA_DIR}/com.kylinsec.Kiran.VauditConfigure.conf
        DESTINATION /etc/dbus-1/system.d)

#install(FILES ${DATA_DIR}/com.kylinsec.Kiran.VauditConfigure.service
#              ${DATA_DIR}/com.kylinsec.Kiran.VauditConfigure.service
#        DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/dbus-1/system-services)


install(FILES ${DATA_DIR}/ks-vaudit-configure
              ${DATA_DIR}/ks-vaudit-configure
        DESTINATION /etc/init.d/)
