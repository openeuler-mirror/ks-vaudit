set(RECORD_UI_ICON_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor)

install(FILES "images/16px.png"
	DESTINATION ${RECORD_UI_ICON_INSTALL_PATH}/16x16/apps RENAME ks-vaudit-record.png)

install(FILES "images/22px.png"
	DESTINATION ${RECORD_UI_ICON_INSTALL_PATH}/22x22/apps RENAME ks-vaudit-record.png)

install(FILES "images/24px.png"
	DESTINATION ${RECORD_UI_ICON_INSTALL_PATH}/24x24/apps RENAME ks-vaudit-record.png)

install(FILES "images/32px.png"
	DESTINATION ${RECORD_UI_ICON_INSTALL_PATH}/32x32/apps RENAME ks-vaudit-record.png)

install(FILES "images/48px.png"
	DESTINATION ${RECORD_UI_ICON_INSTALL_PATH}/48x48/apps RENAME ks-vaudit-record.png)

install(FILES "images/64px.png"
	DESTINATION ${RECORD_UI_ICON_INSTALL_PATH}/64x64/apps RENAME ks-vaudit-record.png)

install(FILES "images/96px.png"
	DESTINATION ${RECORD_UI_ICON_INSTALL_PATH}/96x96/apps RENAME ks-vaudit-record.png)

install(FILES "images/128px.png"
	DESTINATION ${RECORD_UI_ICON_INSTALL_PATH}/128x128/apps RENAME ks-vaudit-record.png)

install(FILES "images/24pxrecord.svg"
    DESTINATION ${RECORD_UI_ICON_INSTALL_PATH}/scalable/apps RENAME ks-vaudit-record.svg)

set(RECORD_UI_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})

configure_file("ks-vaudit-record.desktop.in"
"${CMAKE_CURRENT_BINARY_DIR}/ks-vaudit-record.desktop")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ks-vaudit-record.desktop"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/applications/ )
