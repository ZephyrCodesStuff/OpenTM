qt_add_executable(opentm_app
    main.cpp
    main_window.cpp
    main_window.h
    add_target_dialog.cpp
    add_target_dialog.h
    about_dialog.cpp
    about_dialog.h
    find_dialog.cpp
    find_dialog.h
    shortcut_settings_dialog.cpp
    shortcut_settings_dialog.h
    file_properties_dialog.cpp
    file_properties_dialog.h
    preferences_dialog.cpp
    preferences_dialog.h
    app_style.cpp
    app_style.h
    app_prefs.cpp
    app_prefs.h
    discovery_dialog.cpp
    discovery_dialog.h
    load_executable_dialog.cpp
    load_executable_dialog.h
    target_properties_dialog.cpp
    target_properties_dialog.h
    target_properties_xml.cpp
    target_properties_xml.h
    tty_channel.h

    controllers/load_controller.cpp
    controllers/load_controller.h

    panels/file_explorer_panel.cpp
    panels/file_explorer_panel.h
    panels/kernel_explorer_panel.cpp
    panels/kernel_explorer_panel.h
    panels/target_panel.cpp
    panels/target_panel.h
    panels/wire_log_panel.cpp
    panels/wire_log_panel.h
    panels/cp_panel.cpp
    panels/cp_panel.h
    panels/console_panel.cpp
    panels/console_panel.h

    icons.qrc
)

set_target_properties(opentm_app PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(opentm_app
    PRIVATE
        tm_core
        tm_session
        tm_launch
        Qt6::Core
        Qt6::Network
        Qt6::Widgets
)

target_include_directories(opentm_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

opentm_use_build_info(opentm_app)
opentm_target_speedup(opentm_app NETWORK WIDGETS)

if(WIN32)
    get_target_property(_qt_moc_path Qt6::moc IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_moc_path}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt windeployqt.exe
        HINTS
            "${_qt_bin_dir}"
            "${Qt6_DIR}/../../../bin"
            "${Qt6_DIR}/../../../tools/Qt6/bin"
            "${Qt6_DIR}/../../../tools/qt6/bin"
        DOC "Path to windeployqt"
    )
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET opentm_app POST_BUILD
            COMMAND "${CMAKE_COMMAND}"
                    "-DDEPLOY_TOOL=${WINDEPLOYQT_EXECUTABLE}"
                    "-DAPP_BINARY=$<TARGET_FILE:opentm_app>"
                    -P "${CMAKE_SOURCE_DIR}/cmake/DeployQt.cmake"
            COMMENT "Deploying Qt runtime into bin/ via windeployqt"
            VERBATIM
        )
    else()
        message(WARNING "windeployqt not found - opentm_app may fail to launch from the build tree. " "Set Qt6_DIR or run windeployqt manually against the built exe.")
    endif()

    foreach(_plugin Qt6::QWindowsIntegrationPlugin Qt6::QModernWindowsStylePlugin)
        if(TARGET ${_plugin})
            get_target_property(_plugin_type ${_plugin} QT_PLUGIN_TYPE)
            add_custom_command(TARGET opentm_app POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E make_directory
                        "$<TARGET_FILE_DIR:opentm_app>/${_plugin_type}"
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        "$<TARGET_FILE:${_plugin}>"
                        "$<TARGET_FILE_DIR:opentm_app>/${_plugin_type}/"
                VERBATIM
            )
        endif()
    endforeach()
endif()