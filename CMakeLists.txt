cmake_minimum_required(VERSION 3.24)
project(CoreWatch VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(COREWATCH_BUILD_TESTS "Build CoreWatch C++ unit tests" ON)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /EHsc)
    add_compile_definitions(_UNICODE UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# 1. corewatch_core
add_library(corewatch_core STATIC
    src/core/Format.cpp
)
target_include_directories(corewatch_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(corewatch_core PUBLIC cxx_std_20)

# 2. corewatch_sensors
add_library(corewatch_sensors STATIC
    src/sensors/CpuSensorProvider.cpp
    src/sensors/MemorySensorProvider.cpp
    src/sensors/StorageSensorProvider.cpp
    src/sensors/NvmlGpuSensorProvider.cpp
)
target_link_libraries(corewatch_sensors PUBLIC
    corewatch_core
    pdh
    powrprof
)

# 3. corewatch_config
add_library(corewatch_config STATIC
    src/config/JsonSerializer.cpp
    src/config/ConfigStore.cpp
)
target_link_libraries(corewatch_config PUBLIC
    corewatch_core
    shell32
    ole32
    advapi32
)

# 4. corewatch_engine
add_library(corewatch_engine STATIC
    src/engine/AlertEvaluator.cpp
    src/engine/MonitoringEngine.cpp
)
target_link_libraries(corewatch_engine PUBLIC
    corewatch_core
    corewatch_sensors
    corewatch_config
)

if(COREWATCH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
