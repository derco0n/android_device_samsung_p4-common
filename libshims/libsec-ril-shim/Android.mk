LOCAL_PATH := $(call my-dir)

#
# Shim for libsec-ril-apalone.so
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    libp4shim.cpp

LOCAL_SHARED_LIBRARIES := libbinder libcutils libstdc++

LOCAL_MODULE := libp4shim
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

include $(BUILD_SHARED_LIBRARY)
