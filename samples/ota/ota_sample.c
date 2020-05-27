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
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "uiot_export.h"
#include "uiot_import.h"
#include "uiot_export_ota.h"

#define UIOT_MY_PRODUCT_SN            "PRODUCT_SN"

#define UIOT_MY_DEVICE_SN             "DEVICE_SN"

#define UIOT_MY_DEVICE_SECRET         "DEVICE_SECRET"

#define OTA_BUF_LEN (1024)

#define CURRENT_VER                   "1.0.0"

static void event_handler(void *pClient, void *handle_context, MQTTEventMsg *msg)
{
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
            LOG_INFO("subscribe success.\n");
            break;

        case MQTT_EVENT_SUBSCRIBE_TIMEOUT:
            LOG_INFO("subscribe wait ack timeout.\n");
            break;

        case MQTT_EVENT_SUBSCRIBE_NACK:
            LOG_INFO("subscribe nack.\n");
            break;

        case MQTT_EVENT_PUBLISH_SUCCESS:
            LOG_INFO("publish success.\n");
            break;

        case MQTT_EVENT_PUBLISH_TIMEOUT:
            LOG_INFO("publish timeout.\n");
            break;

        case MQTT_EVENT_PUBLISH_NACK:
            LOG_INFO("publish nack.\n");
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

int IOT_OTA_FetchCallback_func(void *handle, IOT_OTA_UpstreamMsgType state)
{
    OTA_Struct_t * h_ota = (OTA_Struct_t *) handle;
    printf("file:%s, state:%d\r\n",h_ota->download_file_name, state);
    return SUCCESS_RET;
}

int main(int argc, char **argv)
{
    int rc;

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

    void *h_ota = IOT_OTA_Init(UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN, client, IOT_OTA_FetchCallback_func);
    if (NULL == h_ota) {
        IOT_MQTT_Destroy(&client);
        LOG_ERROR("init OTA failed");
        return FAILURE_RET;
    }

    /* Must report current version first */
    if (IOT_OTA_ReportVersion(h_ota, CURRENT_VER) < 0) {
        LOG_ERROR("report OTA version failed");
        return FAILURE_RET;
    }

	/* request new version, fill with current version */
    if (IOT_OTA_RequestFirmware(h_ota, CURRENT_VER) < 0) {
        LOG_ERROR("Request firmware failed");
        return FAILURE_RET;
    }
    
    while(1)
    {
        IOT_MQTT_Yield(client, 5000);
    }
}

