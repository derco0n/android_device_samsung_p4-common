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

# This variable is set first, so it can be overridden
# by BoardConfigVendor.mk
BOARD_USES_GENERIC_AUDIO := false
USE_CAMERA_STUB := false

DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

# USE_CLANG_PLATFORM_BUILD := false
WITH_DEXPREOPT := false
# TARGET_NO_RECOVERY := true

# Architecture
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a
TARGET_CPU_VARIANT := generic
# TARGET_CPU_VARIANT := cortex-a9
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_NO_BOOTLOADER := true

BOARD_KERNEL_BASE := 0x10000000
BOARD_KERNEL_CMDLINE := 
BOARD_PAGE_SIZE := 2048
# BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE  := ext4

TARGET_NO_RADIOIMAGE := true
TARGET_BOARD_PLATFORM := tegra
TARGET_TEGRA_VERSION := t20
TARGET_BOOTLOADER_BOARD_NAME := p3

# Bionic
LINKER_NON_PIE_EXECUTABLES_HEADER_DIR := device/samsung/p4-common/include

MALLOC_IMPL := dlmalloc
MALLOC_SVELTE := true

TARGET_NEEDS_GCC_LIBC := true

# TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK := true
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
# TARGET_DISABLE_TRIPLE_BUFFERING := true
BOARD_EGL_CFG := device/samsung/p4-common/egl.cfg

BOARD_EGL_SKIP_FIRST_DEQUEUE := true
BOARD_USE_MHEAP_SCREENSHOT := true
# TARGET_FORCE_SCREENSHOT_CPU_PATH := true
BOARD_HAVE_SAMSUNG_T20_HWCOMPOSER := true
BOARD_TEGRA2_HWC_SET_RT_IOPRIO := false
TARGET_TEGRA2_HWC_1_1 := true
# TARGET_USES_HWC2 := true
TARGET_32_BIT_SURFACEFLINGER := true

TARGET_ICS_SENSOR_BLOB := true
TARGET_HAS_LEGACY_CAMERA_HAL1 := true
TARGET_NEEDS_NONPIE_CAMERASERVER := true

# Charger
BOARD_CHARGER_ENABLE_SUSPEND := true
BOARD_CHARGER_DISABLE_INIT_BLANK := true

USE_OPENGL_RENDERER := true

### Audio ###
BOARD_HAVE_PRE_KITKAT_AUDIO_BLOB := true
BOARD_HAVE_PRE_KITKAT_AUDIO_POLICY_BLOB := true
USE_LEGACY_AUDIO_POLICY := 0
## Audio Wrapper
AUDIO_WRAPPER_CONFIG := $(ANDROID_BUILD_TOP)/device/samsung/p4-common/audio/config.mk
## Open source audio
BOARD_OPENSOURCE_AUDIOHAL := true

TARGET_RECOVERY_PIXEL_FORMAT := "RGBX_8888"
#TARGET_RECOVERY_UI_LIB := librecovery_ui_ventana

# custom recovery ui
BOARD_CUSTOM_RECOVERY_KEYMAPPING := ../../device/samsung/p4-common/recovery/recovery_ui.c
BOARD_HAS_NO_SELECT_BUTTON := true
BOARD_HAS_LARGE_FILESYSTEM := true

# override recovery init.rc
TARGET_RECOVERY_INITRC := device/samsung/p4-common/recovery/init.rc

RECOVERY_FSTAB_VERSION := 2
TARGET_RECOVERY_FSTAB := device/samsung/p4-common/rootdir/etc/fstab.p3

# device-specific extensions to the updater binary
#TARGET_RECOVERY_UPDATER_LIBS += librecovery_updater_ventana
TARGET_RELEASETOOLS_EXTENSIONS := device/samsung/p4-common
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true
BOARD_FLASH_BLOCK_SIZE := 4096

# Wifi related defines
BOARD_WLAN_DEVICE                   := bcmdhd
WPA_SUPPLICANT_VERSION              := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER         := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB    := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER                := NL80211
BOARD_HOSTAPD_PRIVATE_LIB           := lib_driver_cmd_bcmdhd
BOARD_HAVE_SAMSUNG_WIFI             := true
BOARD_NO_WIFI_HAL                   := true

WIFI_DRIVER_MODULE_PATH     := "/system/lib/modules/dhd.ko"
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/dhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_AP      := "/system/etc/wifi/bcmdhd_apsta.bin"
WIFI_DRIVER_FW_PATH_STA     := "/system/etc/wifi/bcmdhd_sta.bin"
WIFI_DRIVER_FW_PATH_P2P     := "/system/etc/wifi/bcmdhd_p2p.bin"
WIFI_DRIVER_MODULE_NAME     := "dhd"
WIFI_DRIVER_MODULE_ARG      := "firmware_path=/system/etc/wifi/bcmdhd_sta.bin nvram_path=/system/etc/wifi/nvram_net.txt"
WIFI_DRIVER_MODULE_AP_ARG   := "firmware_path=/system/etc/wifi/bcmdhd_apsta.bin nvram_path=/system/etc/wifi/nvram_net.txt"

BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true
# Default value, if not overridden else where.
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/samsung/p4-common/bluetooth

# Custom graphics for recovery
BOARD_CUSTOM_GRAPHICS := ../../../device/samsung/p4-common/recovery/graphics.c

# Preload bootanimation in to memory
TARGET_BOOTANIMATION_PRELOAD := true
TARGET_BOOTANIMATION_TEXTURE_CACHE := true
TARGET_BOOTANIMATION_USE_RGB565 := true
#BOARD_NEEDS_LOWFPS_BOOTANI := true

# Suppress EMMC WIPE
BOARD_SUPPRESS_EMMC_WIPE := true

#TWRP Flags
DEVICE_RESOLUTION := 800x1280
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

# Seccomp filters
BOARD_SECCOMP_POLICY += device/samsung/p4-common/seccomp

BOARD_SEPOLICY_DIRS += \
	device/samsung/p4-common/sepolicy \

# MultiROM config. MultiROM also uses parts of TWRP config
MR_INPUT_TYPE := type_p75xx
MR_INIT_DEVICES := device/samsung/p4-common/mr_init_devices.c
MR_DPI := hdpi
MR_DPI_MUL := 0.5
MR_DPI_FONT := 160
MR_FSTAB := device/samsung/p4-common/twrp.fstab
MR_KEXEC_MEM_MIN := 0x08000000
MR_INFOS := device/samsung/p4-common/mrom_infos
TARGET_RECOVERY_IS_MULTIROM := true

DEVICE_MANIFEST_FILE := device/samsung/p4-common/manifest.xml
