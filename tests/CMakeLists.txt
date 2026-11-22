add_executable(corewatch_tests
    CoreWatchTests.cpp
)

target_link_libraries(corewatch_tests PRIVATE
    corewatch_core
    corewatch_sensors
    corewatch_config
    corewatch_engine
)

add_test(NAME CoreWatchUnitTests COMMAND corewatch_tests)
