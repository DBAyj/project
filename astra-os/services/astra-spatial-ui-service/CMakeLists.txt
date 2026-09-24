find_package(Qt6 6.8 REQUIRED COMPONENTS Network)

add_library(astra-spatial-ui-service-core
    src/ProjectionGateway.cpp
    src/SpatialUIConfiguration.cpp
    src/SpatialUIRuntime.cpp
    src/SpatialUIService.cpp
)
target_include_directories(astra-spatial-ui-service-core PUBLIC include)
target_link_libraries(astra-spatial-ui-service-core PUBLIC Qt6::Core Qt6::Network astra-ui astra-projection-runtime)
target_compile_features(astra-spatial-ui-service-core PUBLIC cxx_std_20)
target_compile_options(astra-spatial-ui-service-core PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(astra-spatial-ui-service src/main.cpp)
target_link_libraries(astra-spatial-ui-service PRIVATE astra-spatial-ui-service-core)
target_compile_features(astra-spatial-ui-service PRIVATE cxx_std_20)
target_compile_options(astra-spatial-ui-service PRIVATE -Wall -Wextra -Wpedantic -Werror)

if(BUILD_TESTING)
    add_executable(astra-spatial-ui-service-tests tests/spatial_ui_service_test.cpp)
    target_link_libraries(astra-spatial-ui-service-tests PRIVATE astra-spatial-ui-service-core)
    add_test(NAME astra-spatial-ui-service-tests COMMAND astra-spatial-ui-service-tests)
    add_executable(astra-spatial-ui-runtime-projection-tests tests/spatial_ui_runtime_projection_test.cpp)
    target_link_libraries(astra-spatial-ui-runtime-projection-tests PRIVATE astra-spatial-ui-service-core)
    add_test(NAME astra-spatial-ui-runtime-projection-tests COMMAND astra-spatial-ui-runtime-projection-tests)
    add_executable(astra-spatial-ui-runtime-state-tests tests/spatial_ui_runtime_state_test.cpp)
    target_link_libraries(astra-spatial-ui-runtime-state-tests PRIVATE astra-spatial-ui-service-core)
    add_test(NAME astra-spatial-ui-runtime-state-tests COMMAND astra-spatial-ui-runtime-state-tests)
    add_executable(astra-spatial-ui-runtime-command-tests tests/spatial_ui_runtime_command_test.cpp)
    target_link_libraries(astra-spatial-ui-runtime-command-tests PRIVATE astra-spatial-ui-service-core)
    add_test(NAME astra-spatial-ui-runtime-command-tests COMMAND astra-spatial-ui-runtime-command-tests)
    add_executable(astra-spatial-ui-configuration-tests tests/spatial_ui_configuration_test.cpp)
    target_link_libraries(astra-spatial-ui-configuration-tests PRIVATE astra-spatial-ui-service-core)
    target_compile_definitions(astra-spatial-ui-configuration-tests PRIVATE ASTRA_TEST_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
    add_test(NAME astra-spatial-ui-configuration-tests COMMAND astra-spatial-ui-configuration-tests)
endif()
