# translations for 'ks-vaudit' executable

if(WITH_QT5)
	find_package(Qt5LinguistTools REQUIRED)
	set(LRELEASE Qt5::lrelease)
else()
	find_program(LRELEASE NAMES lrelease-qt4 lrelease)
endif()

if(LRELEASE)

	set(translations
		ar
		bg
		cs
		de
		el
		es
		fr
		gl
		he
		hu
		id
		it
		ja
		lt
		nl
		pl
		pt_BR
		ru
		sr
		sv
		uk
		zh_CN
		zh_TW
	)

	set(qm_files)

	foreach(tr IN LISTS translations)

		# qm generation rules
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ks-vaudit_${tr}.qm
			COMMAND ${LRELEASE} -silent ${PROJECT_SOURCE_DIR}/src/translations/ks-vaudit_${tr}.ts -qm ${CMAKE_CURRENT_BINARY_DIR}/ks-vaudit_${tr}.qm
			DEPENDS ${PROJECT_SOURCE_DIR}/src/translations/ks-vaudit_${tr}.ts
		)

		# installation rules
		install(
			FILES ${CMAKE_CURRENT_BINARY_DIR}/ks-vaudit_${tr}.qm
			DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/ks-vaudit/translations
		)

		# remember qm files
		list(APPEND qm_files
			${CMAKE_CURRENT_BINARY_DIR}/ks-vaudit_${tr}.qm
		)

	endforeach()

	# main target
	add_custom_target(translations ALL
		DEPENDS ${qm_files}
	)

endif()
