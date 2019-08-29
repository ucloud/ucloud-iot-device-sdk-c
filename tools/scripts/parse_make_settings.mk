include $(CURDIR)/tools/scripts/internal_make_funcs.mk

SETTING_VARS := \
    BUILD_TYPE \
    PLATFORM_CC \
    PLATFORM_AR \
    PLATFORM_OS \

SWITCH_VARS := \
    FEATURE_MQTT_COMM_ENABLED \
    FEATURE_OTA_ENABLED \
    FEATURE_DEVICE_SHADOW_ENABLED \
    FEATURE_SUPPORT_TLS \
    FEATURE_MQTT_RMDUP_MSG_ENABLED \
	FEATURE_AUTH_MODE_DYNAMIC \
	FEATURE_DEVICE_MODEL_ENABLED \
	FEATURE_FILE_UPLOAD_ENABLED	\

$(foreach v, \
    $(SETTING_VARS) $(SWITCH_VARS), \
    $(eval export $(v)=$($(v))) \
)

$(foreach v, \
    $(SWITCH_VARS), \
    $(if $(filter y,$($(v))), \
        $(eval CFLAGS += -D$(subst FEATURE_,,$(v)))) \
)

include $(CURDIR)/tools/scripts/settings.mk

CFLAGS  += -DFORCE_SSL_VERIFY -DENABLE_LOG_ERR -DENABLE_LOG_WARN -DENABLE_LOG_INFO
ifeq (debug,$(strip $(BUILD_TYPE)))
CFLAGS  += -DENABLE_LOG_DEBUG -DENABLE_IOT_TRACE
endif

ifneq (linux,$(strip $(PLATFORM_OS)))
ifeq (y,$(strip $(FEATURE_SDK_TESTS_ENABLED)))
$(error FEATURE_SDK_TESTS_ENABLED with gtest framework just supports to be enabled on PLATFORM_OS = linux!)
endif
else
ifeq (y,$(strip $(FEATURE_SDK_TESTS_ENABLED)))
CFLAGS += -DSDK_TESTS_ENABLED
endif
endif