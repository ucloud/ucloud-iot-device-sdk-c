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

// Based on qcloud-iot-sdk-embedded-c
/*
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "mpu_wrappers.h"
#include "uiot_import.h"

bool HAL_Timer_Expired(Timer *timer) {
    TickType_t now;
    now = xTaskGetTickCount();
    return timer->end_time < now;
}

void HAL_Timer_Countdown_ms(Timer *timer, unsigned int timeout_ms) {
    TickType_t now;
    now = xTaskGetTickCount();
    timer->end_time = now + (timeout_ms / portTICK_RATE_MS);
}

void HAL_Timer_Countdown(Timer *timer, unsigned int timeout) {
    TickType_t now;
    now = xTaskGetTickCount();
    timer->end_time = now + (timeout * 1000 / portTICK_RATE_MS);
}

uint32_t HAL_Timer_Remain_ms(Timer *timer) {
    TickType_t now,result;
    now = xTaskGetTickCount();
    result = timer->end_time - now;
    return result;
}

void HAL_Timer_Init(Timer *timer) {
    timer->end_time = (uint64_t)0;
}

#ifdef __cplusplus
}
#endif
