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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>

#include <pthread.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "uiot_defs.h"
#include "uiot_import.h"

#define DEBUG_DEV_INFO_USED
// 测试时可在此处填入设备四元组
#ifdef DEBUG_DEV_INFO_USED
/* 产品序列号 */
static char sg_product_sn[IOT_PRODUCT_SN_LEN + 1] = "YOUR_PRODUCT_SN";
/* 产品密钥 */
static char sg_product_secret[IOT_PRODUCT_SECRET_LEN + 1] = "PRODUCT_SECRET";
/* 设备序列号 */
static char sg_device_sn[IOT_DEVICE_SN_LEN + 1] = "YOUR_DEVICE_SN";
/* 设备密钥 */
static char sg_device_secret[IOT_DEVICE_SECRET_LEN + 1] = "DEVICE_SECRET";
#endif


void *HAL_MutexCreate(void) {
    pthread_mutex_t *mutex = (pthread_mutex_t *) HAL_Malloc(sizeof(pthread_mutex_t));
    if (NULL == mutex) {
        return NULL;
    }

    if (0 != pthread_mutex_init(mutex, NULL)) {
        HAL_Printf("%s: create mutex failed\n", __FUNCTION__);
        HAL_Free(mutex);
        return NULL;
    }

    return mutex;
}

void HAL_MutexDestroy(_IN_ void *mutex) {
    if (0 != pthread_mutex_destroy((pthread_mutex_t *) mutex)) {
        HAL_Printf("%s: destroy mutex failed\n", __FUNCTION__);
    }

    HAL_Free(mutex);
}

void HAL_MutexLock(_IN_ void *mutex) {
    if (0 != pthread_mutex_lock((pthread_mutex_t *) mutex)) {
        HAL_Printf("%s: lock mutex failed\n", __FUNCTION__);
    }
}

IoT_Error_t HAL_MutexTryLock(_IN_ void *mutex) {
    int rc = pthread_mutex_trylock((pthread_mutex_t *) mutex);
    if (0 != rc) {
        HAL_Printf("%s: try to lock mutex failed\n", __FUNCTION__);
        return FAILURE_RET;
    }

    return SUCCESS_RET;
}


void HAL_MutexUnlock(_IN_ void *mutex) {
    if (0 != pthread_mutex_unlock((pthread_mutex_t *) mutex)) {
        HAL_Printf("%s: unlock mutex failed\n", __FUNCTION__);
    }
}

void *HAL_Malloc(_IN_ uint32_t size) {
    return malloc(size);
}

void HAL_Free(_IN_ void *ptr) {
    free(ptr);
}

void HAL_Printf(_IN_ const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    fflush(stdout);
}

int HAL_Snprintf(_IN_ char *str, _IN_ const int len, _IN_ const char *fmt, ...) {
    va_list args;
    int rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}

int HAL_Vsnprintf(_OU_ char *str, _IN_ const int len, _IN_ const char *format, _IN_ va_list ap) {
    return vsnprintf(str, len, format, ap);
}

uint64_t HAL_UptimeMs(void) {

    uint64_t time_ms;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    time_ms = ((uint64_t) ts.tv_sec * (uint64_t) 1000) + (ts.tv_nsec / 1000 / 1000);

    return time_ms;
}

void HAL_SleepMs(_IN_ uint32_t ms) {
    usleep(1000 * ms);
}

void *HAL_FileOpen(char *file_path){
    FILE *fp;
    if((fp = fopen(file_path, "wb+")) == NULL)
    {
        return NULL;
    }
    return (void *)fp;
    
}

int HAL_FileWrite(void *dest, uint32_t offset, void *src, uint32_t size){
    if(1 == fwrite(src, size, 1, (FILE *)dest))
        return SUCCESS_RET;
    else
        return FAILURE_RET;
}

void HAL_FileClose(void *fp){
    fclose((FILE *)fp);
}

