LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    simple_sdk_billbaord.cpp \
    simple_sdk_common.cpp \
    simple_sdk.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libGLESv2

LOCAL_MODULE := libsimpleSDK
include $(BUILD_SHARED_LIBRARY)
