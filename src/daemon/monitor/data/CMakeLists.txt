cmake_minimum_required(VERSION 2.8)

set(MONITOR_DATA_DIR "${PROJECT_SOURCE_DIR}/src/daemon/monitor/data")
#set(DATA_DIR "${PROJECT_SOURCE_DIR}/data")

install(FILES ${MONITOR_DATA_DIR}/ks-vaudit-monitor
              ${MONITOR_DATA_DIR}/ks-vaudit-monitor
        DESTINATION /etc/init.d/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
