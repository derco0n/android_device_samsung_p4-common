#define LOG_TAG "android.hardware.gnss@1.0-service"

#include <android/hardware/gnss/1.0/IGnss.h>

#include <hidl/LegacySupport.h>

#include <binder/ProcessState.h>

using android::hardware::gnss::V1_0::IGnss;
using android::hardware::defaultPassthroughServiceImplementation;

int main() {

    int pid = fork();

    if ( pid == 0 ) {
        execl("/system/vendor/bin/gpsd", "-c", "/system/vendor/etc/gps.xml", (char *)0);
    }

    // The GNSS HAL may communicate to other vendor components via
    // /dev/vndbinder
    android::ProcessState::initWithDriver("/dev/vndbinder");
    return defaultPassthroughServiceImplementation<IGnss>();
}
