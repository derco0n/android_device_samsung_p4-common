
EXTRA_FILES := \
  adbd \
  dosfsck \
  e2fsck \
  healthd \
  mkdosfs \
  mke2fs \
  pigz \
  tune2fs \
  recovery \
  mkfs.f2fs \
  busybox \
  linker \
  fsck.f2fs \
  libcrypto.so

define build-recoveryramdisk
  @echo -e ${PRT_IMG}"----- Making recovery ramdisk ------"${CL_RST}
  $(hide) mkdir -p $(TARGET_RECOVERY_OUT)
  $(hide) mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/etc $(TARGET_RECOVERY_ROOT_OUT)/tmp
  @echo -e ${PRT_IMG}"Copying baseline ramdisk..."${CL_RST}
  $(hide) rsync -a $(TARGET_ROOT_OUT) $(TARGET_RECOVERY_OUT) # "cp -Rf" fails to overwrite broken symlinks on Mac.
  @echo -e ${PRT_IMG}"Modifying ramdisk contents..."${CL_RST}
  $(hide) rm -f $(TARGET_RECOVERY_ROOT_OUT)/init*.rc
  $(hide) cp -f $(recovery_initrc) $(TARGET_RECOVERY_ROOT_OUT)/
  $(hide) rm -f $(TARGET_RECOVERY_ROOT_OUT)/sepolicy
  $(hide) cp -f $(recovery_sepolicy) $(TARGET_RECOVERY_ROOT_OUT)/sepolicy
  $(hide) cp $(TARGET_ROOT_OUT)/init.recovery.*.rc $(TARGET_RECOVERY_ROOT_OUT)/ || true # Ignore error when the src file doesn't exist.
  $(hide) mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/res
  $(hide) rm -rf $(TARGET_RECOVERY_ROOT_OUT)/res/*
  $(hide) cp -rf $(recovery_resources_common)/* $(TARGET_RECOVERY_ROOT_OUT)/res
  $(hide) $(foreach item,$(recovery_root_private), \
    cp -rf $(item) $(TARGET_RECOVERY_OUT)/)
  $(hide) $(foreach item,$(recovery_resources_private), \
    cp -rf $(item) $(TARGET_RECOVERY_ROOT_OUT)/)
  $(hide) $(foreach item,$(recovery_fstab), \
    cp -f $(item) $(TARGET_RECOVERY_ROOT_OUT)/etc/recovery.fstab)
  $(hide) cp $(RECOVERY_INSTALL_OTA_KEYS) $(TARGET_RECOVERY_ROOT_OUT)/res/keys
  $(hide) cat $(INSTALLED_DEFAULT_PROP_TARGET) $(recovery_build_prop) \
          > $(TARGET_RECOVERY_ROOT_OUT)/default.prop
  $(hide) $(SED_INPLACE) 's/ro.build.date.utc=.*/ro.build.date.utc=0/g' $(TARGET_RECOVERY_ROOT_OUT)/default.prop
  $(hide) $(SED_INPLACE) 's/ro.adb.secure=1/ro.adb.secure=0/g' $(TARGET_RECOVERY_ROOT_OUT)/default.prop

  $(hide) rm -rf $(PRODUCT_OUT)/extra
  $(hide) mkdir -p $(PRODUCT_OUT)/extra/sbin

  $(hide) $(foreach item,$(EXTRA_FILES), \
    mv $(TARGET_RECOVERY_ROOT_OUT)/sbin/$(item) $(PRODUCT_OUT)/extra/sbin/$(item) & )

  $(hide) $(foreach item,$(EXTRA_FILES), \
    ln -sf /extra/sbin/$(item) $(TARGET_RECOVERY_ROOT_OUT)/sbin/ & )

  $(hide) mv $(TARGET_RECOVERY_ROOT_OUT)/res $(PRODUCT_OUT)/extra/
  $(hide) ln -sf /extra/res -t $(TARGET_RECOVERY_ROOT_OUT)
  $(hide) mv $(TARGET_RECOVERY_ROOT_OUT)/twres $(PRODUCT_OUT)/extra/
  $(hide) ln -sf /extra/twres -t $(TARGET_RECOVERY_ROOT_OUT)
  $(hide) mv $(TARGET_RECOVERY_ROOT_OUT)/supersu $(PRODUCT_OUT)/extra/
  $(hide) ln -sf /extra/supersu -t $(TARGET_RECOVERY_ROOT_OUT)

  $(hide) mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/system/bin
  $(hide) ln -sf /sbin/busybox $(TARGET_RECOVERY_ROOT_OUT)/system/bin/sh
endef

define clean-extras
  @echo -e ${PRT_IMG}"----- Clean extras ------"${CL_RST}

  rm $(TARGET_RECOVERY_ROOT_OUT)/res
  rm $(TARGET_RECOVERY_ROOT_OUT)/twres
  rm $(TARGET_RECOVERY_ROOT_OUT)/supersu

  $(foreach item,$(EXTRA_FILES), \
    rm $(TARGET_RECOVERY_OUT)/sbin/$(item) & )
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
