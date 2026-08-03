include(GNUInstallDirs)

set(OPENTM_RUNTIME_TARGETS opentm_app opentm_cli opentm_server opentm_tray)

if(UNIX AND NOT APPLE)
    set_target_properties(${OPENTM_RUNTIME_TARGETS} PROPERTIES
        INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH FALSE
    )
endif()

install(TARGETS ${OPENTM_RUNTIME_TARGETS}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE  DESTINATION .
)

if(UNIX AND NOT APPLE)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/linux/opentm.desktop"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    )
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/linux/opentm.png"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps
    )
endif()
