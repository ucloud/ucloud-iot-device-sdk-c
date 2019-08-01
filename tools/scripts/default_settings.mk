PLATFORM_CC            ?= gcc
PLATFORM_AR            ?= ar
PLATFORM_OS            ?= linux

FEATURE_MQTT_COMM_ENABLED           ?= y
FEATURE_DEVICE_SHADOW_ENABLED       ?= n
FEATURE_OTA_ENABLED                 ?= n
FEATURE_DEVICE_MODEL_ENABLED        ?= y
FEATURE_AUTH_MODE_DYNAMIC           ?= n
FEATURE_SUPPORT_TLS					?= n

FEATURE_SDK_TESTS_ENABLED           ?= n    #是否开启单元测试编译	仅支持linux
FEATURE_MQTT_RMDUP_MSG_ENABLED      ?= n    #是否开启MQTT消息去重能力

ENABLE_MAKEFILE_DEBUG               ?= n