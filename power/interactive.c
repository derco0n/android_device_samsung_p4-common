
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

#define CPUFREQ_INTERACTIVE "/sys/devices/system/cpu/cpufreq/interactive/"
#define BOOST_PATH      "/sys/devices/system/cpu/cpufreq/interactive/boost"
#define BOOSTPULSE_PATH "/sys/devices/system/cpu/cpufreq/interactive/boostpulse"
#define BOOST_DURATION_PATH "/sys/devices/system/cpu/cpufreq/interactive/boostpulse_duration"
// #define HISPEED_FREQ "/sys/devices/system/cpu/cpufreq/interactive/hispeed_freq"

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

void boost_on(struct p3_power_module *p3, void *data)
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

void boostpulse(struct p3_power_module *p3)
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

void boostpulse_init(struct p3_power_module *p3)
{
    char boostpulse_duration[32];

    if (sysfs_read(BOOST_DURATION_PATH, boostpulse_duration, 32) < 0) {
        /* above should not fail but just in case it does use an arbitrary 80ms value */
        snprintf(boostpulse_duration, 32, "%d", 80000);
    }
    p3->pulse_duration = atoi(boostpulse_duration);
    /* initialize last_boost_time */
    clock_gettime(CLOCK_MONOTONIC, &p3->last_boost_time);
}
