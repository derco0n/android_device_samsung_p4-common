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

DEVICE_PACKAGE_OVERLAYS := $(LOCAL_PATH)/overlay

PRODUCT_AAPT_CONFIG := xlarge mdpi hdpi
PRODUCT_AAPT_PREF_CONFIG := mdpi

# Target arch
TARGET_ARCH := arm

TARGET_SCREEN_WIDTH := 1280
TARGET_SCREEN_HEIGHT := 800

PRODUCT_COPY_FILES := \
    $(LOCAL_PATH)/rootdir/init.p3.rc:root/init.p3.rc \
    $(LOCAL_PATH)/rootdir/fstab.p3:root/fstab.p3 \
    $(LOCAL_PATH)/rootdir/ueventd.p3.rc:root/ueventd.p3.rc \
    $(LOCAL_PATH)/rootdir/lpm.rc:root/lpm.rc \
	$(LOCAL_PATH)/rootdir/twrp.fstab:recovery/root/etc/twrp.fstab \
    $(LOCAL_PATH)/rootdir/init.p3.usb.rc:root/init.p3.usb.rc

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/media_profiles.xml:system/etc/media_profiles.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:system/etc/media_codecs_google_telephony.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml \
    $(LOCAL_PATH)/media_codecs.xml:system/etc/media_codecs.xml \
    $(LOCAL_PATH)/keylayout/sec_jack.kl:system/usr/keylayout/sec_jack.kl \
    $(LOCAL_PATH)/keylayout/sec_key.kl:system/usr/keylayout/sec_key.kl \
    $(LOCAL_PATH)/keylayout/sec_power_key.kl:system/usr/keylayout/sec_power_key.kl \
    $(LOCAL_PATH)/keylayout/sec_keyboard.kl:system/usr/keylayout/sec_keyboard.kl \
    $(LOCAL_PATH)/keylayout/Vendor_04e8_Product_7021.kl:system/usr/keylayout/Vendor_04e8_Product_7021.kl \
    $(LOCAL_PATH)/sec_touchscreen.idc:system/usr/idc/sec_touchscreen.idc

PRODUCT_COPY_FILES += \
     $(LOCAL_PATH)/gps.conf:system/etc/gps.conf

PRODUCT_COPY_FILES += \
     $(LOCAL_PATH)/97random:system/etc/init.d/97random \
     $(LOCAL_PATH)/98dcache:system/etc/init.d/98dcache \
     $(LOCAL_PATH)/99su:system/etc/init.d/99su

# LPM (from TW-UX 3.2)
PRODUCT_COPY_FILES += \
     $(LOCAL_PATH)/lpm/lib/libQmageDecoder.so:system/lib/libQmageDecoder.so \
     $(LOCAL_PATH)/lpm/bin/lpmkey:system/bin/lpmkey \
     $(LOCAL_PATH)/lpm/bin/playlpm:system/bin/playlpm \
     $(LOCAL_PATH)/lpm/media/battery_charging_0.qmg:system/media/battery_charging_0.qmg \
     $(LOCAL_PATH)/lpm/media/battery_charging_20.qmg:system/media/battery_charging_20.qmg \
     $(LOCAL_PATH)/lpm/media/battery_charging_40.qmg:system/media/battery_charging_40.qmg \
     $(LOCAL_PATH)/lpm/media/battery_charging_60.qmg:system/media/battery_charging_60.qmg \
     $(LOCAL_PATH)/lpm/media/battery_charging_80.qmg:system/media/battery_charging_80.qmg \
     $(LOCAL_PATH)/lpm/media/battery_charging_100.qmg:system/media/battery_charging_100.qmg \
     $(LOCAL_PATH)/lpm/media/battery_error.qmg:system/media/battery_error.qmg \
     $(LOCAL_PATH)/lpm/media/chargingwarning.qmg:system/media/chargingwarning.qmg \
     $(LOCAL_PATH)/lpm/media/Disconnected.qmg:system/media/Disconnected.qmg

PRODUCT_PROPERTY_OVERRIDES := \
    wifi.interface=wlan0 \
    wifi.supplicant_scan_interval=15 \
    media.stagefright.cache-params=6144/-1/30 \
    ro.sf.lcd_density=160 \
    dalvik.vm.dexopt-data-only=1 \
    debug.hwui.render_dirty_regions=false \
	ro.zygote.disable_gl_preload=true \
	ro.bq.gpu_to_cpu_unsupported=1

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp

#why the FUUUUUUUUUUUCK is wpa_supplicant no longer default
#what a major ass pain
#it takes up 1mb. what kind of storage doesn't have 1 fucking megabyte to spare???!!!!!
PRODUCT_PACKAGES := \
	libwpa_client \
	hostapd \
	wpa_supplicant \
	su

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.root_access=1

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
    $(LOCAL_PATH)/wifi/bcmdhd_mfg.bin:system/etc/wifi/bcmdhd_mfg.bin \
    $(LOCAL_PATH)/wifi/bcmdhd_apsta.bin:system/etc/wifi/bcmdhd_apsta.bin \
    $(LOCAL_PATH)/wifi/bcmdhd_p2p.bin:system/etc/wifi/bcmdhd_p2p.bin \
    $(LOCAL_PATH)/wifi/bcmdhd_sta.bin:system/etc/wifi/bcmdhd_sta.bin

#Prebuilt chromium
# COPY IN SYMBOLIC LINK FOR CHROMIUM APP
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/chromium/webview/webview.apk:system/app/webview/webview.apk \
	$(LOCAL_PATH)/chromium/libwebviewchromium.so:/system/lib/libwebviewchromium.so \
	$(LOCAL_PATH)/chromium/libwebviewchromium_loader.so:/system/lib/libwebviewchromium_loader.so \
	$(LOCAL_PATH)/chromium/libwebviewchromium_plat_support.so:/system/lib/libwebviewchromium_plat_support.so

PRODUCT_PACKAGES += \
        libinvensense_mpl

# Omni packages
PRODUCT_PACKAGES += \
        OmniTorch \
        OmniSwitch
# Audio
PRODUCT_PACKAGES += \
        audio.a2dp.default \
		audio.usb.default \
        libaudioutils \
        libtinyalsa

#disable setup wizard. causes issues.
PRODUCT_PROPERTY_OVERRIDES += \
		ro.setupwizard.mode=DISABLED

PRODUCT_COPY_FILES += \
        $(LOCAL_PATH)/audio/asound.conf:system/etc/asound.conf \
        $(LOCAL_PATH)/audio/audio_policy.conf:system/etc/audio_policy.conf \
        $(LOCAL_PATH)/audio/nvaudio_conf.xml:system/etc/nvaudio_conf.xml
        
# These are the hardware-specific features
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.location.xml:system/etc/permissions/android.hardware.location.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/camera/nvcamera.conf:system/etc/nvcamera.conf \
    $(LOCAL_PATH)/bluetooth/bt_vendor.conf:system/etc/bluetooth/bt_vendor.conf

PRODUCT_CHARACTERISTICS := tablet,nosdcard

PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_PACKAGES += \
    librs_jni \
    com.android.future.usb.accessory \
    power.p3 \
    libnetcmdiface

# Filesystem management tools
PRODUCT_PACKAGES += \
	make_ext4fs \
	setup_fs

DEVICE_PACKAGE_OVERLAYS := \
    $(LOCAL_PATH)/overlay

# for bugmailer
# removed, not present in cm10.2

$(call inherit-product, frameworks/native/build/tablet-dalvik-heap.mk)
$(call inherit-product, vendor/omni/config/common_tablet.mk)

