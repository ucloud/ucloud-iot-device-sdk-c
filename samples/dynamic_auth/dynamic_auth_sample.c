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
#include <string.h>
#include <limits.h>

#include "uiot_export.h"
#include "uiot_import.h"
#include "lite-utils.h"
/* 产品序列号, 与云端同步设备状态时需要  */
#define UIOT_MY_PRODUCT_SN            "PRODUCT_SN"
/* 
    设备序列号, 与云端同步设备状态时需要, 静态认证后的设备不能进行动态认证，
    因此每次执行该用例需要新建一个设备。
*/
#define UIOT_MY_DEVICE_SN             "DEVICE_SN"

#define UIOT_MY_PRODUCT_SECRET        "PRODUCT_SECRET"

#define MAX_SIZE_OF_TOPIC_CONTENT 100
MQTTInitParams init_params = DEFAULT_MQTT_INIT_PARAMS;

void event_handler(void *pclient, void *handle_context, MQTTEventMsg *msg) {
    LOG_DEBUG("Enter event_handler!type:%d\n",msg->event_type);
}

static void on_message_callback(void *pClient, MQTTMessage *message, void *userData) {
    if (message == NULL) {
        return;
    }

    LOG_DEBUG("Receive Message With topicName:%.*s, payload:%.*s",
          (int) message->topic_len, message->topic, (int) message->payload_len, (char *) message->payload);
}

static int _publish_msg(void *client)
{
    char topicName[128] = {0};
    int num = 18;
    HAL_Snprintf(topicName, 128, "/%s/%s/upload", UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN);

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    
    char topic_content[MAX_SIZE_OF_TOPIC_CONTENT + 1] = {0};

    HAL_Snprintf(topic_content, MAX_SIZE_OF_TOPIC_CONTENT, "{\"test\": \"%d\"}", num);

    pub_params.payload = topic_content;
    pub_params.payload_len = strlen(topic_content);

    return IOT_MQTT_Publish(client, topicName, &pub_params);
}

static int _register_subscribe_topics(void *client)
{
    static char topic_name[128] = {0};
    HAL_Snprintf(topic_name, 128, "/%s/%s/set", UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN);

    SubscribeParams sub_params = DEFAULT_SUB_PARAMS;
    sub_params.on_message_handler = on_message_callback;
    return IOT_MQTT_Subscribe(client, topic_name, &sub_params);
}

/**
 * 设置MQTT connet初始化参数
 *
 * @param initParams MQTT connet初始化参数
 *
 * @return 0: 参数初始化成功  非0: 失败
 */
static int _setup_connect_init_params(MQTTInitParams* initParams)
{
    initParams->device_sn = (char *)UIOT_MY_DEVICE_SN;
    initParams->product_sn = (char *)UIOT_MY_PRODUCT_SN;
    initParams->product_secret = (char *)UIOT_MY_PRODUCT_SECRET;

    initParams->command_timeout = UIOT_MQTT_COMMAND_TIMEOUT;
    initParams->keep_alive_interval = UIOT_MQTT_KEEP_ALIVE_INTERNAL;

    initParams->auto_connect_enable = 1;
    initParams->event_handler.h_fp = event_handler;
    initParams->event_handler.context = NULL;

    return SUCCESS_RET;
}

int main()
{
    int ret = 0;
    char secret[IOT_DEVICE_SECRET_LEN+1];
    ret = _setup_connect_init_params(&init_params);
    if (ret != SUCCESS_RET) {
        return ret;
    }
    
    ret = IOT_MQTT_Dynamic_Register(&init_params);
    if (ret != SUCCESS_RET) {
        return ret;
    }
    
    ret = HAL_GetDeviceSecret(secret);
    if(ret != SUCCESS_RET)
    {   
        LOG_ERROR("get device secret fail\n");
        return ret;
    }
    LOG_DEBUG("Password:%s\n",init_params.device_secret);
    init_params.device_secret = secret;
    void *static_client = IOT_MQTT_Construct(&init_params);
    if(static_client == NULL)
    {   
        LOG_ERROR("static_client Construct fail\n");
        return FAILURE_RET;
    }
    _register_subscribe_topics(static_client);
    IOT_MQTT_Yield(static_client, 200);
    _publish_msg(static_client);
    IOT_MQTT_Yield(static_client, 200);
    ret = IOT_MQTT_Destroy(&static_client);
    return ret;
}

