add_library(tm_launch STATIC
    launch.cpp
    launch.h
    single_instance.cpp
    single_instance.h
)

set_target_properties(tm_launch PROPERTIES AUTOMOC ON)

target_include_directories(tm_launch PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(tm_launch
    PUBLIC
        Qt6::Core
        Qt6::Network
)

opentm_target_speedup(tm_launch NETWORK)