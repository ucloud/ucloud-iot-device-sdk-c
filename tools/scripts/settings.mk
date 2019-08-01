# Basic Settings
#
SHELL           := /bin/bash
TOP_DIR         ?= $(CURDIR)

# Settings of input directory
#
SCRIPT_DIR      := $(TOP_DIR)/tools/scripts

# Settings of output directory
#
TEMP_DIR   		:= $(TOP_DIR)/tmp
DIST_DIR        := $(TOP_DIR)/output
FINAL_DIR       := $(DIST_DIR)/release

# Thirdparty libs directory
THIRD_PARTY_PATH 	:= $(TOP_DIR)/external_libs
TEST_LIB_DIR        := $(THIRD_PARTY_PATH)/googletest

# Sample directory
SAMPLE_DIR		:= $(TOP_DIR)/samples

# Test directory
TESTS_DIR		:= $(TOP_DIR)/tests

# Settings of makefile echo
#
ifeq (n,$(strip $(ENABLE_MAKEFILE_DEBUG)))
    Q               := @
    TOP_Q           := @
else
    Q               :=
    TOP_Q           :=
endif