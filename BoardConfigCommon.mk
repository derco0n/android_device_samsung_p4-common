#
# Copyright (C) 2011 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Default value, if not overridden else where.
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR ?= device/samsung/p4-common/bluetooth

# This variable is set first, so it can be overridden
# by BoardConfigVendor.mk
BOARD_USES_GENERIC_AUDIO := false
USE_CAMERA_STUB := false

# KitKat flags
BOARD_USE_MHEAP_SCREENSHOT := true
# TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK := true
ICS_CAMERA_BLOB := true
BOARD_CAMERA_HAVE_ISO := true
BOARD_USES_PROPRIETARY_OMX := TF101
#BOARD_NEEDS_OLD_HWC_API := true
#BOARD_NEED_OMX_COMPAT := true
#BOARD_EGL_NEEDS_FNW := true
#NEED_WORKAROUND_CORTEX_A9_745320 := true
#BOARD_USES_PROPRIETARY_LIBCAMERA := true
#BOARD_SECOND_CAMERA_DEVICE := true
#USE_CAMERA_STUB := true
#BOARD_VENDOR_USE_NV_CAMERA := true

# Optimization hwui
HWUI_COMPILE_FOR_PERF := true

# Flash compatibility
COMMON_GLOBAL_CFLAGS += -DNEEDS_VECTORIMPL_SYMBOLS

DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

TARGET_NO_BOOTLOADER := true

# Architecture
# TARGET_CPU_VARIANT := generic
TARGET_CPU_VARIANT := cortex-a9
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a
TARGET_ARCH_VARIANT_CPU := cortex-a9
TARGET_ARCH_VARIANT_FPU := vfpv3-d16
TARGET_CPU_SMP := true
#TARGET_HAVE_TEGRA_ERRATA_657451 := true
ARCH_ARM_HAVE_TLS_REGISTER := true
ARCH_ARM_USE_NON_NEON_MEMCPY := true
ARCH_ARM_HIGH_OPTIMIZATION := true

BOARD_KERNEL_BASE := 0x10000000
BOARD_KERNEL_CMDLINE := 
BOARD_PAGE_SIZE := 2048

TARGET_NO_RADIOIMAGE := true
TARGET_BOARD_PLATFORM := tegra
TARGET_TEGRA_VERSION := t20
TARGET_BOOTLOADER_BOARD_NAME := p3
#TARGET_BOARD_INFO_FILE := device/samsung/p4-common/board-info.txt

BOARD_EGL_NEEDS_LEGACY_FB := true
# MAX_EGL_CACHE_KEY_SIZE := 4096
# MAX_EGL_CACHE_SIZE := 2146304
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
# TARGET_DISABLE_TRIPLE_BUFFERING := false
BOARD_EGL_CFG := device/samsung/p4-common/egl.cfg
BOARD_EGL_WORKAROUND_BUG_10194508 := true
BOARD_EGL_SKIP_FIRST_DEQUEUE := true
# https://android.googlesource.com/platform/frameworks/native/+/201f3b2da572eb27b9d4b3131e6d8c3c92a13de8
# BOARD_ALLOW_EGL_HIBERNATION := true

#BOARD_USES_HGL := true
BOARD_USES_OVERLAY := true
BOARD_USES_HWCOMPOSER := true
USE_OPENGL_RENDERER := true
# Enable WEBGL in WebKit
ENABLE_WEBGL := true

# BOARD_HAVE_PRE_KITKAT_AUDIO_BLOB := true
# BOARD_HAVE_PRE_KITKAT_AUDIO_POLICY_BLOB := true
# COMMON_GLOBAL_CFLAGS += -DICS_AUDIO_BLOB
# COMMON_GLOBAL_CFLAGS += -DAUDIO_LEGACY_HACK
# TARGET_ENABLE_OFFLOAD_ENHANCEMENTS := true

# skia SIGILL bootloop fix
COMMON_GLOBAL_CFLAGS += -DOLD_TEGRA

