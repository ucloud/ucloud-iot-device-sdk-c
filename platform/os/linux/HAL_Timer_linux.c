/*
* Copyright (C) 2012-2019 UCloud. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <sys/time.h>

#include "uiot_import.h"

bool HAL_Timer_Expired(Timer *timer) {
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&timer->end_time, &now, &res);
    return res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0);
}

void HAL_Timer_Countdown_ms(Timer *timer, unsigned int timeout_ms) {
    struct timeval now;
    gettimeofday(&now, NULL);
#ifdef __cplusplus
    struct timeval interval = {timeout_ms / 1000, static_cast<int>((timeout_ms % 1000) * 1000)};
#else
    struct timeval interval = {timeout_ms / 1000, (int) ((timeout_ms % 1000) * 1000)};
#endif
    timeradd(&now, &interval, &timer->end_time);
}

void HAL_Timer_Countdown(Timer *timer, unsigned int timeout) {
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval interval = {timeout, 0};
    timeradd(&now, &interval, &timer->end_time);
}

uint32_t HAL_Timer_Remain_ms(Timer *timer) {
    struct timeval now, res;
    uint32_t result_ms = 0;
    gettimeofday(&now, NULL);
    timersub(&timer->end_time, &now, &res);
    if (res.tv_sec >= 0) {
        result_ms = (uint32_t) (res.tv_sec * 1000 + res.tv_usec / 1000);
    }
    return result_ms;
}

void HAL_Timer_Init(Timer *timer) {
    timer->end_time = (struct timeval) {0, 0};
}

#ifdef __cplusplus
}
#endif
