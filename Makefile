include make.settings
include tools/scripts/default_settings.mk
include tools/scripts/parse_make_settings.mk

# IoT SDK sources files defination
COMP_LIB            := libiot_sdk.a
COMP_LIB_COMPONENTS := \
    src/utils \
    src/certs \
    src/sdk-impl \

$(call CompLib_Map, MQTT_COMM_ENABLED, src/mqtt/src)

$(call CompLib_Map, DEVICE_SHADOW_ENABLED, src/shadow/src)

$(call CompLib_Map, OTA_ENABLED, src/ota/src)

$(call CompLib_Map, DEVICE_MODEL_ENABLED, src/dev_model/src)

$(call CompLib_Map, HTTP_CLIENT_ENABLED, src/http)

$(call CompLib_Map, SUPPORT_AT_CMD, src/at/src src/at/class/$(PLATFORM_MODULE) platform/module)
IOTSDK_SRC_FILES := \

$(foreach v, \
    $(COMP_LIB_COMPONENTS), \
    $(eval \
    	export IOTSDK_SRC_FILES += \
    	$(wildcard $(TOP_DIR)/$(v)/*.c) \
    ) \
)

# IoT Platform sources files defination
PLATFORM_LIB		:= libiot_platform.a
PLATFORM_LIB_COMPONENTS := \
    platform/os/$(PLATFORM_OS) \
    
ifneq (,$(filter -DSUPPORT_TLS,$(CFLAGS)))
	PLATFORM_LIB_COMPONENTS += \
    platform/ssl/$(PLATFORM_SSL)
endif
	
IOTPLATFORM_SRC_FILES := \

$(foreach v, \
    $(PLATFORM_LIB_COMPONENTS), \
    $(eval \
    	export IOTPLATFORM_SRC_FILES += \
    	$(wildcard $(TOP_DIR)/$(v)/*.c) \
    ) \
)

# IoT Include files defination
COMP_LIB_COMPONENTS_INCLUDES := \
    src/utils \
    src/certs \
    src/sdk-impl \
    platform/os/$(PLATFORM_OS)

$(call CompInc_Map, MQTT_COMM_ENABLED, \
    src/mqtt/include \
)

$(call CompInc_Map, DEVICE_SHADOW_ENABLED, \
	src/shadow/include \
)

$(call CompInc_Map, OTA_ENABLED, \
	src/ota/include \
)

$(call CompInc_Map, DEVICE_MODEL_ENABLED, \
	src/dev_model/include \
)

$(call CompInc_Map, SUPPORT_TLS, \
	external_libs/mbedtls/include \
)
    
$(call CompInc_Map, SUPPORT_AT_CMD, \
	src/at/include \
)
	
IOTSDK_INCLUDE_FILES := \

$(foreach v, \
    $(COMP_LIB_COMPONENTS_INCLUDES), \
    $(eval \
    	export IOTSDK_INCLUDE_FILES += \
    	-I$(TOP_DIR)/$(v) \
    ) \
)

CFLAGS += -Werror -Wall -Wno-error=sign-compare -Wno-error=format -Os ${IOTSDK_INCLUDE_FILES} -pthread

include tools/scripts/rules.mk
include samples/samples.mk

ifneq (,$(filter -DSDK_TESTS_ENABLED, $(CFLAGS)))
include tests/unit_test/unit_test.mk
endif
