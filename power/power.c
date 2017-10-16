/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "P3PowerHAL"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <cutils/log.h>

#include "power.h"
#include "power-common.h"
#include "util.h"

#include "interactive.h"
#include "schedtune.h"

#define TOUCH_SUSPEND_PATH "/sys/bus/i2c/drivers/sec_touch/4-004c/mxt1386/suspended"
#define MPU3050_SUSPEND_PATH "/sys/bus/i2c/drivers/mpu3050/0-0068/mpu3050/suspended"

#define LOW_POWER_MAX_FREQ "456000"
#define LOW_POWER_MIN_FREQ "150000"
#define NORMAL_MAX_FREQ "1000000"

static char screen_off_max_freq[MAX_BUF_SZ] = "456000";
static char scaling_max_freq[MAX_BUF_SZ] = "1000000";
static char normal_max_freq[MAX_BUF_SZ] = "1000000";

#define NSEC_PER_SEC 1000000000LL

static void store_max_freq(char* max_freq, bool low_power_mode)
{
    int len;
    char buf[MAX_BUF_SZ];

    /* read the current scaling max freq */
    len = sysfs_read(CPU0_SCALINGMAXFREQ_PATH, buf, sizeof(buf));

    /* make sure it's not the screen off freq, if the "on"
     * call is skipped (can happen if you press the power
     * button repeatedly) we might have read it. We should
     * skip it if that's the case
     */
    if (len != -1 && strncmp(buf, screen_off_max_freq,
            strlen(screen_off_max_freq)) != 0
            && !low_power_mode)
        memcpy(max_freq, buf, sizeof(buf));
}

static void p3_power_init(struct power_module *module)
{
    struct p3_power_module *p3 =
            (struct p3_power_module *) module;

    store_max_freq(scaling_max_freq, p3->low_power_mode);
    ALOGI("%s: stored scaling_max_freq = %s", __FUNCTION__, scaling_max_freq);

    /*
     * cpufreq interactive governor: timer 20ms, min sample 30ms.
     */

    // sysfs_write(CPUFREQ_INTERACTIVE "timer_rate", "30000");
    // sysfs_write(CPUFREQ_INTERACTIVE "min_sample_time", "40000");
    // sysfs_write(CPUFREQ_INTERACTIVE "go_hispeed_load", "80");

    boostpulse_init(p3);

    schedtune_power_init(p3);
}

static void p3_power_set_interactive(struct power_module *module, int on)
{
    struct p3_power_module *p3 =
            (struct p3_power_module *) module;

    /*
     * Lower maximum frequency when screen is off.  CPU 0 and 1 share a
     * cpufreq policy.
     */
    if (!on) {
        store_max_freq(scaling_max_freq, p3->low_power_mode);

        sysfs_write(CPU0_SCALINGMAXFREQ_PATH, screen_off_max_freq);
        sysfs_write(CPU1_SCALINGMAXFREQ_PATH, screen_off_max_freq);
        // sysfs_write(CPUFREQ_INTERACTIVE "go_hispeed_load", "99");

        sysfs_write(TOUCH_SUSPEND_PATH, "1");
        sysfs_write(MPU3050_SUSPEND_PATH, "1");
    } else {
        if (p3->low_power_mode) {
            store_max_freq(scaling_max_freq, p3->low_power_mode);

            sysfs_write(CPU0_SCALINGMAXFREQ_PATH, LOW_POWER_MAX_FREQ);
            sysfs_write(CPU1_SCALINGMAXFREQ_PATH, LOW_POWER_MAX_FREQ);
            // sysfs_write(CPUFREQ_INTERACTIVE "go_hispeed_load", "99");
        } else {
            sysfs_write(CPU0_SCALINGMAXFREQ_PATH, scaling_max_freq);
            sysfs_write(CPU1_SCALINGMAXFREQ_PATH, scaling_max_freq);
            // sysfs_write(CPUFREQ_INTERACTIVE "go_hispeed_load", "80");
        }

        sysfs_write(TOUCH_SUSPEND_PATH, "0");
        sysfs_write(MPU3050_SUSPEND_PATH, "0");
    }
}

