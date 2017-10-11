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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

#define LOG_TAG "P3PowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define CPU0_SCALINGMAXFREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define CPU1_SCALINGMAXFREQ_PATH "/sys/devices/system/cpu/cpu1/cpufreq/scaling_max_freq"
#define CPUFREQ_INTERACTIVE "/sys/devices/system/cpu/cpufreq/interactive/"
#define BOOST_PATH      "/sys/devices/system/cpu/cpufreq/interactive/boost"
#define BOOSTPULSE_PATH "/sys/devices/system/cpu/cpufreq/interactive/boostpulse"
#define BOOST_DURATION_PATH "/sys/devices/system/cpu/cpufreq/interactive/boostpulse_duration"
// #define HISPEED_FREQ "/sys/devices/system/cpu/cpufreq/interactive/hispeed_freq"

#define TOUCH_SUSPEND_PATH "/sys/bus/i2c/drivers/sec_touch/4-004c/mxt1386/suspended"
#define MPU3050_SUSPEND_PATH "/sys/bus/i2c/drivers/mpu3050/0-0068/mpu3050/suspended"

#define LOW_POWER_MAX_FREQ "456000"
#define LOW_POWER_MIN_FREQ "150000"
#define NORMAL_MAX_FREQ "1000000"

#define MAX_BUF_SZ  10

/* initialize to something safe */
static char screen_off_max_freq[MAX_BUF_SZ] = "456000";
static char scaling_max_freq[MAX_BUF_SZ] = "1000000";
static char normal_max_freq[MAX_BUF_SZ] = "1000000";

static bool low_power_mode = false;

struct p3_power_module {
    struct power_module base;
    pthread_mutex_t lock;
    int boost_fd;
    int boost_warned;
    int boostpulse_fd;
    int boostpulse_warned;
    uint32_t pulse_duration;
    struct timespec last_boost_time; /* latest POWER_HINT_INTERACTION boost */
};

int sysfs_read(const char *path, char *buf, ssize_t size)
{
    int fd;
    ssize_t len;

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    do {
        len = read(fd, buf, size);
    } while (len < 0 && errno == EINTR);

    close(fd);

    if (len >= 0 && len < size)
        memset(buf+len, 0, size - len);

    return len;
}

static int sysfs_write(char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return -1;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static void store_max_freq(char* max_freq)
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

static int boostpulse_open(struct p3_power_module *p3)
{
    char buf[80];

    pthread_mutex_lock(&p3->lock);

    if (p3->boostpulse_fd < 0) {
        p3->boostpulse_fd = open(BOOSTPULSE_PATH, O_WRONLY);

        if (p3->boostpulse_fd < 0) {
            if (!p3->boostpulse_warned) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error opening %s: %s\n", BOOSTPULSE_PATH, buf);
                p3->boostpulse_warned = 1;
            }
        }
    }

    pthread_mutex_unlock(&p3->lock);
    return p3->boostpulse_fd;
}

static void boostpulse_close(struct p3_power_module *p3)
{
    pthread_mutex_lock(&p3->lock);
    if (p3->boostpulse_fd >= 0) {
        close(p3->boostpulse_fd);
        p3->boostpulse_fd = -1;
    }
    pthread_mutex_unlock(&p3->lock);
}

static int boost_open(struct p3_power_module *p3)
{
    char buf[80];

    pthread_mutex_lock(&p3->lock);

    if (p3->boost_fd < 0) {
        p3->boost_fd = open(BOOST_PATH, O_RDWR);

        if (p3->boost_fd < 0) {
            if (!p3->boost_warned) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error opening %s: %s\n", BOOST_PATH, buf);
                p3->boost_warned = 1;
            }
        }
    }

    pthread_mutex_unlock(&p3->lock);
    return p3->boost_fd;
}

static void boost_close(struct p3_power_module *p3)
{
    pthread_mutex_lock(&p3->lock);
    if (p3->boost_fd >= 0) {
        close(p3->boost_fd);
        p3->boost_fd = -1;
    }
    pthread_mutex_unlock(&p3->lock);
}

static void boost_on(struct p3_power_module *p3, void *data)
{
    int boost_req_on;
    char buf[MAX_BUF_SZ];
    ssize_t res = 0;
    int boost_mode = -1;
    char *endptr;
    long ret;

    if (boost_open(p3) < 0){
        if (!p3->boost_warned)
            ALOGE("%s: Error opening %s\n", __FUNCTION__, BOOST_PATH);
        return;
    }

    res = sysfs_read(BOOST_PATH, buf, MAX_BUF_SZ);
    if (res == -1) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("%s: Error reading %s: %s\n", __FUNCTION__, BOOST_PATH, buf);
        boost_close(p3);
        return;
    }

    boost_req_on = data != NULL;
    ALOGV("%s: boost_req_on = %d\n", __FUNCTION__, boost_req_on);

    ret = strtol(buf, &endptr, 10);
    boost_mode = ret != 0;
    ALOGV("%s: boost_mode = %d\n", __FUNCTION__, boost_mode);

    if (boost_req_on && !boost_mode)
        res = write(p3->boost_fd, "1", 1);
    else if (!boost_req_on && boost_mode)
        res = write(p3->boost_fd, "0", 1);

    if (res < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("%s: Error writing to %s: %s\n", __FUNCTION__, BOOST_PATH, buf);
        boost_close(p3);
        return;
    }

    // res = sysfs_read(BOOST_PATH, buf, MAX_BUF_SZ);
    // ALOGD("%s: boost = %s\n", __FUNCTION__, buf);
}