IoT_Error_t HAL_GetProductSN(_OU_ char productSN[IOT_PRODUCT_SN_LEN + 1]) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(sg_product_sn);
    memset(productSN, 0x0, IOT_PRODUCT_SN_LEN + 1);
    strncpy(productSN, sg_product_sn, len);
    return SUCCESS_RET;
#else
    HAL_Printf("HAL_GetProductSN is not implement");
    return FAILURE_RET;
#endif
}

IoT_Error_t HAL_GetProductSecret(_OU_ char productSecret[IOT_PRODUCT_SECRET_LEN + 1]) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(sg_product_secret);
    memset(productSecret, 0x0, IOT_PRODUCT_SECRET_LEN + 1);
    strncpy(productSecret, sg_product_secret, len);
    return SUCCESS_RET;
#else
    Log_e("HAL_GetProductSecret is not implement");
    return FAILURE_RET;
#endif
}

IoT_Error_t HAL_GetDeviceSN(_OU_ char deviceSN[IOT_DEVICE_SN_LEN + 1]) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(sg_device_sn);
    memset(deviceSN, 0x0, IOT_DEVICE_SN_LEN + 1);
    strncpy(deviceSN, sg_device_sn, len);
    return SUCCESS_RET;
#else
    HAL_Printf("HAL_GetDeviceSN is not implement");
    return FAILURE_RET;
#endif
}

IoT_Error_t HAL_GetDeviceSecret(_OU_ char deviceSecret[IOT_DEVICE_SECRET_LEN + 1]) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(sg_device_secret);
    memset(deviceSecret, 0x0, IOT_DEVICE_SECRET_LEN + 1);
    strncpy(deviceSecret, sg_device_secret, len);
    return SUCCESS_RET;
#else
    HAL_Printf("HAL_GetDeviceSecret is not implement");
    return FAILURE_RET;
#endif
}

IoT_Error_t HAL_SetProductSN(_IN_ const char *pProductSN) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(pProductSN);
    if (len > IOT_PRODUCT_SN_LEN) {
        return FAILURE_RET;
    }

    memset(sg_product_sn, 0x0, IOT_PRODUCT_SN_LEN + 1);
    strncpy(sg_product_sn, pProductSN, len);

    return SUCCESS_RET;
#else
    HAL_Printf("HAL_SetProductSN is not implement");
    return FAILURE_RET;
#endif
}

IoT_Error_t HAL_SetProductSecret(_IN_ const char *pProductSecret) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(pProductSecret);
    if (len > IOT_PRODUCT_SECRET_LEN) {
        return FAILURE_RET;
    }

    memset(sg_product_secret, 0x0, IOT_PRODUCT_SECRET_LEN + 1);
    strncpy(sg_product_secret, pProductSecret, len);

    return SUCCESS_RET;
#else
    HAL_Printf("HAL_SetProductSecret is not implement");
    return FAILURE_RET;
#endif

}

IoT_Error_t HAL_SetDeviceSN(_IN_ const char *pDeviceSN) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(pDeviceSN);
    if (len > IOT_DEVICE_SN_LEN) {
        return FAILURE_RET;
    }

    memset(sg_device_sn, 0x0, IOT_DEVICE_SN_LEN + 1);
    strncpy(sg_device_sn, pDeviceSN, len);

    return SUCCESS_RET;
#else
    HAL_Printf("HAL_SetDeviceSN is not implement");
    return FAILURE_RET;
#endif
}

IoT_Error_t HAL_SetDeviceSecret(_IN_ const char *pDeviceSecret) {
#ifdef DEBUG_DEV_INFO_USED
    int len = strlen(pDeviceSecret);
    if (len > IOT_DEVICE_SECRET_LEN) {
        return FAILURE_RET;
    }

    memset(sg_device_secret, 0x0, IOT_DEVICE_SECRET_LEN + 1);
    strncpy(sg_device_secret, pDeviceSecret, len);

    return SUCCESS_RET;
#else
    HAL_Printf("HAL_SetDeviceSecret is not implement");
    return FAILURE_RET;
#endif
}

