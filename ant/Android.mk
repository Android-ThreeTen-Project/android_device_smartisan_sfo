# Build the existing ANT service with SFO's Android 16 receiver compatibility.
LOCAL_PATH := $(call my-dir)

sfo_ant_source := external/ant-wireless/ant_service
sfo_ant_relative := ../../../../$(sfo_ant_source)
sfo_ant_java := $(call intermediates-dir-for,APPS,AntHalService_sfo,,COMMON)/src/com/dsi/ant/server/AntService.java

$(sfo_ant_java): $(sfo_ant_source)/src/com/dsi/ant/server/AntService.java $(LOCAL_PATH)/fix-receivers.py
	@mkdir -p $(dir $@)
	$(hide) python3 device/smartisan/sfo/ant/fix-receivers.py $< $@

include $(CLEAR_VARS)
LOCAL_PACKAGE_NAME := AntHalService_sfo
LOCAL_OVERRIDES_PACKAGES := AntHalService
LOCAL_SRC_FILES := \
    $(filter-out $(sfo_ant_relative)/src/com/dsi/ant/server/AntService.java,$(call all-java-files-under,$(sfo_ant_relative)/src)) \
    $(sfo_ant_relative)/src/com/dsi/ant/server/IAntHal.aidl \
    $(sfo_ant_relative)/src/com/dsi/ant/server/IAntHalCallback.aidl
LOCAL_GENERATED_SOURCES := $(sfo_ant_java)
LOCAL_AIDL_INCLUDES := $(sfo_ant_source)/src
LOCAL_FULL_MANIFEST_FILE := $(sfo_ant_source)/AndroidManifest.xml
LOCAL_RESOURCE_DIR := $(sfo_ant_source)/res
LOCAL_PROGUARD_FLAG_FILES := $(sfo_ant_relative)/proguard.flags
LOCAL_REQUIRED_MODULES := libantradio
LOCAL_CERTIFICATE := platform
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := system_current
LOCAL_SYSTEM_EXT_MODULE := true
include $(BUILD_PACKAGE)
