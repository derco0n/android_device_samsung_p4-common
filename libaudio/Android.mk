LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := audio.primary.tegra
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := audio_hw.c
LOCAL_C_INCLUDES += \
	external/tinyalsa/include \
	$(call include-path-for, audio-utils) \
	$(call include-path-for, audio-route)
LOCAL_SHARED_LIBRARIES := liblog libcutils libtinyalsa libaudioutils libaudioroute
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_CFLAGS += -g

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -g
LOCAL_SRC_FILES := AudioPolicyManager.cpp
LOCAL_SHARED_LIBRARIES := libcutils libutils
LOCAL_STATIC_LIBRARIES := libmedia_helper
LOCAL_WHOLE_STATIC_LIBRARIES := libaudiopolicy_legacy
LOCAL_MODULE := audio_policy.tegra
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
