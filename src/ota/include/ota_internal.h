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

#ifndef C_SDK_OTA_INTERNAL_H_
#define C_SDK_OTA_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "uiot_export_ota.h"

// OTA Signal Channel
typedef void (*OnOTAMessageCallback)(void *pContext, const char *msg, uint32_t msgLen);

void *osc_init(const char *product_sn, const char *device_sn, void *channel, OnOTAMessageCallback callback,
               void *context);

int osc_deinit(void *handle);

int osc_report_progress(void *handle, const char *msg);

int osc_upstream_publish(void *handle, const char *msg);

// OTA Fetch Channel
void *ofc_init(const char *url);

int32_t ofc_connect(void *handle);

int32_t ofc_fetch(void *handle, uint32_t size_fetched, char *buf, uint32_t buf_len, size_t range_len, uint32_t timeout_s);

int ofc_deinit(void *handle);

// ota_lib
void *ota_lib_md5_init(void);

void ota_lib_md5_update(void *md5, const char *buf, size_t buf_len);

void ota_lib_md5_finalize(void *md5, char *output_str);

void ota_lib_md5_deinit(void *md5);

int ota_lib_get_msg_type(char *json, char **type);

int ota_lib_get_params(char *json, char **url, char **file_name, char **version, char **md5,
                       uint32_t *fileSize);

int ota_lib_gen_upstream_msg(char *buf, size_t bufLen, const char *version, int progress,
                             IOT_OTA_UpstreamMsgType reportType);

#ifdef __cplusplus
}
#endif

#endif //C_SDK_OTA_INTERNAL_H_
