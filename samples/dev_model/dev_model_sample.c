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
#include "uiot_export_dm.h"

//用实际设备四元组替换
#define UIOT_MY_PRODUCT_SN            "PRODUCT_SN"

#define UIOT_MY_DEVICE_SN             "DEVICE_SN"

#define UIOT_MY_DEVICE_SECRET         "DEVICE_SECRET"

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


int event_post_cb(const char *request_id, const int ret_code){
    LOG_INFO("event_post_cb; request_id: %s; ret_code: %d", request_id, ret_code);
    return SUCCESS_RET;
}

int property_post_cb(const char *request_id, const int ret_code){
    LOG_INFO("property_post_cb; request_id: %s; ret_code: %d", request_id, ret_code);
    return SUCCESS_RET;
}

int g_set_temp_correct = 0;
int command_cb(const char *request_id, const char *identifier, const char *input, char *output){
    LOG_INFO("command_cb; request_id: %s; identifier: %s; input: %s", request_id, identifier, input);
    int temp = 0;
    if((strlen(identifier) == strlen("set_temp_correction")) && (0 == strncmp(identifier, "set_temp_correction", strlen("set_temp_correction"))))
    {
        char *set_temp_correction_temp_correction = NULL;
        set_temp_correction_temp_correction = LITE_json_value_of((char *)"temp_correction", (char *)input);
        temp = atoi(set_temp_correction_temp_correction);
        HAL_Free(set_temp_correction_temp_correction);
    }
    
    if((temp <= 5)
        && (temp >= -5))
    {
        g_set_temp_correct = temp;
        HAL_Snprintf(output, 100, "\"effect_temp_correction\":%d, \"correction_result\":%d", g_set_temp_correct, 0);
    }
    else
    {
        HAL_Snprintf(output, 100, "\"effect_temp_correction\":%d, \"correction_result\":%d", g_set_temp_correct, 1);
    }
    return SUCCESS_RET;
}

int property_set_cb(const char *request_id, const char *property){
    LOG_INFO("property_set_cb; request_id: %s; property: %s", request_id, property);
    return SUCCESS_RET;
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
    int i = 0;

    MQTTInitParams init_params = DEFAULT_MQTT_INIT_PARAMS;
    rc = _setup_connect_init_params(&init_params);
    if (rc != SUCCESS_RET) {
        return rc;
    }

    void *client = IOT_MQTT_Construct(&init_params);
    if (client != NULL) {
        LOG_INFO("Cloud Device Construct Success");
    } else {
        LOG_ERROR("Cloud Device Construct Failed");
        return FAILURE_RET;
    }
    IOT_MQTT_Yield(client, 50);

    void *h_dm = IOT_DM_Init(UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN, client);
    if (NULL == h_dm) {        
        IOT_MQTT_Destroy(&client);
        LOG_ERROR("initialize device model failed");
        return FAILURE_RET;
    }
    IOT_DM_Yield(h_dm, 50);

    IOT_DM_RegisterCallback(EVENT_POST, h_dm, event_post_cb);
    IOT_DM_RegisterCallback(COMMAND , h_dm, command_cb);
    IOT_DM_RegisterCallback(PROPERTY_POST , h_dm, property_post_cb);
    IOT_DM_RegisterCallback(PROPERTY_SET , h_dm, property_set_cb);
    IOT_DM_Yield(h_dm, 200);

    char *property_payload = (char *)HAL_Malloc(100);
    char *event_payload = (char *)HAL_Malloc(100);
    for (i = 0; i < 20; i++) {
        HAL_Snprintf(property_payload, 100, "\"humidity\": {\"Value\": %d}, \"temperature\": {\"Value\": %d}", i*5 + g_set_temp_correct, i*3);
        IOT_DM_Property_Report(h_dm, PROPERTY_POST, i * 2, property_payload);
        if(i * 5 + g_set_temp_correct >= 80)
        {
            HAL_Snprintf(event_payload, 100, "\"temperature\": %d", i*5 + g_set_temp_correct);
            IOT_DM_TriggerEvent(h_dm, i * 2 + 1, "high_temp", event_payload);
        }
        IOT_DM_Yield(h_dm, 200);
        HAL_SleepMs(2000);
    }

    HAL_Free(property_payload);
    HAL_Free(event_payload);
    //等待属性设置及命令下发
    IOT_DM_Yield(h_dm, 60000);

    IOT_DM_Destroy(h_dm);

    IOT_MQTT_Destroy(&client);

    return 0;
}
