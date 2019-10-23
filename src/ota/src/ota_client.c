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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uiot_defs.h"
#include "uiot_export_ota.h"
#include "uiot_internal.h"

#include "ota_config.h"
#include "ota_internal.h"

#include "utils_timer.h"


typedef struct  {
    uint32_t 				id;                		/* message id */
    IOT_OTA_State 		    state;   				/* OTA state */
    uint32_t 				size_last_fetched; 		/* size of last downloaded */
    uint32_t 				size_fetched;      		/* size of already downloaded */
    uint32_t 				size_file;         		/* size of file */

    char 					*url;                 	/* point to URL */
    char 					*version;              	/* point to version */
    char 					*md5sum;            	/* MD5 string */

    void 					*md5;                  	/* MD5 handle */
    void 					*ch_signal;           	/* channel handle of signal exchanged with OTA server */
    void 					*ch_fetch;             	/* channel handle of download */

    int 					err;                    /* last error code */

    Timer                   report_timer;
} OTA_Struct_t;


static void _ota_callback(void *pContext, const char *msg, uint32_t msg_len) {
    char *msg_method = NULL;
    char *msg_str = NULL;

    OTA_Struct_t *h_ota = (OTA_Struct_t *) pContext;

    if (h_ota == NULL || msg == NULL) {
        LOG_ERROR("pointer is NULL");
        return;
    }

    if (h_ota->state == OTA_STATE_FETCHING) {
        LOG_INFO("In OTA_STATE_FETCHING state");
        return;
    }

    if (NULL == (msg_str = HAL_Malloc(msg_len + 1))) {
        LOG_ERROR("HAL_Malloc failed!");
        return;
    }

    HAL_Snprintf(msg_str, msg_len + 1, "%s", msg);

    if (SUCCESS_RET != ota_lib_get_msg_type(msg_str, &msg_method)) {
        LOG_ERROR("Get message type failed!");
        goto do_exit;
    }

    if (strcmp(msg_method, UPDATE_FIRMWARE_METHOD) != 0) {
        LOG_ERROR("Message type error! type: %s", msg_method);
        goto do_exit;
    }

    if (SUCCESS_RET != ota_lib_get_params(msg_str, &h_ota->url, &h_ota->version,
                                &h_ota->md5sum, &h_ota->size_file)) {
        LOG_ERROR("Get firmware parameter failed");
        goto do_exit;
    }

    if (NULL == (h_ota->ch_fetch = ofc_init(h_ota->url))) {
        LOG_ERROR("Initialize fetch module failed");
        goto do_exit;
    }

    if (SUCCESS_RET != ofc_connect(h_ota->ch_fetch)) {
        LOG_ERROR("Connect fetch module failed");
        h_ota->state = OTA_STATE_DISCONNECTED;
        goto do_exit;
    }

    h_ota->state = OTA_STATE_FETCHING;

do_exit:
    HAL_Free(msg_str);
    HAL_Free(msg_method);
    return;
}


int IOT_OTA_ReportProgress(void *handle, int progress, IOT_OTA_ProgressState state)
{
    POINTER_VALID_CHECK(handle, ERR_OTA_INVALID_PARAM);

    int ret;
    char *msg_report;
    OTA_Struct_t *h_ota = (OTA_Struct_t *) handle;

    if (OTA_STATE_UNINITED == h_ota->state) {
        LOG_ERROR("handle is uninitialized");
        h_ota->err = ERR_OTA_INVALID_STATE;
        return ERR_OTA_INVALID_STATE;
    }

    if (progress < 0 || progress > 100) {
        LOG_ERROR("progress is a invalid parameter");
        h_ota->err = ERR_OTA_INVALID_PARAM;
        return ERR_OTA_INVALID_PARAM;
    }

    if (NULL == (msg_report = HAL_Malloc(OTA_UPSTREAM_MSG_BUF_LEN))) {
        LOG_ERROR("allocate memory for msg_report failed");
        h_ota->err = ERR_OTA_NO_MEMORY;
        return ERR_OTA_NO_MEMORY;
    }

    ret = ota_lib_gen_upstream_msg(msg_report, OTA_UPSTREAM_MSG_BUF_LEN, h_ota->version, progress, (IOT_OTA_UpstreamMsgType)state);
    if (SUCCESS_RET != ret) {
        LOG_ERROR("generate reported message failed");
        h_ota->err = ret;
        goto do_exit;
    }

    ret = osc_report_progress(h_ota->ch_signal, msg_report);
    if (ret < 0) {
        LOG_ERROR("Report progress failed");
        h_ota->err = ret;
        goto do_exit;
    }

do_exit:
    if (NULL != msg_report) {
        HAL_Free(msg_report);
    }
    return ret;
}


