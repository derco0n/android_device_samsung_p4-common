/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "lights.p3"

#include <hardware/vibrator.h>
#include <hardware/hardware.h>

#include <cutils/log.h>

#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#define DEFAULT_AMPLITUDE 200

static const char ENABLE[] = "/sys/class/leds/isa1200/enable";
static const char AMPLITUDE[] = "/sys/class/leds/isa1200/amplitude";

static int vibra_exists() {
    int fd;

    fd = TEMP_FAILURE_RETRY(open(ENABLE, O_WRONLY));
    if(fd < 0) {
        ALOGE("Vibrator file does not exist : %d", fd);
        return 0;
    }

    close(fd);
    return 1;
}

static int sendit(unsigned int timeout_ms, const char *path)
{
    int to_write, written, ret, fd;

    char value[20]; /* large enough for millions of years */

    fd = TEMP_FAILURE_RETRY(open(path, O_WRONLY));
    if(fd < 0) {
        return -errno;
    }

    to_write = snprintf(value, sizeof(value), "%u\n", timeout_ms);
    written = TEMP_FAILURE_RETRY(write(fd, value, to_write));

    if (written == -1) {
        ret = -errno;
    } else if (written != to_write) {
        /* even though EAGAIN is an errno value that could be set
           by write() in some cases, none of them apply here.  So, this return
           value can be clearly identified when debugging and suggests the
           caller that it may try to call vibraror_on() again */
        ret = -EAGAIN;
    } else {
        ret = 0;
    }

    errno = 0;
    close(fd);

    return ret;
}

static int vibra_on(vibrator_device_t* vibradev __unused, unsigned int timeout_ms)
{
    int res;
    /* constant on, up to maximum allowed time */
    res = sendit(timeout_ms, ENABLE);
    if (res)
        return res;

    return sendit(DEFAULT_AMPLITUDE, AMPLITUDE);
}

static int vibra_off(vibrator_device_t* vibradev __unused)
{
    return sendit(0, ENABLE);
}

static int vibra_close(hw_device_t *device)
{
    free(device);
    return 0;
}

static int vibra_open(const hw_module_t* module, const char* id __unused,
                      hw_device_t** device __unused) {
    if (!vibra_exists()) {
        ALOGE("Vibrator device does not exist. Cannot start vibrator");
        return -ENODEV;
    }

    vibrator_device_t *vibradev = calloc(1, sizeof(vibrator_device_t));

    if (!vibradev) {
        ALOGE("Can not allocate memory for the vibrator device");
        return -ENOMEM;
    }

    vibradev->common.tag = HARDWARE_DEVICE_TAG;
    vibradev->common.module = (hw_module_t *) module;
    vibradev->common.version = HARDWARE_DEVICE_API_VERSION(1,0);
    vibradev->common.close = vibra_close;

    vibradev->vibrator_on = vibra_on;
    vibradev->vibrator_off = vibra_off;

    *device = (hw_device_t *) vibradev;

    return 0;
}

/*===========================================================================*/
/* Default vibrator HW module interface definition                           */
/*===========================================================================*/

static struct hw_module_methods_t vibrator_module_methods = {
    .open = vibra_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = VIBRATOR_API_VERSION,
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = VIBRATOR_HARDWARE_MODULE_ID,
    .name = "Default vibrator HAL",
    .author = "The Android Open Source Project",
    .methods = &vibrator_module_methods,
};
