
if (LRELEASE)
    set(translations_record zh_CN)
    add_custom_command(
        OUTPUT ${RECORDSRCDIR}/ks-vaudit-record.zh_CN.qm
        COMMAND ${LRELEASE} -silent ${CMAKE_CURRENT_SOURCE_DIR}/ks-vaudit-record.zh_CN.ts -qm ${RECORDSRCDIR}/ks-vaudit-record.zh_CN.qm
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ks-vaudit-record.zh_CN.ts
    )

    set(QM_FILE_RECORD ${RECORDSRCDIR}/ks-vaudit-record.zh_CN.qm)

    install(FILES ${QM_FILE_RECORD} DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/ks-vaudit/translations)

    # main target
    add_custom_target(translations_record ALL
        DEPENDS ${QM_FILE_RECORD}
    )

endif()
