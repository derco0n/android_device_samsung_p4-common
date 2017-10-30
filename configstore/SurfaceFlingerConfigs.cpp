#include "SurfaceFlingerConfigs.h"

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace configstore {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::configstore::V1_0::ISurfaceFlingerConfigs follow.
Return<void> SurfaceFlingerConfigs::vsyncEventPhaseOffsetNs(vsyncEventPhaseOffsetNs_cb _hidl_cb) {
    _hidl_cb({true, 0});
    return Void();
}

Return<void> SurfaceFlingerConfigs::vsyncSfEventPhaseOffsetNs(vsyncEventPhaseOffsetNs_cb _hidl_cb) {
    _hidl_cb({true, 0});
    return Void();
}

Return<void> SurfaceFlingerConfigs::useContextPriority(useContextPriority_cb _hidl_cb) {
    _hidl_cb({false, false});
    return Void();
}

Return<void> SurfaceFlingerConfigs::maxFrameBufferAcquiredBuffers(maxFrameBufferAcquiredBuffers_cb _hidl_cb) {
    _hidl_cb({true, 3});
    return Void();
}

Return<void> SurfaceFlingerConfigs::hasWideColorDisplay(hasWideColorDisplay_cb _hidl_cb) {
    bool value = false;
    _hidl_cb({true, value});
    return Void();
}

Return<void> SurfaceFlingerConfigs::hasSyncFramework(hasSyncFramework_cb _hidl_cb) {
    bool value = false;
    _hidl_cb({true, value});
    return Void();
}

Return<void> SurfaceFlingerConfigs::hasHDRDisplay(hasHDRDisplay_cb _hidl_cb) {
    bool value = false;
    _hidl_cb({true, value});
    return Void();
}

Return<void> SurfaceFlingerConfigs::presentTimeOffsetFromVSyncNs(presentTimeOffsetFromVSyncNs_cb _hidl_cb) {
      _hidl_cb({false, 0});
      return Void();
}

Return<void> SurfaceFlingerConfigs::useHwcForRGBtoYUV(useHwcForRGBtoYUV_cb _hidl_cb) {
    bool value = false;
    _hidl_cb({true, value});
    return Void();
}

Return<void> SurfaceFlingerConfigs::maxVirtualDisplaySize(maxVirtualDisplaySize_cb _hidl_cb) {
  uint64_t maxSize = 0;
  _hidl_cb({false, maxSize});
  return Void();
}

Return<void> SurfaceFlingerConfigs::useVrFlinger(useVrFlinger_cb _hidl_cb) {
    bool value = false;
    bool specified = false;
    _hidl_cb({specified, value});
    return Void();
}

Return<void> SurfaceFlingerConfigs::startGraphicsAllocatorService(
    startGraphicsAllocatorService_cb _hidl_cb) {
    bool value = false;
#ifdef START_GRAPHICS_ALLOCATOR_SERVICE
    value = true;
#endif
    _hidl_cb({true, value});
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace configstore
}  // namespace hardware
}  // namespace android
