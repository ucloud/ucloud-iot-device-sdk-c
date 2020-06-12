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

DM_Property_t property_humidity;
DM_Node_t node_property_humidity;

DM_Property_t property_temperature;
DM_Node_t node_property_temperature;

static void _init_data_template(){
    node_property_humidity.base_type = TYPE_FLOAT;
    node_property_humidity.key = "humidity";
    node_property_humidity.value.float32_value = 0.0;
    property_humidity.parse_type = TYPE_NODE;
    property_humidity.value.dm_node = &node_property_humidity;

    node_property_temperature.base_type = TYPE_FLOAT;
    node_property_temperature.key = "temperature";
    node_property_temperature.value.float32_value = 0.0;
    property_temperature.parse_type = TYPE_NODE;
    property_temperature.value.dm_node = &node_property_temperature;

}

DM_Property_t event_high_temp_temperature;
DM_Node_t node_event_high_temp_temperature;

static void _init_event_property_template(){
    node_event_high_temp_temperature.base_type = TYPE_FLOAT;
    node_event_high_temp_temperature.key = "temperature";
    node_event_high_temp_temperature.value.float32_value = 0.0;
    event_high_temp_temperature.parse_type = TYPE_NODE;
    event_high_temp_temperature.value.dm_node = &node_event_high_temp_temperature;

}
DM_Event_t event_high_temp_warning;
DM_Property_t high_temp[1];

static void _init_event_template(){
    _init_event_property_template();
    high_temp[0] = event_high_temp_temperature;
    event_high_temp_warning.event_identy = "high_temp";
    event_high_temp_warning.dm_property = high_temp;
    event_high_temp_warning.property_num = 1;

}

DM_Property_t cmd_output_temp_modify_modify_result;
DM_Node_t node_cmd_output_temp_modify_modify_result;

DM_Property_t cmd_output_temp_modify_effect_temp_modify;
DM_Node_t node_cmd_output_temp_modify_effect_temp_modify;

static void _init_command_output_template(){
    node_cmd_output_temp_modify_modify_result.base_type = TYPE_BOOL;
    node_cmd_output_temp_modify_modify_result.key = "modify_result";
    node_cmd_output_temp_modify_modify_result.value.bool_value = 0;
    cmd_output_temp_modify_modify_result.parse_type = TYPE_NODE;
    cmd_output_temp_modify_modify_result.value.dm_node = &node_cmd_output_temp_modify_modify_result;

    node_cmd_output_temp_modify_effect_temp_modify.base_type = TYPE_INT;
    node_cmd_output_temp_modify_effect_temp_modify.key = "effect_temp_modify";
    node_cmd_output_temp_modify_effect_temp_modify.value.int32_value = 0;
    cmd_output_temp_modify_effect_temp_modify.parse_type = TYPE_NODE;
    cmd_output_temp_modify_effect_temp_modify.value.dm_node = &node_cmd_output_temp_modify_effect_temp_modify;

}

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

int command_cb(const char *request_id, const char *identifier, const char *input, char *output){
    LOG_INFO("command_cb; request_id: %s; identifier: %s; input: %s", request_id, identifier, input);
    char *temp_modify = NULL;
    if (NULL == (temp_modify = LITE_json_value_of((char *) "temp_modify", (char *)input))) {
        LOG_ERROR("allocate for input failed\r\n");
        return FAILURE_RET;
    }
    
    node_cmd_output_temp_modify_effect_temp_modify.value.int32_value = atoi(temp_modify);
    node_cmd_output_temp_modify_modify_result.value.bool_value = 0;

    IOT_DM_GenCommandOutput(output, 2, &cmd_output_temp_modify_effect_temp_modify, &cmd_output_temp_modify_modify_result);
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

    _init_data_template();
    _init_event_template();
    _init_command_output_template();

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

    for (i = 0; i < 20; i++) {
        node_property_humidity.value.float32_value = (float)(node_property_humidity.value.float32_value + i);
        node_property_temperature.value.float32_value = (float)(node_property_temperature.value.float32_value + i \
                                                              + node_cmd_output_temp_modify_modify_result.value.bool_value);
        node_property_humidity.value.float32_value = node_property_humidity.value.float32_value > 100.0?100.0:node_property_humidity.value.float32_value;
        node_property_temperature.value.float32_value = node_property_temperature.value.float32_value > 100.0?100.0:node_property_temperature.value.float32_value;
        IOT_DM_Property_ReportEx(h_dm, PROPERTY_POST, i * 10 + 8, 2, &property_humidity, &property_temperature);     
        
        IOT_DM_Yield(h_dm, 200);

        //温度大于80度上报高温预警
        if(node_property_temperature.value.float32_value > 80.0)
        {
            node_event_high_temp_temperature.value.float32_value = node_property_temperature.value.float32_value;
            IOT_DM_TriggerEventEx(h_dm, i * 10 + 1, &event_high_temp_warning);
        
            IOT_DM_Yield(h_dm, 200);
        }
        HAL_SleepMs(200);
    }

    //等待属性设置及命令下发
    IOT_DM_Yield(h_dm, 60000);

    IOT_DM_Destroy(h_dm);

    IOT_MQTT_Destroy(&client);

    return 0;
}
