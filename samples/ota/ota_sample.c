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
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "uiot_export.h"
#include "uiot_import.h"
#include "uiot_export_ota.h"

#define UIOT_MY_PRODUCT_SN            "PRODUCT_SN"

#define UIOT_MY_DEVICE_SN             "DEVICE_SN"

#define UIOT_MY_DEVICE_SECRET         "DEVICE_SECRET"

#define OTA_BUF_LEN (5000)

static void event_handler(void *pClient, void *handle_context, MQTTEventMsg *msg)
{
    uintptr_t packet_id = 0;
    packet_id = (uintptr_t)msg->msg;

	switch(msg->event_type) {
		case MQTT_EVENT_UNDEF:
			LOG_INFO("undefined event occur.\n");
			break;

		case MQTT_EVENT_DISCONNECT:
			LOG_INFO("MQTT disconnect.\n");
			break;

		case MQTT_EVENT_RECONNECT:
			LOG_INFO("MQTT reconnect.\n");
			break;

		case MQTT_EVENT_SUBSCRIBE_SUCCESS:
			LOG_INFO("subscribe success, packet-id=%u.\n", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_SUBSCRIBE_TIMEOUT:
			LOG_INFO("subscribe wait ack timeout, packet-id=%u.\n", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_SUBSCRIBE_NACK:
			LOG_INFO("subscribe nack, packet-id=%u.\n", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_PUBLISH_SUCCESS:
			LOG_INFO("publish success, packet-id=%u.\n", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_PUBLISH_TIMEOUT:
			LOG_INFO("publish timeout, packet-id=%u.\n", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_PUBLISH_NACK:
			LOG_INFO("publish nack, packet-id=%u.\n", (unsigned int)packet_id);
			break;
		default:
			LOG_INFO("Should NOT arrive here.\n");
			break;
	}
}


static int _setup_connect_init_params(MQTTInitParams* initParams)
{
	initParams->device_sn = UIOT_MY_DEVICE_SN;
	initParams->product_sn = UIOT_MY_PRODUCT_SN;
	initParams->device_secret = UIOT_MY_DEVICE_SECRET;
	initParams->command_timeout = UIOT_MQTT_COMMAND_TIMEOUT;    
	initParams->keep_alive_interval = UIOT_MQTT_KEEP_ALIVE_INTERNAL;
	initParams->auto_connect_enable = 1;
    initParams->event_handler.h_fp = event_handler;

    return SUCCESS_RET;
}

int main(int argc, char **argv)
{
    int rc;
    int ota_over = 0;
    bool upgrade_fetch_success = true;
    // 用于存放云端下发的固件版本
    char msg_version[33];
    FILE *fp;
    char buf_ota[OTA_BUF_LEN];

    MQTTInitParams init_params = DEFAULT_MQTT_INIT_PARAMS;
    rc = _setup_connect_init_params(&init_params);
	if (rc != SUCCESS_RET) {
		return rc;
	}

    void *client = IOT_MQTT_Construct(&init_params);
    if (client != NULL) {
        LOG_INFO("MQTT Construct Success");
    } else {
        LOG_ERROR("MQTT Construct Failed");
        return FAILURE_RET;
    }

    void *h_ota = IOT_OTA_Init(UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN, client);
    if (NULL == h_ota) {
        LOG_ERROR("init OTA failed");
        return FAILURE_RET;
    }

    /* Must report version first */
    if (IOT_OTA_ReportVersion(h_ota, "1.0.0") < 0) {
        LOG_ERROR("report OTA version failed");
        return FAILURE_RET;
    }

    if (IOT_OTA_RequestFirmware(h_ota, "1.0.0") < 0) {
        LOG_ERROR("Request firmware failed");
        return FAILURE_RET;
    }

    if (NULL == (fp = fopen("ota.bin", "wb+"))) {
        LOG_ERROR("open file failed");
        return FAILURE_RET;
    }

    do {
        uint32_t firmware_valid;
        LOG_INFO("wait for ota upgrade command...");

        IOT_MQTT_Yield(client, 100);

        if (IOT_OTA_IsFetching(h_ota)) {
            char version[33], md5sum[33];
            uint32_t size_downloaded, size_file;
            do {
                int len = IOT_OTA_FetchYield(h_ota, buf_ota, OTA_BUF_LEN, 1);
                if (len > 0) {
                    if (1 != fwrite(buf_ota, len, 1, fp)) {
                        LOG_ERROR("write data to file failed");
                        upgrade_fetch_success = false;
                        break;
                    }
                } else if (len < 0) {
                    LOG_ERROR("download fail rc=%d", len);
                    upgrade_fetch_success = false;
                    break;
                }

                /* get OTA information */
                IOT_OTA_Ioctl(h_ota, OTA_IOCTL_FETCHED_SIZE, &size_downloaded, 4);
                IOT_OTA_Ioctl(h_ota, OTA_IOCTL_FILE_SIZE, &size_file, 4);
                IOT_OTA_Ioctl(h_ota, OTA_IOCTL_MD5SUM, md5sum, 33);
                IOT_OTA_Ioctl(h_ota, OTA_IOCTL_VERSION, version, 33);
                IOT_OTA_Ioctl(h_ota, OTA_IOCTL_VERSION, msg_version, 33);


                IOT_MQTT_Yield(client, 100);
            } while (!IOT_OTA_IsFetchFinish(h_ota));

            /* Must check MD5 match or not */
            if (upgrade_fetch_success) {
                IOT_OTA_Ioctl(h_ota, OTA_IOCTL_CHECK_FIRMWARE, &firmware_valid, 4);
                if (0 == firmware_valid) {
                    LOG_ERROR("The firmware is invalid");
                    upgrade_fetch_success = false;
                } else {
                    LOG_INFO("The firmware is valid");
                    upgrade_fetch_success = true;
                }
            }
            ota_over = 1;
        }

        HAL_SleepMs(2000);
    } while(!ota_over);

    if (upgrade_fetch_success)
    {
        HAL_SleepMs(1000);
        IOT_OTA_ReportSuccess(h_ota, msg_version);
    }

    fclose(fp);

    IOT_OTA_Destroy(h_ota);

    IOT_MQTT_Destroy(&client);

    return 0;
}
