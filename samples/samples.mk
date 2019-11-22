DEPENDS             := platform
LDFLAGS             := $(FINAL_DIR)/lib/libiot_sdk.a
LDFLAGS             += $(FINAL_DIR)/lib/libiot_platform.a
ifneq (,$(filter -DSUPPORT_TLS,$(CFLAGS)))
LDFLAGS             += $(FINAL_DIR)/lib/libmbedtls.a $(FINAL_DIR)/lib/libmbedx509.a $(FINAL_DIR)/lib/libmbedcrypto.a
endif
CFLAGS              := $(filter-out -ansi,$(CFLAGS))

ifneq (,$(filter -DMQTT_COMM_ENABLED,$(CFLAGS)))
mqtt_sample:
	$(TOP_Q) \
	$(PLATFORM_CC) $(CFLAGS) $(SAMPLE_DIR)/mqtt/$@.c $(LDFLAGS) -o $@

	mv $@ $(FINAL_DIR)/bin
endif

ifneq (,$(filter -DAUTH_MODE_DYNAMIC,$(CFLAGS)))
dynamic_auth_sample:
	$(TOP_Q) \
	$(PLATFORM_CC) $(CFLAGS) $(SAMPLE_DIR)/dynamic_auth/$@.c $(LDFLAGS) -o $@

	$(TOP_Q) \
	mv $@ $(FINAL_DIR)/bin
endif

ifneq (,$(filter -DOTA_ENABLED,$(CFLAGS)))
ota_sample:
	$(TOP_Q) \
	$(PLATFORM_CC) $(CFLAGS) $(SAMPLE_DIR)/ota/$@.c $(LDFLAGS) -o $@

	$(TOP_Q) \
	mv $@ $(FINAL_DIR)/bin
endif

ifneq (,$(filter -DDEVICE_SHADOW_ENABLED,$(CFLAGS)))
shadow_sample:
	$(TOP_Q) \
	$(PLATFORM_CC) $(CFLAGS) $(SAMPLE_DIR)/shadow/$@.c $(LDFLAGS) -o $@

	$(TOP_Q) \
	mv $@ $(FINAL_DIR)/bin

smart_bracelet_heart_rate_shadow_sample:
	$(TOP_Q) \
	$(PLATFORM_CC) $(CFLAGS) $(SAMPLE_DIR)/shadow/$@.c $(LDFLAGS) -o $@

	$(TOP_Q) \
	mv $@ $(FINAL_DIR)/bin

smart_bracelet_walk_step_shadow_sample:
	$(TOP_Q) \
	$(PLATFORM_CC) $(CFLAGS) $(SAMPLE_DIR)/shadow/$@.c $(LDFLAGS) -o $@

	$(TOP_Q) \
	mv $@ $(FINAL_DIR)/bin
endif

ifneq (,$(filter -DDEVICE_MODEL_ENABLED,$(CFLAGS)))
dev_model_sample:
	$(TOP_Q) \
	$(PLATFORM_CC) $(CFLAGS) $(SAMPLE_DIR)/dev_model/$@.c $(LDFLAGS) -o $@

	$(TOP_Q) \
	mv $@ $(FINAL_DIR)/bin
endif

ifneq (,$(filter -DFILE_UPLOAD_ENABLED,$(CFLAGS)))
download_file_sample:
	$(TOP_Q) \
	$(PLATFORM_CC) $(CFLAGS) $(SAMPLE_DIR)/upload_file/$@.c $(LDFLAGS) -o $@

	$(TOP_Q) \
	mv $@ $(FINAL_DIR)/bin

upload_file_sample:
	$(TOP_Q) \
	$(PLATFORM_CC) $(CFLAGS) $(SAMPLE_DIR)/upload_file/$@.c $(LDFLAGS) -o $@

	$(TOP_Q) \
	mv $@ $(FINAL_DIR)/bin
endif


samples_final:
	$(TOP_Q) \
	cp -rf $(TOP_DIR)/src/sdk-impl/*port*.h $(FINAL_DIR)/include/

	$(TOP_Q) \
	cp -rf $(TOP_DIR)/platform/os/$(PLATFORM_OS)/*.h $(FINAL_DIR)/include/