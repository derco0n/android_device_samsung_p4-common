ifeq ($(BOARD_OPENSOURCE_AUDIOHAL),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := audio.primary.tegra
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := audio_hw.c
LOCAL_C_INCLUDES += \
	external/tinyalsa/include \
	$(call include-path-for, audio-utils) \
	$(call include-path-for, audio-route) \
	$(call include-path-for, audio-effects)
LOCAL_SHARED_LIBRARIES := liblog libcutils libtinyalsa libaudioutils libaudioroute
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Werror -Wall
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_LDFLAGS += -ldl

LOCAL_CLANG := true

include $(BUILD_SHARED_LIBRARY)

endif
