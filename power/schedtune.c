
#define LOG_TAG "P3PowerHAL"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <cutils/log.h>

#include "power.h"
#include "power-common.h"
#include "util.h"

#include "schedtune.h"

#define SCHEDTUNE_BOOST_PATH "/dev/stune/top-app/schedtune.boost"
#define SCHEDTUNE_BOOST_NORM "10"
#define SCHEDTUNE_BOOST_INTERACTIVE "40"
#define SCHEDTUNE_BOOST_TIME_NS 1000000000LL

#define NSEC_PER_SEC 1000000000LL
static long long gettime_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec;
}
static void nanosleep_ns(long long ns)
{
    struct timespec ts;
    ts.tv_sec = ns/NSEC_PER_SEC;
    ts.tv_nsec = ns%NSEC_PER_SEC;
    nanosleep(&ts, NULL);
}

int schedtune_sysfs_boost(struct p3_power_module *p3, char* booststr)
{
    char buf[80];
    int len;
    if (p3->schedtune_boost_fd < 0)
        return p3->schedtune_boost_fd;
    len = write(p3->schedtune_boost_fd, booststr, 2);
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", SCHEDTUNE_BOOST_PATH, buf);
    }
    return len;
}
static void* schedtune_deboost_thread(void* arg)
{
    struct p3_power_module *p3 = (struct p3_power_module *)arg;
    while(1) {
        sem_wait(&p3->signal_lock);
        while(1) {
            long long now, sleeptime = 0;
            pthread_mutex_lock(&p3->lock);
            now = gettime_ns();
            if (p3->deboost_time > now) {
                sleeptime = p3->deboost_time - now;
                pthread_mutex_unlock(&p3->lock);
                nanosleep_ns(sleeptime);
                continue;
            }
            schedtune_sysfs_boost(p3, SCHEDTUNE_BOOST_NORM);
            p3->deboost_time = 0;
            pthread_mutex_unlock(&p3->lock);
            break;
        }
    }
    return NULL;
}

int schedtune_boost(struct p3_power_module *p3, long long boost_time)
{
    long long now;
    if (p3->schedtune_boost_fd < 0)
        return p3->schedtune_boost_fd;
    now = gettime_ns();
    if (!p3->deboost_time) {
        schedtune_sysfs_boost(p3, SCHEDTUNE_BOOST_INTERACTIVE);
        sem_post(&p3->signal_lock);
    }
    p3->deboost_time = now + boost_time;
    return 0;
}

void schedtune_power_init(struct p3_power_module *p3)
{
    char buf[50];
    pthread_t tid;
    p3->deboost_time = 0;
    sem_init(&p3->signal_lock, 0, 1);
    p3->schedtune_boost_fd = open(SCHEDTUNE_BOOST_PATH, O_WRONLY);
    if (p3->schedtune_boost_fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", SCHEDTUNE_BOOST_PATH, buf);
    }
    pthread_create(&tid, NULL, schedtune_deboost_thread, p3);
}
