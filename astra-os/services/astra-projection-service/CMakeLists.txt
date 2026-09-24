find_package(Qt6 6.8 REQUIRED COMPONENTS Network)

add_library(astra-projection-service-core src/ProjectionService.cpp)
target_include_directories(astra-projection-service-core PUBLIC include)
target_link_libraries(astra-projection-service-core PUBLIC Qt6::Core Qt6::Network astra-projection-runtime astra-render astra-policy)
target_compile_features(astra-projection-service-core PUBLIC cxx_std_20)
target_compile_options(astra-projection-service-core PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(astra-projection-service src/main.cpp)
target_link_libraries(astra-projection-service PRIVATE astra-projection-service-core)
target_compile_features(astra-projection-service PRIVATE cxx_std_20)
target_compile_options(astra-projection-service PRIVATE -Wall -Wextra -Wpedantic -Werror)

if(BUILD_TESTING)
    add_executable(astra-projection-service-tests tests/projection_service_test.cpp)
    target_link_libraries(astra-projection-service-tests PRIVATE astra-projection-service-core)
    add_test(NAME astra-projection-service-tests COMMAND astra-projection-service-tests)
endif()
