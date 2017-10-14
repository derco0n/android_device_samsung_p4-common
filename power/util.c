
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

#include "power-common.h"
#include "util.h"

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

int sysfs_write(char *path, char *s)
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

void timespec_diff(struct timespec *result, struct timespec *start, struct timespec *stop)
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

unsigned long timespec_to_us(struct timespec *t) {
    unsigned long time_in_micros = (1000 * t->tv_sec) + (t->tv_nsec / 1000);
    return time_in_micros;
}
