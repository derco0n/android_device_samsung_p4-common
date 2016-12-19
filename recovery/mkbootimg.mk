
EXTRA_FILES := \
	adbd \
	bu \
	busybox \
	e2fsck \
	fatlabel \
	fsck.f2fs \
	fsck.fat \
	linker \
	libbase.so \
	libblkid.so \
	libcrypto.so \
	libext2_e2p.so \
	libext2fs.so \
	libext4_utils.so \
	libf2fs.so \
	libft2.so \
	libm.so \
	libminuitwrp.so \
	libminzip.so \
	libmtdutils.so \
	libpng.so \
	libsparse.so \
	libtar.so \
	libtwrpmtp.so \
	make_ext4fs \
	mke2fs \
	mkfs.f2fs \
	mkfs.fat \
	pigz \
	recovery \
	resize2fs \
	simg2img \
	tune2fs \
	unpigz \

EXTRA_TMP := extra_tmp
# Needs to be actual working path on the device
EXTRA_SYMLINK_NAME := extra
EXTRA_OUT := $(EXTRA_SYMLINK_NAME)

define build-extra-recoveryramdisk
	@echo -e ${PRT_IMG}"----- Clean extra recovery partition ------"${CL_RST}
	$(hide) rm -rf $(PRODUCT_OUT)/$(EXTRA_TMP)

	@echo -e ${PRT_IMG}"----- Making extra recovery partition ------"${CL_RST}
	$(hide) mkdir -p $(PRODUCT_OUT)/$(EXTRA_TMP)/sbin


	$(hide) $(foreach file,$(EXTRA_FILES), \
			mv $(TARGET_RECOVERY_ROOT_OUT)/sbin/$(file) $(PRODUCT_OUT)/$(EXTRA_TMP)/sbin/ ; )

	$(hide) $(foreach file,$(EXTRA_FILES), \
			ln -sf /$(EXTRA_SYMLINK_NAME)/sbin/$(file) -t $(TARGET_RECOVERY_ROOT_OUT)/sbin/ ; )

	$(hide) mv $(TARGET_RECOVERY_ROOT_OUT)/res $(PRODUCT_OUT)/$(EXTRA_TMP)/
	$(hide) ln -sf /$(EXTRA_SYMLINK_NAME)/res -t $(TARGET_RECOVERY_ROOT_OUT)
	$(hide) mv $(TARGET_RECOVERY_ROOT_OUT)/twres $(PRODUCT_OUT)/$(EXTRA_TMP)/
	$(hide) ln -sf /$(EXTRA_SYMLINK_NAME)/twres -t $(TARGET_RECOVERY_ROOT_OUT)

	@echo -e ${PRT_IMG}"----- Moving extra files to 'extra' directory ------"${CL_RST}
	$(hide) rm -rf $(PRODUCT_OUT)/$(EXTRA_OUT)
	$(hide) cp -r $(PRODUCT_OUT)/$(EXTRA_TMP) $(PRODUCT_OUT)/$(EXTRA_OUT)
endef

define revert-recoveryramdisk-custom
	@echo -e ${PRT_IMG}"----- Reverting extra recovery partition ------"${CL_RST}

	$(hide) rm -f $(TARGET_RECOVERY_ROOT_OUT)/twres
	$(hide) rm -f $(TARGET_RECOVERY_ROOT_OUT)/res
	$(hide) mv $(PRODUCT_OUT)/$(EXTRA_TMP)/twres $(TARGET_RECOVERY_ROOT_OUT)/
	$(hide) mv $(PRODUCT_OUT)/$(EXTRA_TMP)/res $(TARGET_RECOVERY_ROOT_OUT)/

	$(hide) $(foreach file,$(EXTRA_FILES), \
			rm -f $(TARGET_RECOVERY_ROOT_OUT)/sbin/$(file) ; )

	$(hide) $(foreach file,$(EXTRA_FILES), \
			mv $(PRODUCT_OUT)/$(EXTRA_TMP)/sbin/$(file) $(TARGET_RECOVERY_ROOT_OUT)/sbin/ ; )

	$(hide) rm -rf $(PRODUCT_OUT)/$(EXTRA_TMP)
endef

$(recovery_uncompressed_ramdisk): $(MKBOOTFS) \
		$(INSTALLED_BOOTIMAGE_TARGET) \
		$(INTERNAL_RECOVERYIMAGE_FILES) \
		$(recovery_initrc) $(recovery_sepolicy) $(recovery_kernel) \
		$(INSTALLED_2NDBOOTLOADER_TARGET) \
		$(recovery_build_prop) $(recovery_resource_deps) $(recovery_root_deps) \
		$(recovery_fstab) \
		$(RECOVERY_INSTALL_OTA_KEYS)
	$(call build-recoveryramdisk)
	$(call build-extra-recoveryramdisk)
	@echo -e ${PRT_IMG}"----- Making uncompressed recovery ramdisk ------"${CL_RST}
	$(hide) $(MKBOOTFS) $(TARGET_RECOVERY_ROOT_OUT) > $@

$(recovery_ramdisk): $(MINIGZIP) \
		$(recovery_uncompressed_ramdisk)
	@echo -e ${PRT_IMG}"----- Making compressed recovery ramdisk ------"${CL_RST}
	$(hide) $(MINIGZIP) < $(recovery_uncompressed_ramdisk) > $@

$(INSTALLED_RECOVERYIMAGE_TARGET): $(recovery_ramdisk) $(MKBOOTIMG) $(recovery_kernel)
	@echo -e ${PRT_IMG}"----- Making recovery image ------"${CL_RST}
	$(call build-recoveryimage-target, $@)
	@echo -e ${PRT_IMG}"----- Made recovery image: $@ --------"${CL_RST}
	$(call revert-recoveryramdisk-custom)
