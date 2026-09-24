add_library(astra-ui
    src/Types.cpp
    src/SpatialUIComponent.cpp
    src/UIComponentRegistry.cpp
    src/SpatialWindowManager.cpp
    src/SpatialLayoutEngine.cpp
    src/SpatialFocusManager.cpp
    src/InputRouter.cpp
    src/InteractionStateMachine.cpp
    src/TaskSurface.cpp
    src/NotificationSurface.cpp
    src/PrivacyAwareUIService.cpp
    src/UIStateStore.cpp
    src/UIAnimationCoordinator.cpp
    src/AccessibilitySemanticService.cpp
)
target_include_directories(astra-ui PUBLIC include)
target_link_libraries(astra-ui PUBLIC Qt6::Core astra-common astra-policy astra-render)
target_compile_features(astra-ui PUBLIC cxx_std_20)
target_compile_options(astra-ui PRIVATE -Wall -Wextra -Wpedantic -Werror)

if(BUILD_TESTING)
    add_executable(astra-ui-component-tests tests/component_lifecycle_test.cpp)
    target_link_libraries(astra-ui-component-tests PRIVATE astra-ui)
    add_test(NAME astra-ui-component-tests COMMAND astra-ui-component-tests)
    add_executable(astra-ui-window-tests tests/spatial_window_manager_test.cpp)
    target_link_libraries(astra-ui-window-tests PRIVATE astra-ui)
    add_test(NAME astra-ui-window-tests COMMAND astra-ui-window-tests)
    add_executable(astra-ui-layout-tests tests/spatial_layout_engine_test.cpp)
    target_link_libraries(astra-ui-layout-tests PRIVATE astra-ui)
    add_test(NAME astra-ui-layout-tests COMMAND astra-ui-layout-tests)
    add_executable(astra-ui-focus-tests tests/spatial_focus_manager_test.cpp)
    target_link_libraries(astra-ui-focus-tests PRIVATE astra-ui)
    add_test(NAME astra-ui-focus-tests COMMAND astra-ui-focus-tests)
    add_executable(astra-ui-input-tests tests/input_router_test.cpp)
    target_link_libraries(astra-ui-input-tests PRIVATE astra-ui)
    add_test(NAME astra-ui-input-tests COMMAND astra-ui-input-tests)
    add_executable(astra-ui-interaction-tests tests/interaction_state_machine_test.cpp)
    target_link_libraries(astra-ui-interaction-tests PRIVATE astra-ui)
    add_test(NAME astra-ui-interaction-tests COMMAND astra-ui-interaction-tests)
    add_executable(astra-ui-task-surface-tests tests/task_surface_test.cpp)
    target_link_libraries(astra-ui-task-surface-tests PRIVATE astra-ui)
    add_test(NAME astra-ui-task-surface-tests COMMAND astra-ui-task-surface-tests)
    add_executable(astra-ui-notification-tests tests/notification_surface_test.cpp)
    target_link_libraries(astra-ui-notification-tests PRIVATE astra-ui)
    add_test(NAME astra-ui-notification-tests COMMAND astra-ui-notification-tests)
    add_executable(astra-ui-privacy-tests tests/privacy_aware_ui_test.cpp)
    target_link_libraries(astra-ui-privacy-tests PRIVATE astra-ui)
    add_test(NAME astra-ui-privacy-tests COMMAND astra-ui-privacy-tests)
    add_executable(astra-ui-state-store-tests tests/ui_state_store_test.cpp)
    target_link_libraries(astra-ui-state-store-tests PRIVATE astra-ui)
    add_test(NAME astra-ui-state-store-tests COMMAND astra-ui-state-store-tests)
    add_executable(astra-ui-animation-accessibility-tests tests/animation_accessibility_test.cpp)
    target_link_libraries(astra-ui-animation-accessibility-tests PRIVATE astra-ui)
    add_test(NAME astra-ui-animation-accessibility-tests COMMAND astra-ui-animation-accessibility-tests)

    add_executable(astra-ui-p5-performance-benchmark tests/p5_performance_benchmark.cpp)
    target_link_libraries(astra-ui-p5-performance-benchmark PRIVATE astra-ui)

    add_executable(astra-ui-p5-stability-test tests/p5_stability_test.cpp)
    target_link_libraries(astra-ui-p5-stability-test PRIVATE astra-ui)
endif()
