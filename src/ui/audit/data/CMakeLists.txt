set(AUDIT_UI_ICON_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor)

install(FILES "images/a16px.png"
        DESTINATION ${AUDIT_UI_ICON_INSTALL_PATH}/16x16/apps RENAME ks-vaudit-audit.png)

install(FILES "images/a22px.png"
        DESTINATION ${AUDIT_UI_ICON_INSTALL_PATH}/22x22/apps RENAME ks-vaudit-audit.png)

install(FILES "images/a24px.png"
        DESTINATION ${AUDIT_UI_ICON_INSTALL_PATH}/24x24/apps RENAME ks-vaudit-audit.png)

install(FILES "images/a32px.png"
        DESTINATION ${AUDIT_UI_ICON_INSTALL_PATH}/32x32/apps RENAME ks-vaudit-audit.png)

install(FILES "images/a48px.png"
        DESTINATION ${AUDIT_UI_ICON_INSTALL_PATH}/48x48/apps RENAME ks-vaudit-audit.png)

install(FILES "images/a64px.png"
        DESTINATION ${AUDIT_UI_ICON_INSTALL_PATH}/64x64/apps RENAME ks-vaudit-audit.png)

install(FILES "images/a96px.png"
        DESTINATION ${AUDIT_UI_ICON_INSTALL_PATH}/96x96/apps RENAME ks-vaudit-audit.png)

install(FILES "images/a128px.png"
        DESTINATION ${AUDIT_UI_ICON_INSTALL_PATH}/128x128/apps RENAME ks-vaudit-audit.png)

install(FILES "images/24pxaudit.svg"
        DESTINATION ${AUDIT_UI_ICON_INSTALL_PATH}/scalable/apps RENAME ks-vaudit-audit.svg)

set(AUDIT_UI_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})

configure_file("ks-vaudit-audit.desktop.in"
"${CMAKE_CURRENT_BINARY_DIR}/ks-vaudit-audit.desktop")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ks-vaudit-audit.desktop"
DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/applications/ )
