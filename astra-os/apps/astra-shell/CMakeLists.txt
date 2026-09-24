find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Network Qml Quick Quick3D QuickControls2)
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

add_library(astra-shell-core)
set_target_properties(astra-shell-core PROPERTIES AUTOMOC ON)
target_sources(astra-shell-core PRIVATE
    src/clients/IntentServiceClient.cpp
    src/clients/SpatialServiceClient.cpp
    src/models/IntentResultModel.cpp
    src/models/ProjectionStateModel.cpp
    src/models/SystemStateModel.cpp
    src/models/SpatialStateModel.cpp
    src/models/TaskModel.cpp
    src/models/SpatialUIStateModel.cpp
    src/services/ConfigurationService.cpp
    src/services/IntentSimulator.cpp
    src/services/ProjectionSessionService.cpp
    src/clients/ProjectionServiceClient.cpp
    src/clients/SpatialUIServiceClient.cpp
    src/services/AuditLogService.cpp
)
target_include_directories(astra-shell-core PUBLIC src)
target_link_libraries(astra-shell-core PUBLIC Qt6::Core Qt6::Network astra-common astra-policy)
target_compile_features(astra-shell-core PUBLIC cxx_std_20)
target_compile_options(astra-shell-core PRIVATE -Wall -Wextra -Wpedantic -Werror)