static void timespec_diff(struct timespec *result, struct timespec *start, struct timespec *stop)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

static unsigned long timespec_to_us(struct timespec *t) {
    unsigned long time_in_micros = (1000 * t->tv_sec) + (t->tv_nsec / 1000);
    return time_in_micros;
}

static void boostpulse(struct p3_power_module *p3)
{
    char buf[80];
    struct timespec curr_time;
    struct timespec diff_time;
    uint64_t diff;
    int len = 0;

    if (boostpulse_open(p3) < 0) {
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    timespec_diff(&diff_time, &curr_time, &p3->last_boost_time);
    diff = timespec_to_us(&diff_time);

    if (diff > p3->pulse_duration) {
        len = write(p3->boostpulse_fd, "1", 1);
        p3->last_boost_time = curr_time;
    }

    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        if (!p3->boostpulse_warned)
            ALOGE("Error writing to %s: %s\n", BOOSTPULSE_PATH, buf);

        boostpulse_close(p3);
    }
}

static void p3_power_init( __attribute__((unused)) struct power_module *module)
{
    struct p3_power_module *p3 =
            (struct p3_power_module *) module;
    char boostpulse_duration[32];

    store_max_freq(scaling_max_freq);
    ALOGI("%s: stored scaling_max_freq = %s", __FUNCTION__, scaling_max_freq);

    /*
     * cpufreq interactive governor: timer 20ms, min sample 30ms.
     */

    sysfs_write(CPUFREQ_INTERACTIVE "timer_rate", "30000");
    sysfs_write(CPUFREQ_INTERACTIVE "min_sample_time", "40000");
    sysfs_write(CPUFREQ_INTERACTIVE "go_hispeed_load", "80");

    if (sysfs_read(BOOST_DURATION_PATH, boostpulse_duration, 32) < 0) {
        /* above should not fail but just in case it does use an arbitrary 80ms value */
        snprintf(boostpulse_duration, 32, "%d", 80000);
    }
    p3->pulse_duration = atoi(boostpulse_duration);
    /* initialize last_boost_time */
    clock_gettime(CLOCK_MONOTONIC, &p3->last_boost_time);
}

static void p3_power_set_interactive( __attribute__((unused)) struct power_module *module, int on)
{
    /*
     * Lower maximum frequency when screen is off.  CPU 0 and 1 share a
     * cpufreq policy.
     */
    if (!on) {
        store_max_freq(scaling_max_freq);

        sysfs_write(CPU0_SCALINGMAXFREQ_PATH, screen_off_max_freq);
        sysfs_write(CPU1_SCALINGMAXFREQ_PATH, screen_off_max_freq);
        sysfs_write(CPUFREQ_INTERACTIVE "go_hispeed_load", "99");

        sysfs_write(TOUCH_SUSPEND_PATH, "1");
        sysfs_write(MPU3050_SUSPEND_PATH, "1");
    } else if (low_power_mode) {
        store_max_freq(scaling_max_freq);

        sysfs_write(CPU0_SCALINGMAXFREQ_PATH, LOW_POWER_MAX_FREQ);
        sysfs_write(CPU1_SCALINGMAXFREQ_PATH, LOW_POWER_MAX_FREQ);
        sysfs_write(CPUFREQ_INTERACTIVE "go_hispeed_load", "99");
    } else {
        sysfs_write(CPU0_SCALINGMAXFREQ_PATH, scaling_max_freq);
        sysfs_write(CPU1_SCALINGMAXFREQ_PATH, scaling_max_freq);
        sysfs_write(CPUFREQ_INTERACTIVE "go_hispeed_load", "80");

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
#if 0
    case POWER_HINT_INTERACTION:
        boostpulse(p3);
        break;
#endif
    case POWER_HINT_LOW_POWER:
        pthread_mutex_lock(&p3->lock);
        if (data) {
            store_max_freq(normal_max_freq);

            low_power_mode = true;
            sysfs_write(CPU0_SCALINGMAXFREQ_PATH, LOW_POWER_MAX_FREQ);
            sysfs_write(CPU1_SCALINGMAXFREQ_PATH, LOW_POWER_MAX_FREQ);
            sysfs_write(CPUFREQ_INTERACTIVE "hispeed_freq", LOW_POWER_MAX_FREQ);
        } else {
            low_power_mode = false;
            sysfs_write(CPU0_SCALINGMAXFREQ_PATH, normal_max_freq);
            sysfs_write(CPU1_SCALINGMAXFREQ_PATH, normal_max_freq);
            sysfs_write(CPUFREQ_INTERACTIVE "hispeed_freq", NORMAL_MAX_FREQ);
        }
        pthread_mutex_unlock(&p3->lock);
        break;

    case POWER_HINT_LAUNCH:
        ALOGV("POWER_HINT_LAUNCH\n");
        boost_on(p3, data);
        break;
    default:
        break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
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
    .boost_fd = -1,
    .boost_warned = 0,
    .boostpulse_fd = -1,
    .boostpulse_warned = 0,
};
