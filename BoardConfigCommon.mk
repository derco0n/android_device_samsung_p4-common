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
#USE_CAMERA_STUB := false
COMMON_GLOBAL_CFLAGS += -DNEEDS_VECTORIMPL_SYMBOLS #flash compatibility
PRODUCT_PREBUILT_WEBVIEWCHROMIUM := yes

DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

TARGET_NO_BOOTLOADER := true

# Architecture
TARGET_CPU_VARIANT := tegra2
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a
TARGET_ARCH_VARIANT_CPU := cortex-a9
TARGET_ARCH_VARIANT_FPU := vfpv3-d16
TARGET_CPU_SMP := true
ARCH_ARM_HAVE_TLS_REGISTER := true
ARCH_ARM_USE_NON_NEON_MEMCPY := true

#Board info
BOARD_KERNEL_BASE := 0x10000000
BOARD_KERNEL_CMDLINE := 
BOARD_PAGE_SIZE := 2048
TARGET_NO_RADIOIMAGE := true
TARGET_BOARD_PLATFORM := tegra
TARGET_BOOTLOADER_BOARD_NAME := p3

#Graphics
BOARD_EGL_NEEDS_LEGACY_FB := true
#Screenshot, freeze and hwc fix from omni
BOARD_EGL_SKIP_FIRST_DEQUEUE := true
BOARD_USE_MHEAP_SCREENSHOT := true
BOARD_NEEDS_OLD_HWC_API := true
##################
MAX_EGL_CACHE_KEY_SIZE := 4096
MAX_EGL_CACHE_SIZE := 2146304
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
BOARD_EGL_CFG := device/samsung/p4-common/egl.cfg
USE_OPENGL_RENDERER := true
BOARD_EGL_WORKAROUND_BUG_10194508 := true
#camera also needed for HW decoding

#TF101 stuff
BOARD_USES_PROPRIETARY_LIBCAMERA := true
BOARD_SECOND_CAMERA_DEVICE := true
BOARD_CAMERA_HAVE_ISO := true
USE_CAMERA_STUB := true
ICS_CAMERA_BLOB := true
BOARD_VENDOR_USE_NV_CAMERA := true
BOARD_USES_PROPRIETARY_OMX := TF101

#Sound
COMMON_GLOBAL_CFLAGS += -DICS_AUDIO_BLOB
HAVE_PRE_KITKAT_AUDIO_BLOB := true
BOARD_HAVE_PRE_KITKAT_AUDIO_POLICY_BLOB := true
USE_LEGACY_AUDIO_POLICY := 1
AUDIO_LEGACY_HACK := true
TARGET_ENABLE_OFFLOAD_ENHANCEMENTS := true

#CWM Recovery
TARGET_RECOVERY_PIXEL_FORMAT := "RGBX_8888"
BOARD_CUSTOM_RECOVERY_KEYMAPPING := ../../device/samsung/p4-common/recovery/recovery_ui.c
BOARD_HAS_NO_SELECT_BUTTON := true
BOARD_HAS_LARGE_FILESYSTEM := true

# Indicate that the board has an Internal SD Card
BOARD_HAS_SDCARD_INTERNAL := true

# device-specific extensions to the updater binary
TARGET_RELEASETOOLS_EXTENSIONS := device/samsung/p4-common
TARGET_USERIMAGES_USE_EXT4 := true
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

# Use nicer font rendering
#BOARD_USE_SKIA_LCDTEXT := true

# Charging Mode (LPM)
BOARD_CHARGING_MODE_BOOTING_LPM := "/sys/class/power_supply/battery/charging_mode_booting"

# Custom graphics for recovery
BOARD_CUSTOM_GRAPHICS := ../../../device/samsung/p4-common/recovery/graphics.c

# Preload bootanimation in to memory
TARGET_BOOTANIMATION_PRELOAD := true
TARGET_BOOTANIMATION_TEXTURE_CACHE := true
TARGET_BOOTANIMATION_USE_RGB565 := true
BOARD_NEEDS_LOWFPS_BOOTANI := true

# Suppress EMMC WIPE
BOARD_SUPPRESS_EMMC_WIPE := true
SUPPRESS_EMMC_WIPE := true
SUPPRESS_SECURE_ERASE := true
WIPE_IS_SUPPORTED := false

#skip dumb selinux metadata stuff
SKIP_SET_METADATA := true

# Lollipop
# https://gerrit.omnirom.org/#/c/10652/4/
TARGET_32_BIT_SURFACEFLINGER := true
MALLOC_IMPL := dlmalloc
USE_LEGACY_AUDIO_POLICY := 1
BOARD_NEEDS_SEC_RIL_WORKAROUND := true

#TWRP Flags
#we don't have cpu temperature only battery temperature :-(
TARGET_RECOVERY_FSTAB := device/samsung/p4-common/rootdir/fstab.p3
HAVE_SELINUX := true
DEVICE_RESOLUTION := 1280x800
BOARD_HAS_NO_REAL_SDCARD := true
RECOVERY_SDCARD_ON_DATA := true
TW_NO_REBOOT_BOOTLOADER := true
TW_ALWAYS_RMRF := true
TW_FLASH_FROM_STORAGE := true
TW_HAS_DOWNLOAD_MODE := true
TW_INTERNAL_STORAGE_PATH := "/data/media"
TW_INTERNAL_STORAGE_MOUNT_POINT := "data"
TW_NO_EXFAT_FUSE := true
TW_NO_EXFAT := true
TW_BRIGHTNESS_PATH := "/sys/devices/platform/cmc623_pwm_bl/backlight/pwm-backlight/brightness"
TW_MAX_BRIGHTNESS := 255
#TW_USE_TOOLBOX := true #causes weirdness. do not use. kill supersu instead
TW_NO_USB_STORAGE := true
# TW_EXCLUDE_SUPERSU := true
#in the ongoing battle to reduce recovery size no true type font
# TW_DISABLE_TTF := true

# TARGET_USERIMAGES_USE_F2FS := false
# TW_EXCLUDE_ENCRYPTED_BACKUPS := true
# TW_INCLUDE_CRYPTO := false

# TW_INCLUDE_CRYPTO := true
TW_INCLUDE_L_CRYPTO := true
TW_CRYPTO_FS_TYPE := "ext4"
TW_CRYPTO_REAL_BLKDEV := "/dev/block/mmcblk0p8"
TW_CRYPTO_MNT_POINT := "/data"
TW_CRYPTO_FS_OPTIONS := "noatime,nosuid,nodev,journal_async_commit,errors=panic      wait,check,encryptable=/efs/userdata_footer"
TW_CRYPTO_FS_FLAGS := "0x00000406"
TW_CRYPTO_KEY_LOC := "/efs/userdata_footer"
# TWRP_EVENT_LOGGING := true
# TW_EXCLUDE_MTP := true

# Build omnirom twrp android-5.0 on android-4.0 sources
PLATFORM_VERSION := 4.4.4

BOARD_RECOVERYIMAGE_PARTITION_SIZE := 5583457484
TW_EXTRA_RECOVERY_PARTITION := true
