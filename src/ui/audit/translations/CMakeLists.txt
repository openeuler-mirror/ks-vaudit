
if (LRELEASE)
    set(translations_audit zh_CN)
    add_custom_command(
        OUTPUT ${AUDITSRCDIR}/ks-vaudit-audit.zh_CN.qm
        COMMAND ${LRELEASE} -silent ${CMAKE_CURRENT_SOURCE_DIR}/ks-vaudit-audit.zh_CN.ts -qm ${AUDITSRCDIR}/ks-vaudit-audit.zh_CN.qm
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ks-vaudit-audit.zh_CN.ts
    )

    set(QM_FILE_AUDIT ${AUDITSRCDIR}/ks-vaudit-audit.zh_CN.qm)

    install(FILES ${QM_FILE_AUDIT} DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/ks-vaudit/translations)

    # main target
    add_custom_target(translations_audit ALL
        DEPENDS ${QM_FILE_AUDIT}
    )

endif()