static int send_upstream_msg_with_version(void *handle, const char *version, IOT_OTA_UpstreamMsgType reportType)
{
    POINTER_VALID_CHECK(handle, ERR_OTA_INVALID_PARAM);
    POINTER_VALID_CHECK(version, ERR_OTA_INVALID_PARAM);

    int ret, len;
    char *msg_upstream;
    OTA_Struct_t *h_ota = (OTA_Struct_t *)handle;

    if (OTA_STATE_UNINITED == h_ota->state) {
        LOG_ERROR("handle is uninitialized");
        h_ota->err = ERR_OTA_INVALID_STATE;
        return ERR_OTA_INVALID_STATE;
    }

    len = strlen(version);
    if ((len < OTA_VERSION_STR_LEN_MIN) || (len > OTA_VERSION_STR_LEN_MAX)) {
        LOG_ERROR("version string is invalid: must be [1, 32] chars");
        h_ota->err = ERR_OTA_INVALID_PARAM;
        return ERR_OTA_INVALID_PARAM;
    }

    if (NULL == (msg_upstream = HAL_Malloc(OTA_UPSTREAM_MSG_BUF_LEN))) {
        LOG_ERROR("allocate for msg_informed failed");
        h_ota->err = ERR_OTA_NO_MEMORY;
        return ERR_OTA_NO_MEMORY;
    }

    ret = ota_lib_gen_upstream_msg(msg_upstream, OTA_UPSTREAM_MSG_BUF_LEN, version, 0, reportType);
    if (SUCCESS_RET != ret) {
        LOG_ERROR("generate upstream message failed");
        h_ota->err = ret;
        goto do_exit;
    }

    ret = osc_upstream_publish(h_ota->ch_signal, msg_upstream);
    if (ret < 0) {
        LOG_ERROR("Report result failed");
        h_ota->err = ret;
        goto do_exit;
    }

do_exit:
    if (NULL != msg_upstream) {
        HAL_Free(msg_upstream);
    }
    return ret;
}


void *IOT_OTA_Init(const char *product_sn, const char *device_sn, void *ch_signal)
{
    POINTER_VALID_CHECK(product_sn, NULL);
    POINTER_VALID_CHECK(device_sn, NULL);
    POINTER_VALID_CHECK(ch_signal, NULL);

    OTA_Struct_t *h_ota = NULL;

    if (NULL == (h_ota = HAL_Malloc(sizeof(OTA_Struct_t)))) {
        LOG_ERROR("allocate failed");
        return NULL;
    }
    memset(h_ota, 0, sizeof(OTA_Struct_t));
    h_ota->state = OTA_STATE_UNINITED;

    h_ota->ch_signal = osc_init(product_sn, device_sn, ch_signal, _ota_callback, h_ota);
    if (NULL == h_ota->ch_signal) {
        LOG_ERROR("initialize signal channel failed");
        goto do_exit;
    }

    IOT_MQTT_Yield(ch_signal, 200);

    h_ota->md5 = ota_lib_md5_init();
    if (NULL == h_ota->md5) {
        LOG_ERROR("initialize md5 failed");
        goto do_exit;
    }

    h_ota->state = OTA_STATE_INITED;
    return h_ota;

do_exit:
    if (NULL != h_ota->ch_signal) {
        osc_deinit(h_ota->ch_signal);
    }

    if (NULL != h_ota->md5) {
        ota_lib_md5_deinit(h_ota->md5);
    }

    if (NULL != h_ota) {
        HAL_Free(h_ota);
    }

    return NULL;
}


int IOT_OTA_Destroy(void *handle)
{
    POINTER_VALID_CHECK(handle, ERR_OTA_INVALID_PARAM);

    OTA_Struct_t *h_ota = (OTA_Struct_t*) handle;

    if (OTA_STATE_UNINITED == h_ota->state) {
        LOG_ERROR("handle is uninitialized");
        return FAILURE_RET;
    }

    osc_deinit(h_ota->ch_signal);
    ofc_deinit(h_ota->ch_fetch);
    ota_lib_md5_deinit(h_ota->md5);

    if (NULL != h_ota->url) {
        HAL_Free(h_ota->url);
    }

    if (NULL != h_ota->version) {
        HAL_Free(h_ota->version);
    }

    if (NULL != h_ota->md5sum) {
        HAL_Free(h_ota->md5sum);
    }

    HAL_Free(h_ota);
    return SUCCESS_RET;
}


int IOT_OTA_ReportVersion(void *handle, const char *version)
{
    return send_upstream_msg_with_version(handle, version, OTA_REPORT_VERSION);
}


int IOT_OTA_RequestFirmware(void *handle, const char *version)
{
    return send_upstream_msg_with_version(handle, version, OTA_REQUEST_FIRMWARE);
}

