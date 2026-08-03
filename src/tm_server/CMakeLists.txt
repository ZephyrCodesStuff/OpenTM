qt_add_executable(opentm_server
    main.cpp
    rpc_server.cpp
    rpc_server.h
)

set_target_properties(opentm_server PROPERTIES
    AUTOMOC ON
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(opentm_server
    PRIVATE
        tm_core
        tm_session
        Qt6::Core
        Qt6::Network
)

opentm_target_speedup(opentm_server NETWORK)