TARGET_RECOVERY_PIXEL_FORMAT := "RGBX_8888"
#TARGET_RECOVERY_UI_LIB := librecovery_ui_ventana

# custom recovery ui
BOARD_CUSTOM_RECOVERY_KEYMAPPING := ../../device/samsung/p4-common/recovery/recovery_ui.c
BOARD_HAS_NO_SELECT_BUTTON := true
BOARD_HAS_LARGE_FILESYSTEM := true

# override recovery init.rc
TARGET_RECOVERY_INITRC := device/samsung/p4-common/recovery/init.rc

RECOVERY_FSTAB_VERSION := 2
ifeq ($(F2FS_BUILD), true)
TARGET_RECOVERY_FSTAB := device/samsung/p4-common/fstab.p3-f2fs
else
TARGET_RECOVERY_FSTAB := device/samsung/p4-common/fstab.p3
endif

# Indicate that the board has an Internal SD Card
BOARD_HAS_SDCARD_INTERNAL := true

# device-specific extensions to the updater binary
#TARGET_RECOVERY_UPDATER_LIBS += librecovery_updater_ventana
TARGET_RELEASETOOLS_EXTENSIONS := device/samsung/p4-common
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true
BOARD_FLASH_BLOCK_SIZE := 4096

# Wifi related defines
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_WLAN_DEVICE := bcmdhd
BOARD_LEGACY_NL80211_STA_EVENTS := true

WIFI_DRIVER_MODULE_PATH     := "/system/lib/modules/dhd.ko"
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_AP      := "/system/etc/wifi/bcmdhd_apsta.bin"
WIFI_DRIVER_FW_PATH_STA     := "/system/etc/wifi/bcmdhd_sta.bin"
WIFI_DRIVER_FW_PATH_P2P     := "/system/etc/wifi/bcmdhd_p2p.bin"
WIFI_DRIVER_MODULE_NAME     := "dhd"
WIFI_DRIVER_MODULE_ARG      := "iface_name=wlan0 firmware_path=/system/etc/wifi/bcmdhd_sta.bin nvram_path=/system/etc/wifi/nvram_net.txt"

BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true

#BOARD_LIB_DUMPSTATE := libdumpstate.ventana

# Use nicer font rendering
# BOARD_USE_SKIA_LCDTEXT := true

# Charging Mode (LPM)
BOARD_CHARGING_MODE_BOOTING_LPM := "/sys/class/power_supply/battery/charging_mode_booting"

# Custom graphics for recovery
BOARD_CUSTOM_GRAPHICS := ../../../device/samsung/p4-common/recovery/graphics.c

# Preload bootanimation in to memory
TARGET_BOOTANIMATION_PRELOAD := true
TARGET_BOOTANIMATION_TEXTURE_CACHE := true
TARGET_BOOTANIMATION_USE_RGB565 := true
#BOARD_NEEDS_LOWFPS_BOOTANI := true

# Suppress EMMC WIPE
BOARD_SUPPRESS_EMMC_WIPE := true

# Skip SELinux metadata
SKIP_SET_METADATA := true

#TWRP Flags
DEVICE_RESOLUTION := 1280x800
RECOVERY_SDCARD_ON_DATA := true
BOARD_HAS_NO_REAL_SDCARD := true
TW_NO_REBOOT_BOOTLOADER := true
TW_NO_USB_STORAGE := true
TW_FLASH_FROM_STORAGE := true
TW_HAS_DOWNLOAD_MODE := true
SP1_NAME := "efs"
SP1_BACKUP_METHOD := files
SP1_MOUNTABLE := 1
TW_ALWAYS_RMRF := true
TW_NO_EXFAT_FUSE := true
TW_BRIGHTNESS_PATH := "/sys/devices/platform/cmc623_pwm_bl/backlight/pwm-backlight/brightness"
TW_MAX_BRIGHTNESS := 255
TW_NO_EXFAT := true
TW_EXCLUDE_SUPERSU := true
