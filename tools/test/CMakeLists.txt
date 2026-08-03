qt_add_executable(remote_probe remote_probe.cpp)

set_target_properties(remote_probe PROPERTIES
    AUTOMOC ON
    WIN32_EXECUTABLE FALSE
)

target_link_libraries(remote_probe PRIVATE tm_core tm_session Qt6::Core Qt6::Network)

opentm_target_speedup(remote_probe NETWORK)
