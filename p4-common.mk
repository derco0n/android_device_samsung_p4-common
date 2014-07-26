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

#saves a world of butt hurt. the prebuilt is genuinely compiled from source but for some reason
#  the build script has an urge to rebuild this every single time even though 
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/chromium/libwebviewchromium.so:system/lib/libwebviewchromium.so \
	$(LOCAL_PATH)/chromium/libwebviewchromium.so:system/lib/libwebviewchromium_plat_support.so \
	$(LOCAL_PATH)/chromium/webviewchromium.jar:system/framework/webviewchromium.jar \
	$(LOCAL_PATH)/chromium/webview/paks/am.pak:system/framework/webview/paks/am.pak \
	$(LOCAL_PATH)/chromium/webview/paks/am.pak:system/framework/webview/paks/am.pak \
	$(LOCAL_PATH)/chromium/webview/paks/ar.pak:system/framework/webview/paks/ar.pak \
	$(LOCAL_PATH)/chromium/webview/paks/bg.pak:system/framework/webview/paks/bg.pak \
	$(LOCAL_PATH)/chromium/webview/paks/bn.pak:system/framework/webview/paks/bn.pak \
	$(LOCAL_PATH)/chromium/webview/paks/ca.pak:system/framework/webview/paks/ca.pak \
	$(LOCAL_PATH)/chromium/webview/paks/cs.pak:system/framework/webview/paks/cs.pak \
	$(LOCAL_PATH)/chromium/webview/paks/da.pak:system/framework/webview/paks/da.pak \
	$(LOCAL_PATH)/chromium/webview/paks/de.pak:system/framework/webview/paks/de.pak \
	$(LOCAL_PATH)/chromium/webview/paks/el.pak:system/framework/webview/paks/el.pak \
	$(LOCAL_PATH)/chromium/webview/paks/en-GB.pak:system/framework/webview/paks/en-GB.pak \
	$(LOCAL_PATH)/chromium/webview/paks/en-US.pak:system/framework/webview/paks/en-US.pak \
	$(LOCAL_PATH)/chromium/webview/paks/es-419.pak:system/framework/webview/paks/es-419.pak \
	$(LOCAL_PATH)/chromium/webview/paks/es.pak:system/framework/webview/paks/es.pak \
	$(LOCAL_PATH)/chromium/webview/paks/et.pak:system/framework/webview/paks/et.pak \
	$(LOCAL_PATH)/chromium/webview/paks/fa.pak:system/framework/webview/paks/fa.pak \
	$(LOCAL_PATH)/chromium/webview/paks/fil.pak:system/framework/webview/paks/fil.pak \
	$(LOCAL_PATH)/chromium/webview/paks/fi.pak:system/framework/webview/paks/fi.pak \
	$(LOCAL_PATH)/chromium/webview/paks/fr.pak:system/framework/webview/paks/fr.pak \
	$(LOCAL_PATH)/chromium/webview/paks/gu.pak:system/framework/webview/paks/gu.pak \
	$(LOCAL_PATH)/chromium/webview/paks/he.pak:system/framework/webview/paks/he.pak \
	$(LOCAL_PATH)/chromium/webview/paks/hi.pak:system/framework/webview/paks/hi.pak \
	$(LOCAL_PATH)/chromium/webview/paks/hr.pak:system/framework/webview/paks/hr.pak \
	$(LOCAL_PATH)/chromium/webview/paks/hu.pak:system/framework/webview/paks/hu.pak \
	$(LOCAL_PATH)/chromium/webview/paks/id.pak:system/framework/webview/paks/id.pak \
	$(LOCAL_PATH)/chromium/webview/paks/it.pak:system/framework/webview/paks/it.pak \
	$(LOCAL_PATH)/chromium/webview/paks/ja.pak:system/framework/webview/paks/ja.pak \
	$(LOCAL_PATH)/chromium/webview/paks/kn.pak:system/framework/webview/paks/kn.pak \
	$(LOCAL_PATH)/chromium/webview/paks/ko.pak:system/framework/webview/paks/ko.pak \
	$(LOCAL_PATH)/chromium/webview/paks/lt.pak:system/framework/webview/paks/lt.pak \
	$(LOCAL_PATH)/chromium/webview/paks/lv.pak:system/framework/webview/paks/lv.pak \
	$(LOCAL_PATH)/chromium/webview/paks/ml.pak:system/framework/webview/paks/ml.pak \
	$(LOCAL_PATH)/chromium/webview/paks/mr.pak:system/framework/webview/paks/mr.pak \
	$(LOCAL_PATH)/chromium/webview/paks/ms.pak:system/framework/webview/paks/ms.pak \
	$(LOCAL_PATH)/chromium/webview/paks/nb.pak:system/framework/webview/paks/nb.pak \
	$(LOCAL_PATH)/chromium/webview/paks/nl.pak:system/framework/webview/paks/nl.pak \
	$(LOCAL_PATH)/chromium/webview/paks/pl.pak:system/framework/webview/paks/pl.pak \
	$(LOCAL_PATH)/chromium/webview/paks/pt-BR.pak:system/framework/webview/paks/pt-BR.pak \
	$(LOCAL_PATH)/chromium/webview/paks/pt-PT.pak:system/framework/webview/paks/pt-PT.pak \
	$(LOCAL_PATH)/chromium/webview/paks/ro.pak:system/framework/webview/paks/ro.pak \
	$(LOCAL_PATH)/chromium/webview/paks/ru.pak:system/framework/webview/paks/ru.pak \
	$(LOCAL_PATH)/chromium/webview/paks/sk.pak:system/framework/webview/paks/sk.pak \
	$(LOCAL_PATH)/chromium/webview/paks/sl.pak:system/framework/webview/paks/sl.pak \
	$(LOCAL_PATH)/chromium/webview/paks/sr.pak:system/framework/webview/paks/sr.pak \
	$(LOCAL_PATH)/chromium/webview/paks/sv.pak:system/framework/webview/paks/sv.pak \
	$(LOCAL_PATH)/chromium/webview/paks/sw.pak:system/framework/webview/paks/sw.pak \
	$(LOCAL_PATH)/chromium/webview/paks/ta.pak:system/framework/webview/paks/ta.pak \
	$(LOCAL_PATH)/chromium/webview/paks/te.pak:system/framework/webview/paks/te.pak \
	$(LOCAL_PATH)/chromium/webview/paks/th.pak:system/framework/webview/paks/th.pak \
	$(LOCAL_PATH)/chromium/webview/paks/tr.pak:system/framework/webview/paks/tr.pak \
	$(LOCAL_PATH)/chromium/webview/paks/uk.pak:system/framework/webview/paks/uk.pak \
	$(LOCAL_PATH)/chromium/webview/paks/vi.pak:system/framework/webview/paks/vi.pak \
	$(LOCAL_PATH)/chromium/webview/paks/webviewchromium.pak:system/framework/webview/paks/webviewchromium.pak \
	$(LOCAL_PATH)/chromium/webview/paks/zh-CN.pak:system/framework/webview/paks/zn-CH.pak \
	$(LOCAL_PATH)/chromium/webview/paks/zh-TW.pak:system/framework/webview/paks/zn-TW.pak

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

# Disable SELinux
PRODUCT_PROPERTY_OVERRIDES += \
    ro.boot.selinux=disabled \
	ro.build.selinux=0

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.root_access=1

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
    $(LOCAL_PATH)/wifi/bcmdhd_apsta.bin:system/etc/wifi/bcmdhd_apsta.bin \
    $(LOCAL_PATH)/wifi/bcmdhd_p2p.bin:system/etc/wifi/bcmdhd_p2p.bin \
    $(LOCAL_PATH)/wifi/bcmdhd_sta.bin:system/etc/wifi/bcmdhd_sta.bin

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

#make use of that wasted cache parition
PRODUCT_PROPERTY_OVERRIDES += \
		dalvik.vm.dexopt-data-only=0

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

