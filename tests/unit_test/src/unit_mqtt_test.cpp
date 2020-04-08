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
#include <gtest/gtest.h>
#include <limits.h>
#include <iostream>

#include "uiot_export.h"
#include "uiot_import.h"

#define UIOT_MY_PRODUCT_SN            "PRODUCT_SN"

#define UIOT_MY_DEVICE_SN             "DEVICE_SN"

#define UIOT_MY_DEVICE_SECRET         "DEVICE_SECRET"

#define MAX_SIZE_OF_TOPIC_CONTENT 100

class MqttTests : public testing::Test
{
protected:
    virtual void SetUp()
    {
        std::cout << "MqttTests Test Begin \n";
    }
    virtual void TearDown()
    {
        std::cout << "MqttTests Test End \n";
    } 
    
};

void event_handler(void *pClient, void *handle_context, MQTTEventMsg *msg) {
    MQTTMessage* mqtt_message = (MQTTMessage*)msg->msg;
    uintptr_t packet_id = (uintptr_t)msg->msg;

    switch(msg->event_type) {
        case MQTT_EVENT_UNDEF:
            HAL_Printf("undefined event occur.");
            break;

        case MQTT_EVENT_DISCONNECT:
            HAL_Printf("MQTT disconnect.");
            break;

        case MQTT_EVENT_RECONNECT:
            HAL_Printf("MQTT reconnect.");
            break;

        case MQTT_EVENT_PUBLISH_RECEIVED:
            HAL_Printf("topic message arrived but without any related handle: topic=%.*s, topic_msg=%.*s",
                       mqtt_message->topic_len,
                       mqtt_message->topic,
                       mqtt_message->payload_len,
                       mqtt_message->payload);
            break;
        case MQTT_EVENT_SUBSCRIBE_SUCCESS:
            HAL_Printf("subscribe success, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_SUBSCRIBE_TIMEOUT:
            HAL_Printf("subscribe wait ack timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_SUBSCRIBE_NACK:
            HAL_Printf("subscribe nack, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBE_SUCCESS:
            HAL_Printf("unsubscribe success, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBE_TIMEOUT:
            HAL_Printf("unsubscribe timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBE_NACK:
            HAL_Printf("unsubscribe nack, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_SUCCESS:
            HAL_Printf("publish success, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_TIMEOUT:
            HAL_Printf("publish timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_NACK:
            HAL_Printf("publish nack, packet-id=%u", (unsigned int)packet_id);
            break;
        default:
            HAL_Printf("Should NOT arrive here.");
            break;
    }
}

static void on_message_callback(void *pClient, MQTTMessage *message, void *userData) {
    if (message == NULL) {
        return;
    }

    HAL_Printf("Receive Message With topicName:%.*s, payload:%.*s",
          (int) message->topic_len, message->topic, (int) message->payload_len, (char *) message->payload);
}

static int _publish_msg(void *client)
{
    char topicName[128] = {0};
    int num = 18;
    HAL_Snprintf(topicName, 128, "/%s/%s/upload/event", UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN);

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
    //说明:此处订阅的topic是一个可订阅可发布的，需要在控制台上新建
    HAL_Snprintf(topic_name, 128, "/%s/%s/upload/event", UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN);

    SubscribeParams sub_params = DEFAULT_SUB_PARAMS;
    sub_params.on_message_handler = on_message_callback;
    return IOT_MQTT_Subscribe(client, topic_name, &sub_params);
}

static int _register_unsubscribe_topics(void *client)
{
    static char topic_name[128] = {0};
    HAL_Snprintf(topic_name, 128, "/%s/%s/upload/event", UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN);

    return IOT_MQTT_Unsubscribe(client, topic_name);
}

static int _setup_connect_init_params(MQTTInitParams* initParams)
{
    int ret = 0;
    initParams->device_sn = (char *)UIOT_MY_DEVICE_SN;
    initParams->product_sn = (char *)UIOT_MY_PRODUCT_SN;
    initParams->device_secret = (char *)UIOT_MY_DEVICE_SECRET;

    initParams->command_timeout = UIOT_MQTT_COMMAND_TIMEOUT;
    initParams->keep_alive_interval = UIOT_MQTT_KEEP_ALIVE_INTERNAL;
    initParams->auto_connect_enable = 1;
    initParams->event_handler.h_fp = event_handler;
    initParams->event_handler.context = NULL;

    ret = HAL_SetProductSN(initParams->product_sn);  
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("set ProductSN fail:%d\n", ret);
        return ret;
    }
    ret = HAL_SetDeviceSN(initParams->device_sn);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("set DeviceSN fail:%d\n", ret);
        return ret;
    }
    ret = HAL_SetDeviceSecret(initParams->device_secret);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("set DeviceSecret fail:%d\n", ret);
        return ret;
    }

    return ret;
}


TEST_F(MqttTests, IOT_MQTT_test) {
    int ret = 0;
    MQTTInitParams init_params = DEFAULT_MQTT_INIT_PARAMS;
    ret = _setup_connect_init_params(&init_params);
    ASSERT_TRUE(ret == SUCCESS_RET);
    
    void *client = IOT_MQTT_Construct(&init_params);
    ASSERT_TRUE(client != NULL);
    
    _register_subscribe_topics(client);
    IOT_MQTT_Yield(client, 200);
        
    _publish_msg(client);
    IOT_MQTT_Yield(client, 200);

    _register_unsubscribe_topics(client);
    IOT_MQTT_Yield(client, 200);

    ASSERT_TRUE(true == IOT_MQTT_IsConnected(client));

    _register_subscribe_topics(client);
    IOT_MQTT_Yield(client, 200);
        
    _publish_msg(client);
    IOT_MQTT_Yield(client, 200);

    _register_unsubscribe_topics(client);
    IOT_MQTT_Yield(client, 200);

    ASSERT_TRUE(true == IOT_MQTT_IsConnected(client));
    
    ret = IOT_MQTT_Destroy(&client);
    ASSERT_TRUE(SUCCESS_RET == ret);
}

