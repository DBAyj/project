find_package(OpenCV REQUIRED COMPONENTS calib core geometry imgcodecs imgproc videoio)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Multimedia Network)

add_library(astra-spatial-core STATIC)
target_sources(astra-spatial-core PRIVATE
    src/domain/SpatialTypes.cpp
    src/input/FrameSource.cpp
    src/application/SpatialService.cpp
    src/transport/SpatialJsonRpcServer.cpp
    src/vision/SurfaceDetector.cpp
    src/vision/CalibrationService.cpp
)
target_include_directories(astra-spatial-core PUBLIC src)
target_link_libraries(astra-spatial-core PUBLIC astra-common Qt6::Core Qt6::Multimedia ${OpenCV_LIBS})
target_compile_features(astra-spatial-core PUBLIC cxx_std_20)
target_compile_options(astra-spatial-core PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(astra-spatial-service src/main.cpp)
target_link_libraries(astra-spatial-service PRIVATE astra-spatial-core Qt6::Core Qt6::Network)
target_compile_options(astra-spatial-service PRIVATE -Wall -Wextra -Wpedantic -Werror)

if(BUILD_TESTING)
    add_executable(astra-spatial-domain-tests tests/unit/spatial_domain_types_test.cpp)
    target_link_libraries(astra-spatial-domain-tests PRIVATE astra-spatial-core)
    add_test(NAME astra-spatial-domain-tests COMMAND astra-spatial-domain-tests)

    add_executable(astra-spatial-frame-source-tests tests/unit/frame_source_test.cpp)
    target_link_libraries(astra-spatial-frame-source-tests PRIVATE astra-spatial-core)
    target_compile_definitions(astra-spatial-frame-source-tests PRIVATE ASTRA_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")
    add_test(NAME astra-spatial-frame-source-tests COMMAND astra-spatial-frame-source-tests)

    add_executable(astra-spatial-vision-tests tests/unit/surface_detector_test.cpp)
    target_link_libraries(astra-spatial-vision-tests PRIVATE astra-spatial-core)
    target_compile_definitions(astra-spatial-vision-tests PRIVATE ASTRA_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")
    add_test(NAME astra-spatial-vision-tests COMMAND astra-spatial-vision-tests)

    add_executable(astra-spatial-calibration-tests tests/unit/calibration_service_test.cpp)
    target_link_libraries(astra-spatial-calibration-tests PRIVATE astra-spatial-core)
    add_test(NAME astra-spatial-calibration-tests COMMAND astra-spatial-calibration-tests)

    add_executable(astra-spatial-service-tests tests/integration/spatial_service_test.cpp)
    target_link_libraries(astra-spatial-service-tests PRIVATE astra-spatial-core)
    target_compile_definitions(astra-spatial-service-tests PRIVATE ASTRA_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")
    add_test(NAME astra-spatial-service-tests COMMAND astra-spatial-service-tests)

    add_executable(astra-spatial-jsonrpc-tests tests/integration/spatial_jsonrpc_test.cpp)
    target_link_libraries(astra-spatial-jsonrpc-tests PRIVATE astra-spatial-core Qt6::Core Qt6::Network)
    target_compile_definitions(astra-spatial-jsonrpc-tests PRIVATE ASTRA_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")
    add_test(NAME astra-spatial-jsonrpc-tests COMMAND astra-spatial-jsonrpc-tests)

    add_executable(astra-spatial-camera-evidence-tests tests/integration/camera_adapter_evidence_test.cpp)
    target_link_libraries(astra-spatial-camera-evidence-tests PRIVATE astra-spatial-core Qt6::Core)
    target_compile_definitions(astra-spatial-camera-evidence-tests PRIVATE ASTRA_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")
    add_test(NAME astra-spatial-camera-evidence-tests COMMAND astra-spatial-camera-evidence-tests)

    add_executable(astra-spatial-performance-tests tests/performance/spatial_performance_test.cpp)
    target_link_libraries(astra-spatial-performance-tests PRIVATE astra-spatial-core)
    target_compile_definitions(astra-spatial-performance-tests PRIVATE ASTRA_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")
    add_test(NAME astra-spatial-performance-tests COMMAND astra-spatial-performance-tests)
    # Latency budgets are measured; parallel CTest load would skew them.
    set_tests_properties(astra-spatial-performance-tests PROPERTIES RUN_SERIAL TRUE)

    add_executable(astra-spatial-stability-tests tests/performance/spatial_stability_test.cpp)
    target_link_libraries(astra-spatial-stability-tests PRIVATE astra-spatial-core Qt6::Network)
    target_compile_definitions(astra-spatial-stability-tests PRIVATE ASTRA_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")
    add_test(NAME astra-spatial-stability-tests COMMAND astra-spatial-stability-tests)
endif()
