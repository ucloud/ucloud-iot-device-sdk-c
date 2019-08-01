CXX 				:= g++
LDFLAGS             := $(FINAL_DIR)/lib/libiot_sdk.a
LDFLAGS             += $(FINAL_DIR)/lib/libiot_platform.a
ifneq (,$(filter -DSUPPORT_TLS,$(CFLAGS)))
LDFLAGS             += $(FINAL_DIR)/lib/libmbedtls.a $(FINAL_DIR)/lib/libmbedx509.a $(FINAL_DIR)/lib/libmbedcrypto.a
endif

UNIT_SRC_DIR = ${TESTS_DIR}/unit_test/src
UNIT_SRC_FILES =

ifneq (,$(filter -DMQTT_COMM_ENABLED,$(CFLAGS)))

UNIT_SRC_FILES += $(UNIT_SRC_DIR)/unit_mqtt_test.cpp

ifneq (,$(filter -DAUTH_MODE_DYNAMIC,$(CFLAGS)))
UNIT_SRC_FILES += $(UNIT_SRC_DIR)/unit_dynamic_auth_test.cpp
endif

ifneq (,$(filter -DOTA_ENABLED,$(CFLAGS)))
UNIT_SRC_FILES += $(UNIT_SRC_DIR)/unit_http_test.cpp
endif

ifneq (,$(filter -DDEVICE_SHADOW_ENABLED,$(CFLAGS)))
UNIT_SRC_FILES += $(UNIT_SRC_DIR)/unit_shadow_test.cpp
endif

endif

HELPER_C_FILES = $(wildcard $(UNIT_SRC_DIR)/*.c)
TLS_C_FILES = $(wildcard ${TESTS_DIR}/unit_test/tls_mock/*.c)

unit_objects = $(patsubst %.cpp,%, $(UNIT_SRC_FILES))

ifneq (,$(filter -DSDK_TESTS_ENABLED,$(CFLAGS)))
run_unit_test: gtest ${unit_objects}

gtest:
	$(TOP_Q) \
	make -s -C $(TEST_LIB_DIR)

${unit_objects}:%:%.cpp
	$(call Brief_Log,"LD")
	$(TOP_Q) \
	$(CXX) $(CFLAGS) -I$(TEST_LIB_DIR)/include -I$(TESTS_DIR)/unit_test/include \
	-I$(TESTS_DIR)/unit_test/tls_mock -pthread \
	$^ $(HELPER_C_FILES) $(TLS_C_FILES) $(LDFLAGS) ${TEST_LIB_DIR}/libgtest.a \
	-o $@

	$(TOP_Q) \
	mv $@ $(FINAL_DIR)/unittest

endif
