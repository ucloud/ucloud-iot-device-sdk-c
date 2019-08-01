include_directories(
        include
        ${CMAKE_SOURCE_DIR}/external_libs/googletest/include
)

link_libraries(gtest iot_sdk iot_platform)

if (ENABLE_FEATURE_MQTT_COMM)
    add_executable(unit_mqtt_test src/unit_mqtt_test.cpp)
    set_target_properties(unit_mqtt_test PROPERTIES LINKER_LANGUAGE CXX)
endif ()

if (ENABLE_FEATURE_DEVICE_SHADOW)
    add_executable(unit_shadow_test src/unit_shadow_test.cpp)
    set_target_properties(unit_shadow_test PROPERTIES LINKER_LANGUAGE CXX)
endif ()

if (ENABLE_FEATURE_OTA)
    add_executable(unit_http_test src/unit_http_test.cpp)
    set_target_properties(unit_http_test PROPERTIES LINKER_LANGUAGE CXX)
endif ()

if (ENABLE_FEATURE_AUTH_MODE_DYNAMIC)
    add_executable(unit_dynamic_auth_test src/unit_dynamic_auth_test.cpp)
    set_target_properties(unit_dynamic_auth_test PROPERTIES LINKER_LANGUAGE CXX)
endif ()