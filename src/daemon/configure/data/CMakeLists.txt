cmake_minimum_required(VERSION 2.8)

install(FILES ${PROJECT_SOURCE_DIR}/src/daemon/configure/data/com.kylinsec.Kiran.VauditConfigure.conf
              ${PROJECT_SOURCE_DIR}/src/daemon/configure/data/com.kylinsec.Kiran.VauditConfigure.conf
        DESTINATION /etc/dbus-1/system.d)

install(FILES ${PROJECT_SOURCE_DIR}/src/daemon/configure/data/com.kylinsec.Kiran.VauditConfigure.service
              ${PROJECT_SOURCE_DIR}/src/daemon/configure/data/com.kylinsec.Kiran.VauditConfigure.service
        DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/dbus-1/system-services)