int IOT_OTA_ReportSuccess(void *handle, const char *version)
{
    return send_upstream_msg_with_version(handle, version, OTA_REPORT_SUCCESS);
}


int IOT_OTA_ReportFail(void *handle, IOT_OTA_ReportErrCode err_code)
{
    POINTER_VALID_CHECK(handle, ERR_OTA_INVALID_PARAM);

    int ret;
    char *msg_upstream;
    OTA_Struct_t *h_ota = (OTA_Struct_t *)handle;

    if (OTA_STATE_UNINITED == h_ota->state) {
        LOG_ERROR("handle is uninitialized");
        h_ota->err = ERR_OTA_INVALID_STATE;
        return ERR_OTA_INVALID_STATE;
    }

    if (NULL == (msg_upstream = HAL_Malloc(OTA_UPSTREAM_MSG_BUF_LEN))) {
        LOG_ERROR("allocate for msg_informed failed");
        h_ota->err = ERR_OTA_NO_MEMORY;
        return ERR_OTA_NO_MEMORY;
    }

    ret = ota_lib_gen_upstream_msg(msg_upstream, OTA_UPSTREAM_MSG_BUF_LEN, "", 0, (IOT_OTA_UpstreamMsgType)err_code);
    if (SUCCESS_RET != ret) {
        LOG_ERROR("generate upstream message failed");
        h_ota->err = ret;
        goto do_exit;
    }

    ret = osc_upstream_publish(h_ota->ch_signal, msg_upstream);
    if (ret < 0) {
        LOG_ERROR("Report result failed");
        h_ota->err = ret;
        goto do_exit;
    }

    do_exit:
    if (NULL != msg_upstream) {
        HAL_Free(msg_upstream);
    }
    return ret;
}


int IOT_OTA_IsFetching(void *handle)
{
    OTA_Struct_t *h_ota = (OTA_Struct_t *)handle;

    if (NULL == handle) {
        LOG_ERROR("handle is NULL");
        return 0;
    }

    if (OTA_STATE_UNINITED == h_ota->state) {
        LOG_ERROR("handle is uninitialized");
        h_ota->err = ERR_OTA_INVALID_STATE;
        return 0;
    }

    return (OTA_STATE_FETCHING == h_ota->state);
}


int IOT_OTA_IsFetchFinish(void *handle)
{
    OTA_Struct_t *h_ota = (OTA_Struct_t *) handle;

    if (NULL == handle) {
        LOG_ERROR("handle is NULL");
        return 0;
    }

    if (OTA_STATE_UNINITED == h_ota->state) {
        LOG_ERROR("handle is uninitialized");
        h_ota->err = ERR_OTA_INVALID_STATE;
        return 0;
    }

    return (OTA_STATE_FETCHED == h_ota->state);
}


int IOT_OTA_FetchYield(void *handle, char *buf, size_t buf_len, uint32_t timeout_s)
{
    int ret;
    OTA_Struct_t *h_ota = (OTA_Struct_t *) handle;

    POINTER_VALID_CHECK(handle, ERR_OTA_INVALID_PARAM);
    POINTER_VALID_CHECK(buf, ERR_OTA_INVALID_PARAM);
    NUMERIC_VALID_CHECK(buf_len, ERR_OTA_INVALID_PARAM);

    if (OTA_STATE_FETCHING != h_ota->state) {
        h_ota->err = ERR_OTA_INVALID_STATE;
        return ERR_OTA_INVALID_STATE;
    }

    ret = ofc_fetch(h_ota->ch_fetch, buf, buf_len, timeout_s);
    if (ret < 0) {
        LOG_ERROR("Fetch firmware failed");
        h_ota->state = OTA_STATE_FETCHED;
        h_ota->err = ret;
        if (NULL != h_ota->url) {
            HAL_Free(h_ota->url);
        }
        h_ota->url = NULL;

        if (ret == ERR_OTA_FETCH_AUTH_FAIL) { // 上报签名过期
            IOT_OTA_ReportFail(h_ota, OTA_ERRCODE_SIGNATURE_EXPIRED);
        } else if (ret == ERR_OTA_FILE_NOT_EXIST) { // 上报文件不存在
            IOT_OTA_ReportFail(h_ota, OTA_ERRCODE_FIRMWARE_NOT_EXIST);
        } else if (ret == ERR_OTA_FETCH_TIMEOUT) { // 上报下载超时
            IOT_OTA_ReportFail(h_ota, OTA_ERRCODE_DOWNLOAD_TIMEOUT);
        } else {
            h_ota->err = ERR_OTA_FETCH_FAILED;
        }

        return ret;
    } else if (0 == h_ota->size_fetched) {
        /* force report status in the first */
        IOT_OTA_ReportProgress(h_ota, 0, OTA_PROGRESS_DOWNLOADING);

        init_timer(&h_ota->report_timer);
        countdown(&h_ota->report_timer, OTA_REPORT_PROGRESS_INTERVAL);
    }

    ota_lib_md5_update(h_ota->md5, buf, ret);
    h_ota->size_last_fetched = ret;
    h_ota->size_fetched += ret;

    /* report percent every second. */
    uint32_t percent = (h_ota->size_fetched * 100) / h_ota->size_file;
    if (percent == 100) {
        IOT_OTA_ReportProgress(h_ota, percent, OTA_PROGRESS_DOWNLOADING);
    } else if (h_ota->size_last_fetched > 0 && has_expired(&h_ota->report_timer)) {
        IOT_OTA_ReportProgress(h_ota, percent, OTA_PROGRESS_DOWNLOADING);
        countdown(&h_ota->report_timer, OTA_REPORT_PROGRESS_INTERVAL);
    }

    if (h_ota->size_fetched >= h_ota->size_file) {
        h_ota->state = OTA_STATE_FETCHED;
        if (NULL != h_ota->url) {
            HAL_Free(h_ota->url);
        }
        h_ota->url = NULL;
    }

    return ret;
}