qt_add_executable(astra-shell src/main.cpp src/application/AstraApplication.cpp src/controllers/ShellController.cpp)
set_target_properties(astra-shell PROPERTIES AUTOMOC ON)
target_include_directories(astra-shell PRIVATE src)
target_link_libraries(astra-shell PRIVATE astra-shell-core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Quick3D Qt6::QuickControls2)
set_source_files_properties(qml/theme/AstraTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/spatial/AstraSpatialTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(astra-shell
    URI Astra.Shell
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/AstraButton.qml
        qml/components/AstraCard.qml
        qml/components/AstraErrorBanner.qml
        qml/components/AstraPrivacyBadge.qml
        qml/components/AstraStatusBadge.qml
        qml/components/AstraTextField.qml
        qml/components/AstraSpatialButton.qml
        qml/components/AstraSpatialCard.qml
        qml/components/AstraSpatialPanel.qml
        qml/components/AstraSpatialSlider.qml
        qml/components/AstraSpatialToggle.qml
        qml/phone/PhoneHeader.qml
        qml/phone/IntentAnalysisCard.qml
        qml/phone/ConfirmationPanel.qml
        qml/phone/PhoneWindow.qml
        qml/phone/PrivacySelector.qml
        qml/phone/RecentTasksPanel.qml
        qml/phone/SystemStatusPanel.qml
        qml/phone/SpatialStatusPanel.qml
        qml/phone/TaskInputPanel.qml
        qml/phone/TaskResultCard.qml
        qml/projection/ProjectionControls.qml
        qml/projection/ProjectionEmptyState.qml
        qml/projection/ProjectionHeader.qml
        qml/projection/ProjectionScene.qml
        qml/projection/ProjectionWindow.qml
        qml/spatial/AstraSpatialTheme.qml
        qml/spatial/SpatialComponentView.qml
        qml/spatial/SpatialWorkspace.qml
        qml/spatial/SpatialWindowView.qml
        qml/spatial/SpatialTaskCard.qml
        qml/spatial/SpatialSystemPanel.qml
        qml/spatial/SpatialNotificationView.qml
        qml/spatial/SpatialFocusIndicator.qml
        qml/spatial/SpatialPrivacyBadge.qml
        qml/spatial/SpatialContextMenu.qml
        qml/spatial/SpatialDebugOverlay.qml
        qml/theme/AstraTheme.qml
    RESOURCES qml/theme/qmldir qml/spatial/qmldir
)

if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
    add_executable(astra-shell-projection-policy-tests tests/unit/projection_policy_service_test.cpp)
    target_link_libraries(astra-shell-projection-policy-tests PRIVATE astra-shell-core)
    add_test(NAME astra-shell-projection-policy-tests COMMAND astra-shell-projection-policy-tests)
    add_executable(astra-shell-application-tests tests/unit/astra_application_test.cpp src/application/AstraApplication.cpp src/controllers/ShellController.cpp)
    set_target_properties(astra-shell-application-tests PROPERTIES AUTOMOC ON)
    target_link_libraries(astra-shell-application-tests PRIVATE astra-shell-core Qt6::Gui Qt6::Qml Qt6::Quick)
    add_test(NAME astra-shell-application-tests COMMAND astra-shell-application-tests)
    set_tests_properties(astra-shell-application-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Basic")
    add_executable(astra-shell-controller-integration-tests tests/integration/shell_controller_integration_test.cpp src/controllers/ShellController.cpp)
    set_target_properties(astra-shell-controller-integration-tests PROPERTIES AUTOMOC ON)
    target_link_libraries(astra-shell-controller-integration-tests PRIVATE astra-shell-core)
    add_test(NAME astra-shell-controller-integration-tests COMMAND astra-shell-controller-integration-tests)
    add_executable(astra-shell-window-interaction-tests tests/integration/window_interaction_application_test.cpp src/controllers/ShellController.cpp)
    set_target_properties(astra-shell-window-interaction-tests PROPERTIES AUTOMOC ON)
    target_compile_definitions(astra-shell-window-interaction-tests PRIVATE ASTRA_QML_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml")
    target_link_libraries(astra-shell-window-interaction-tests PRIVATE astra-shell-core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Quick3D Qt6::QuickControls2 Qt6::Test)
    add_test(NAME astra-shell-window-interaction-tests COMMAND astra-shell-window-interaction-tests)
    set_tests_properties(astra-shell-window-interaction-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Basic")
    add_executable(astra-shell-state-model-tests tests/unit/state_models_test.cpp)
    target_link_libraries(astra-shell-state-model-tests PRIVATE astra-shell-core)
    add_test(NAME astra-shell-state-model-tests COMMAND astra-shell-state-model-tests)
    add_executable(astra-shell-config-tests tests/unit/configuration_service_test.cpp)
    target_compile_definitions(astra-shell-config-tests PRIVATE ASTRA_SCHEMA_PATH="${CMAKE_SOURCE_DIR}/schemas/application-config.schema.json")
    target_link_libraries(astra-shell-config-tests PRIVATE astra-shell-core)
    add_test(NAME astra-shell-config-tests COMMAND astra-shell-config-tests)
    add_executable(astra-shell-intent-tests tests/unit/intent_simulator_test.cpp)
    target_link_libraries(astra-shell-intent-tests PRIVATE astra-shell-core)
    add_test(NAME astra-shell-intent-tests COMMAND astra-shell-intent-tests)
    add_executable(astra-shell-projection-tests tests/unit/projection_session_service_test.cpp)
    target_link_libraries(astra-shell-projection-tests PRIVATE astra-shell-core)
    add_test(NAME astra-shell-projection-tests COMMAND astra-shell-projection-tests)
    add_executable(astra-shell-projection-service-client-tests tests/unit/projection_service_client_test.cpp)
    target_link_libraries(astra-shell-projection-service-client-tests PRIVATE astra-shell-core)
    add_test(NAME astra-shell-projection-service-client-tests COMMAND astra-shell-projection-service-client-tests)
    add_executable(astra-shell-p4-projection-integration-tests tests/integration/p4_projection_integration_test.cpp)
    target_link_libraries(astra-shell-p4-projection-integration-tests PRIVATE astra-shell-core astra-projection-service-core)
    add_test(NAME astra-shell-p4-projection-integration-tests COMMAND astra-shell-p4-projection-integration-tests)
    add_executable(astra-shell-audit-tests tests/unit/audit_log_service_test.cpp)
    target_link_libraries(astra-shell-audit-tests PRIVATE astra-shell-core)
    add_test(NAME astra-shell-audit-tests COMMAND astra-shell-audit-tests)
    add_executable(astra-shell-spatial-ui-client-model-tests tests/unit/spatial_ui_client_model_test.cpp)
    target_link_libraries(astra-shell-spatial-ui-client-model-tests PRIVATE astra-shell-core)
    add_test(NAME astra-shell-spatial-ui-client-model-tests COMMAND astra-shell-spatial-ui-client-model-tests)
    add_executable(astra-shell-p5-spatial-ui-integration-tests tests/integration/p5_spatial_ui_integration_test.cpp)
    target_compile_definitions(astra-shell-p5-spatial-ui-integration-tests PRIVATE
        ASTRA_P4_SERVICE_BINARY="$<TARGET_FILE:astra-projection-service>"
        ASTRA_P5_SERVICE_BINARY="$<TARGET_FILE:astra-spatial-ui-service>"
        ASTRA_TEST_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
    target_link_libraries(astra-shell-p5-spatial-ui-integration-tests PRIVATE astra-shell-core Qt6::Network)
    add_dependencies(astra-shell-p5-spatial-ui-integration-tests astra-projection-service astra-spatial-ui-service)
    add_test(NAME astra-shell-p5-spatial-ui-integration-tests COMMAND astra-shell-p5-spatial-ui-integration-tests)
    add_executable(astra-shell-p5-spatial-target-chain-tests tests/integration/p5_spatial_target_chain_integration_test.cpp)
    target_compile_definitions(astra-shell-p5-spatial-target-chain-tests PRIVATE
        ASTRA_P3_SERVICE_BINARY="$<TARGET_FILE:astra-spatial-service>"
        ASTRA_P4_SERVICE_BINARY="$<TARGET_FILE:astra-projection-service>"
        ASTRA_P5_SERVICE_BINARY="$<TARGET_FILE:astra-spatial-ui-service>"
        ASTRA_TEST_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
    target_link_libraries(astra-shell-p5-spatial-target-chain-tests PRIVATE astra-shell-core Qt6::Network)
    add_dependencies(astra-shell-p5-spatial-target-chain-tests astra-spatial-service astra-projection-service astra-spatial-ui-service)
    add_test(NAME astra-shell-p5-spatial-target-chain-tests COMMAND astra-shell-p5-spatial-target-chain-tests)
    # Starts real P3/P4/P5 services with readiness timeouts; parallel CTest load would skew them.
    set_tests_properties(astra-shell-p5-spatial-target-chain-tests PROPERTIES RUN_SERIAL TRUE)
    add_executable(astra-shell-intent-client-tests tests/integration/intent_service_client_test.cpp src/clients/IntentServiceClient.cpp)
    target_include_directories(astra-shell-intent-client-tests PRIVATE src)
    target_link_libraries(astra-shell-intent-client-tests PRIVATE astra-common Qt6::Core Qt6::Network)
    add_test(NAME astra-shell-intent-client-tests COMMAND astra-shell-intent-client-tests)
    add_executable(astra-shell-intent-result-model-tests tests/unit/intent_result_model_test.cpp src/models/IntentResultModel.cpp)
    set_target_properties(astra-shell-intent-result-model-tests PROPERTIES AUTOMOC ON)
    target_include_directories(astra-shell-intent-result-model-tests PRIVATE src)
    target_link_libraries(astra-shell-intent-result-model-tests PRIVATE Qt6::Core)
    add_test(NAME astra-shell-intent-result-model-tests COMMAND astra-shell-intent-result-model-tests)
    add_executable(astra-shell-p2-controller-integration-tests tests/integration/p2_shell_controller_integration_test.cpp src/controllers/ShellController.cpp)
    set_target_properties(astra-shell-p2-controller-integration-tests PROPERTIES AUTOMOC ON)
    target_include_directories(astra-shell-p2-controller-integration-tests PRIVATE src)
    target_link_libraries(astra-shell-p2-controller-integration-tests PRIVATE astra-shell-core Qt6::Core Qt6::Network)
    add_test(NAME astra-shell-p2-controller-integration-tests COMMAND astra-shell-p2-controller-integration-tests)
    add_executable(astra-shell-p3-spatial-safety-tests tests/integration/p3_spatial_safety_integration_test.cpp src/controllers/ShellController.cpp)
    set_target_properties(astra-shell-p3-spatial-safety-tests PROPERTIES AUTOMOC ON)
    target_include_directories(astra-shell-p3-spatial-safety-tests PRIVATE src)
    target_link_libraries(astra-shell-p3-spatial-safety-tests PRIVATE astra-shell-core Qt6::Core Qt6::Network)
    add_test(NAME astra-shell-p3-spatial-safety-tests COMMAND astra-shell-p3-spatial-safety-tests)
endif()
