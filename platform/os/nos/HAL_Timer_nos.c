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

#include "uiot_import.h"
#include "stm32f1xx_hal.h"

bool HAL_Timer_Expired(Timer *timer) {
    uint32_t now;
    now = HAL_GetTick();
    return timer->end_time < now;
}

void HAL_Timer_Countdown_ms(Timer *timer, unsigned int timeout_ms) {
    uint32_t now;
    now = HAL_GetTick();
    timer->end_time = now + (timeout_ms / HAL_GetTickFreq());
}

void HAL_Timer_Countdown(Timer *timer, unsigned int timeout) {
    uint32_t now;
    now = HAL_GetTick();
    timer->end_time = now + (timeout * 1000 / HAL_GetTickFreq());
}

uint32_t HAL_Timer_Remain_ms(Timer *timer) {
    uint32_t now;
    now = HAL_GetTick();
    uint32_t result = timer->end_time - now;
    return result;
}

void HAL_Timer_Init(Timer *timer) {
    timer->end_time = (uint32_t)0;
}

#ifdef __cplusplus
}
#endif
