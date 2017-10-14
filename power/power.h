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

#ifndef __POWER_H__
#define __POWER_H__

#include <semaphore.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

struct p3_power_module {
    struct power_module base;
    pthread_mutex_t lock;

    bool low_power_mode;

    /* interactive governor boost values */
    int boost_fd;
    int boost_warned;
    int boostpulse_fd;
    int boostpulse_warned;
    uint32_t pulse_duration;
    struct timespec last_boost_time; /* latest POWER_HINT_INTERACTION boost */

    /* EAS schedtune values */
    int schedtune_boost_fd;
    long long deboost_time;
    sem_t signal_lock;
};



#endif /* __POWER_H__ */