/*
 * Copyright (C) 2015 The Android Open Source Project
 * Written by ryang <decatf@gmail.com>
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

#define LOG_TAG "macloader"
// #define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <unistd.h>
#include <pwd.h>

#include <cutils/log.h>

#define MAC_LEN 18
#define MAC_MATCH "[a-f0-9][a-f0-9]:[a-f0-9][a-f0-9]:[a-f0-9][a-f0-9]:[a-f0-9][a-f0-9]:[a-f0-9][a-f0-9]:[a-f0-9][a-f0-9]"

// wifi driver reads mac from this file
#define MACINFO_EFS "/efs/wifi/.mac.info"

// alternate mac address source
#define MACCOB_EFS "/efs/wifi/.mac.cob"
#define NVMACINFO_EFS "/efs/wifi/.nvmac.info"


int validate_mac(const char* mac) {
    int res = -1;

    if (!strncmp(mac, "00:00:00:00:00:00", 17)) {
        ALOGV("%s: WIFI mac is NOT valid.", __FUNCTION__);
        return -1;
    }

    res = fnmatch(MAC_MATCH, mac, FNM_CASEFOLD);
    if (!res) {
        ALOGV("%s: WIFI mac is valid.", __FUNCTION__);
        return 0;
    }

    ALOGV("%s: WIFI mac is NOT valid.", __FUNCTION__);

    return -1;
}

int get_alt_mac(const char* path, const char* buf, int len) {
    int ret = -1;
    FILE* file;
    char *str = NULL;

    file = fopen(path, "r");
    if (file != NULL) {
        str = fgets(buf, len, file);
        if (str) {
            ALOGV("mac(%s) from %s", buf, path);
            ret = validate_mac(buf);
        }
        fclose(file);
    }

    return ret;
}

int generate_mac(const char* buf, int len) {

    unsigned int rand_bytes[3];
    int i = 0;
    int n = sizeof(rand_bytes) / sizeof(rand_bytes[0]);

    srand48(time(NULL));
    while (i < n) {
        rand_bytes[i] = (unsigned int) (lrand48() % 0x100);
        i++;
    }

    unsigned int a = rand_bytes[0];
    unsigned int b = rand_bytes[1];
    unsigned int c = rand_bytes[2];

    sprintf((char *)buf, "00:12:34:%02X:%02X:%02X", a, b, c);
    ALOGV("Random bytes are %02x.%02x.%02x", a, b, c);

    return 0;
}

int main() {
    int res;
    FILE* file;
    char *str;
    char mac[MAC_LEN];
    const char* path;

    path = MACINFO_EFS;

    file = fopen(path, "r");
    if (file == NULL) {
        ALOGE("Can't open mac file %s.", path);

        if (access(path, F_OK) != -1) {
            ALOGE("mac file exists. check permissions.");
            return 1;
        }
    } else {
        str = fgets(mac, sizeof(mac), file);
        if (str) {
            ALOGV("mac(%s) from %s", mac, path);

            if (!validate_mac(mac)) {
                ALOGI("Valid mac found in %s", path);
                fclose(file);
                return 0;
            }

            fclose(file);
        }
    }

    ALOGV("try to load mac from alternate locations.");

    res = get_alt_mac(MACCOB_EFS, mac, sizeof(mac));
    if (!res)
        ALOGI("Got a valid mac address from %s\n", MACCOB_EFS);

    if (res < 0) {
        res = get_alt_mac(NVMACINFO_EFS, mac, sizeof(mac));
        if (!res)
            ALOGI("Got a valid mac address from %s\n", NVMACINFO_EFS);
    }

    if (res < 0) {
        res = generate_mac(mac, sizeof(mac));
        if (res != 0) {
            ALOGE("Failed to generate a mac address.");
            return 1;
        }

        res = validate_mac(mac);
        if (res != 0) {
            ALOGE("Could not validate randomly generate mac address.");
            return 1;
        }

        ALOGI("Generated a mac address.");
    }

    if (res == 0) {
        int fd;
        int amode;
        struct passwd *pwd;

        path = MACINFO_EFS;
        file = fopen(path, "w");

        ALOGV("Writing mac (%s) to %s", mac, path);

        if (file == NULL) {
            ALOGE("Can't access mac file %s.", path);
            return -1;
        }

        res = fputs(mac, file);
        if (res < 0) {
            fclose(file);
            ALOGE("Error writing to file %s\n", path);
        } else
            ALOGI("Wrote mac to %s", path);

        //
        // set the file permissions
        //
        fd = fileno(file);
        amode = S_IRUSR | S_IWUSR | S_IRGRP;
        res = fchmod(fd, amode);
        if (res != 0) {
            fclose(file);
            ALOGE("Can't set permissions on %s\n", path);
            return 1;
        }

        pwd = getpwnam("wifi");
        if (pwd == NULL) {
            fclose(file);
            ALOGE("Failed to find 'system' user\n");
            return 1;
        }

        res = fchown(fd, pwd->pw_uid, pwd->pw_gid);
        if (res != 0) {
            fclose(file);
            ALOGE("Failed to change owner of %s\n",
                  path);
            return 1;
        }

        fclose(file);
    }

    return 0;
}
