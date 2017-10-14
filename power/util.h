
#ifndef __UTIL_H__
#define __UTIL_H__

int sysfs_read(const char *path, char *buf, ssize_t size);
int sysfs_write(char *path, char *s);

void timespec_diff(struct timespec *result, struct timespec *start, struct timespec *stop);
unsigned long timespec_to_us(struct timespec *t);

#endif /* __UTIL_H__ */