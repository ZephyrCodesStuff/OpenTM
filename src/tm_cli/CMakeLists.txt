qt_add_executable(opentm_cli
    main.cpp
)

set_target_properties(opentm_cli PROPERTIES
    AUTOMOC ON
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(opentm_cli
    PRIVATE
        tm_core
        tm_session
        Qt6::Core
        Qt6::Network
)

opentm_target_speedup(opentm_cli NETWORK)