int IOT_OTA_Ioctl(void *handle, IOT_OTA_CmdType type, void *buf, size_t buf_len)
{
    OTA_Struct_t * h_ota = (OTA_Struct_t *) handle;

    POINTER_VALID_CHECK(handle, ERR_OTA_INVALID_PARAM);
    POINTER_VALID_CHECK(buf, ERR_OTA_INVALID_PARAM);
    NUMERIC_VALID_CHECK(buf_len, ERR_OTA_INVALID_PARAM);

    if (h_ota->state < OTA_STATE_FETCHING) {
        h_ota->err = ERR_OTA_INVALID_STATE;
        return ERR_OTA_INVALID_STATE;
    }

    switch (type) {
        case OTA_IOCTL_FETCHED_SIZE:
            if ((4 != buf_len) || (0 != ((unsigned long)buf & 0x3))) {
                LOG_ERROR("Invalid parameter");
                h_ota->err = ERR_OTA_INVALID_PARAM;
                return FAILURE_RET;
            } else {
                *((uint32_t *)buf) = h_ota->size_fetched;
                return SUCCESS_RET;
            }

        case OTA_IOCTL_FILE_SIZE:
            if ((4 != buf_len) || (0 != ((unsigned long)buf & 0x3))) {
                LOG_ERROR("Invalid parameter");
                h_ota->err = ERR_OTA_INVALID_PARAM;
                return FAILURE_RET;
            } else {
                *((uint32_t *)buf) = h_ota->size_file;
                return SUCCESS_RET;
            }

        case OTA_IOCTL_VERSION:
            strncpy(buf, h_ota->version, buf_len);
            ((char *)buf)[buf_len - 1] = '\0';
            break;

        case OTA_IOCTL_MD5SUM:
            strncpy(buf, h_ota->md5sum, buf_len);
            ((char *)buf)[buf_len - 1] = '\0';
            break;

        case OTA_IOCTL_CHECK_FIRMWARE:
            if ((4 != buf_len) || (0 != ((unsigned long)buf & 0x3))) {
                LOG_ERROR("Invalid parameter");
                h_ota->err = ERR_OTA_INVALID_PARAM;
                return FAILURE_RET;
            } else if (h_ota->state != OTA_STATE_FETCHED) {
                h_ota->err = ERR_OTA_INVALID_STATE;
                LOG_ERROR("Firmware can be checked in OTA_STATE_FETCHED state only");
                return FAILURE_RET;
            } else {
                char md5_str[33];
                ota_lib_md5_finalize(h_ota->md5, md5_str);
                LOG_DEBUG("origin=%s, now=%s", h_ota->md5sum, md5_str);
                if (0 == strcmp(h_ota->md5sum, md5_str)) {
                    *((uint32_t *)buf) = 1;
                } else {
                    *((uint32_t *)buf) = 0;
                    // 上报MD5不匹配
                    h_ota->err = ERR_OTA_MD5_MISMATCH;
                    IOT_OTA_ReportFail(h_ota, OTA_ERRCODE_MD5_MISMATCH);
                }
                return SUCCESS_RET;
            }
            
        default:
            LOG_ERROR("invalid cmd type");
            h_ota->err = ERR_OTA_INVALID_PARAM;
            return FAILURE_RET;
    }

    return SUCCESS_RET;
}


int IOT_OTA_GetLastError(void *handle)
{
    OTA_Struct_t * h_ota = (OTA_Struct_t *) handle;

    if (NULL == handle) {
        LOG_ERROR("handle is NULL");
        return  ERR_OTA_INVALID_PARAM;
    }

    return h_ota->err;
}
