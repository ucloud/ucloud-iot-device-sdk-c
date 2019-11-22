link_libraries(iot_sdk iot_platform)

if (ENABLE_FEATURE_MQTT_COMM)
    add_executable(mqtt_sample mqtt/mqtt_sample.c)
    add_dependencies(mqtt_sample iot_sdk iot_platform)
endif ()

if (ENABLE_FEATURE_DEVICE_SHADOW)
    add_executable(shadow_sample shadow/shadow_sample.c)
    add_dependencies(shadow_sample iot_sdk iot_platform)

    add_executable(smart_bracelet_heart_rate_shadow_sample shadow/smart_bracelet_heart_rate_shadow_sample.c)
    add_dependencies(smart_bracelet_heart_rate_shadow_sample iot_sdk iot_platform)

    add_executable(smart_bracelet_walk_step_shadow_sample shadow/smart_bracelet_walk_step_shadow_sample.c)
    add_dependencies(smart_bracelet_walk_step_shadow_sample iot_sdk iot_platform)
endif ()

if (ENABLE_FEATURE_OTA)
    add_executable(ota_sample ota/ota_sample.c)
    add_dependencies(ota_sample iot_sdk iot_platform)
endif ()

if (ENABLE_FEATURE_DEVICE_MODEL)
    add_executable(dev_model_sample dev_model/dev_model_sample.c)
    add_dependencies(dev_model_sample iot_sdk iot_platform)
endif ()

if (ENABLE_FEATURE_AUTH_MODE_DYNAMIC)
    add_executable(dynamic_auth_sample dynamic_auth/dynamic_auth_sample.c)
    add_dependencies(dynamic_auth_sample iot_sdk iot_platform)
endif ()

if (ENABLE_FEATURE_UPLOAD_FILE)
    add_executable(download_file_sample upload_file/download_file_sample.c)
    add_dependencies(download_file_sample iot_sdk iot_platform)
	
    add_executable(upload_file_sample upload_file/upload_file_sample.c)
    add_dependencies(upload_file_sample iot_sdk iot_platform)	
endif ()