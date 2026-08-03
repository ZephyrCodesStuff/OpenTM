qt_add_executable(opentm_tray
    main.cpp
    tray_supervisor.cpp
    tray_supervisor.h
    ${CMAKE_SOURCE_DIR}/src/tm_ui/about_dialog.cpp
    ${CMAKE_SOURCE_DIR}/src/tm_ui/app_style.cpp
    ${CMAKE_SOURCE_DIR}/src/tm_ui/icons.qrc
)

set_target_properties(opentm_tray PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(opentm_tray
    PRIVATE
        tm_launch
        Qt6::Core
        Qt6::Network
        Qt6::Widgets
)

target_include_directories(opentm_tray PRIVATE ${CMAKE_SOURCE_DIR}/src)

opentm_use_build_info(opentm_tray)
opentm_target_speedup(opentm_tray NETWORK WIDGETS)