static void p3_power_hint(struct power_module *module, power_hint_t hint,
                            void *data)
{
    struct p3_power_module *p3 =
            (struct p3_power_module *) module;

    switch (hint) {
    case POWER_HINT_VSYNC:
        break;
    case POWER_HINT_INTERACTION:
        ALOGV("POWER_HINT_INTERACTION\n");
        // boostpulse(p3);
        schedtune_boost(p3, 1*NSEC_PER_SEC);
        break;
    case POWER_HINT_LOW_POWER:
        pthread_mutex_lock(&p3->lock);
        if (data) {
            store_max_freq(scaling_max_freq, p3->low_power_mode);

            p3->low_power_mode = true;
            sysfs_write(CPU0_SCALINGMAXFREQ_PATH, LOW_POWER_MAX_FREQ);
            sysfs_write(CPU1_SCALINGMAXFREQ_PATH, LOW_POWER_MAX_FREQ);
            // sysfs_write(CPUFREQ_INTERACTIVE "hispeed_freq", LOW_POWER_MAX_FREQ);
        } else {
            p3->low_power_mode = false;
            sysfs_write(CPU0_SCALINGMAXFREQ_PATH, normal_max_freq);
            sysfs_write(CPU1_SCALINGMAXFREQ_PATH, normal_max_freq);
            // sysfs_write(CPUFREQ_INTERACTIVE "hispeed_freq", NORMAL_MAX_FREQ);
        }
        pthread_mutex_unlock(&p3->lock);
        break;

    case POWER_HINT_LAUNCH:
        ALOGV("POWER_HINT_LAUNCH\n");
        // boost_on(p3, data);
        schedtune_boost(p3, 5*NSEC_PER_SEC);
        break;
    default:
        break;
    }
}

static int power_open(const hw_module_t* module, const char* name,
                    hw_device_t** device)
{
    ALOGD("%s: enter; name=%s", __FUNCTION__, name);
    int retval = 0; /* 0 is ok; -1 is error */

    if (strcmp(name, POWER_HARDWARE_MODULE_ID) == 0) {
        power_module_t *dev = (power_module_t *)calloc(1,
                sizeof(power_module_t));

        if (dev) {
            /* Common hw_device_t fields */
            dev->common.tag = HARDWARE_DEVICE_TAG;
            dev->common.module_api_version = POWER_MODULE_API_VERSION_0_2;
            dev->common.module_api_version = HARDWARE_HAL_API_VERSION;

            dev->init = p3_power_init;
            dev->powerHint = p3_power_hint;
            dev->setInteractive = p3_power_set_interactive;

            *device = (hw_device_t*)dev;
        } else
            retval = -ENOMEM;
    } else {
        retval = -EINVAL;
    }

    ALOGD("%s: exit %d", __FUNCTION__, retval);
    return retval;
}

static struct hw_module_methods_t power_module_methods = {
    .open = power_open,
};

struct p3_power_module HAL_MODULE_INFO_SYM = {
    .base = {
        .common = {
            .tag = HARDWARE_MODULE_TAG,
            .module_api_version = POWER_MODULE_API_VERSION_0_2,
            .hal_api_version = HARDWARE_HAL_API_VERSION,
            .id = POWER_HARDWARE_MODULE_ID,
            .name = "P3 Power HAL",
            .author = "The Android Open Source Project",
            .methods = &power_module_methods,
        },
        .init = p3_power_init,
        .setInteractive = p3_power_set_interactive,
        .powerHint = p3_power_hint,
    },

    .lock = PTHREAD_MUTEX_INITIALIZER,

    .low_power_mode = false,

    .boost_fd = -1,
    .boost_warned = 0,
    .boostpulse_fd = -1,
    .boostpulse_warned = 0,
};
