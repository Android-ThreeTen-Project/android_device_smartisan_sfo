# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.vibrator-service.sfo
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_INIT_RC := android.hardware.vibrator-service.sfo.rc
LOCAL_VINTF_FRAGMENTS := android.hardware.vibrator-service.sfo.xml
# Reuse the legacy backend; device-specific capability reporting is in service.cpp.
LOCAL_SRC_FILES := \
    service.cpp \
    ../../../../hardware/lineage/interfaces/vibrator/aidl-legacy/Vibrator.cpp
LOCAL_C_INCLUDES := hardware/lineage/interfaces/vibrator/aidl-legacy
LOCAL_SHARED_LIBRARIES := \
    libbase \
    libbinder_ndk \
    libhardware \
    liblog \
    android.hardware.vibrator-V2-ndk
include $(BUILD_EXECUTABLE)
