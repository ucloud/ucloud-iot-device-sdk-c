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
#include <stdbool.h>
#include <string.h>
#include <signal.h>

#include "uiot_export.h"
#include "uiot_import.h"

#define UIOT_MY_PRODUCT_SN            "PRODUCT_SN"

#define UIOT_MY_DEVICE_SN             "DEVICE_SN"

#define UIOT_MY_DEVICE_SECRET         "DEVICE_SECRET"


#define MAX_SIZE_OF_TOPIC_CONTENT 100

static int sg_count = 0;
static int sg_sub_packet_id = -1;

void event_handler(void *pClient, void *handle_context, MQTTEventMsg *msg) {
	MQTTMessage* mqtt_message = (MQTTMessage*)msg->msg;
	uintptr_t packet_id = (uintptr_t)msg->msg;

	switch(msg->event_type) {
		case MQTT_EVENT_UNDEF:
			HAL_Printf("undefined event occur.\n");
			break;

		case MQTT_EVENT_DISCONNECT:
			HAL_Printf("MQTT disconnect.\n");
			break;

		case MQTT_EVENT_RECONNECT:
			HAL_Printf("MQTT reconnect.\n");
			break;

		case MQTT_EVENT_PUBLISH_RECEIVED:
			HAL_Printf("topic message arrived but without any related handle: topic=%.*s, topic_msg=%.*s\n",
					  mqtt_message->topic_len,
					  mqtt_message->topic,
					  mqtt_message->payload_len,
					  mqtt_message->payload);
			break;
		case MQTT_EVENT_SUBSCRIBE_SUCCESS:
			HAL_Printf("subscribe success, packet-id=%u\n", (unsigned int)packet_id);
			sg_sub_packet_id = packet_id;
			break;

		case MQTT_EVENT_SUBSCRIBE_TIMEOUT:
			HAL_Printf("subscribe wait ack timeout, packet-id=%u\n", (unsigned int)packet_id);
			sg_sub_packet_id = packet_id;
			break;

		case MQTT_EVENT_SUBSCRIBE_NACK:
			HAL_Printf("subscribe nack, packet-id=%u\n", (unsigned int)packet_id);
			sg_sub_packet_id = packet_id;
			break;

		case MQTT_EVENT_UNSUBSCRIBE_SUCCESS:
			HAL_Printf("unsubscribe success, packet-id=%u\n", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_UNSUBSCRIBE_TIMEOUT:
			HAL_Printf("unsubscribe timeout, packet-id=%u\n", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_UNSUBSCRIBE_NACK:
			HAL_Printf("unsubscribe nack, packet-id=%u\n", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_PUBLISH_SUCCESS:
			HAL_Printf("publish success, packet-id=%u\n", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_PUBLISH_TIMEOUT:
			HAL_Printf("publish timeout, packet-id=%u\n", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_PUBLISH_NACK:
			HAL_Printf("publish nack, packet-id=%u\n", (unsigned int)packet_id);
			break;
		default:
			HAL_Printf("Should NOT arrive here.\n");
			break;
	}
}

/**
 * MQTT消息接收处理函数
 *
 * @param topicName         topic主题
 * @param topicNameLen      topic长度
 * @param message           已订阅消息的结构
 * @param userData         消息负载
 */
static void on_message_callback(void *pClient, MQTTMessage *message, void *userData) {
	if (message == NULL) {
		return;
	}

	HAL_Printf("Receive Message With topicName:%.*s, payload:%.*s\n",
		  (int) message->topic_len, message->topic, (int) message->payload_len, (char *) message->payload);
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
	initParams->device_sn = UIOT_MY_DEVICE_SN;
	initParams->product_sn = UIOT_MY_PRODUCT_SN;
	initParams->device_secret = UIOT_MY_DEVICE_SECRET;

	initParams->command_timeout = UIOT_MQTT_COMMAND_TIMEOUT;
	initParams->keep_alive_interval = UIOT_MQTT_KEEP_ALIVE_INTERNAL;

	initParams->auto_connect_enable = 1;
	initParams->event_handler.h_fp = event_handler;
	initParams->event_handler.context = NULL;

    HAL_SetProductSN(initParams->product_sn);  
    HAL_SetDeviceSN(initParams->device_sn);
    HAL_SetDeviceSecret(initParams->device_secret);

    return SUCCESS_RET;
}

/**
 * 发送topic消息
 *
 */
static int _publish_msg(void *client)
{
    char topicName[128] = {0};
    HAL_Snprintf(topicName, 128, "/%s/%s/%s", UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN, "set");

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos = QOS1;

    char topic_content[MAX_SIZE_OF_TOPIC_CONTENT + 1] = {0};

	int size = HAL_Snprintf(topic_content, sizeof(topic_content), "{\"test\": \"%d\"}", sg_count++);
	if (size < 0 || size > sizeof(topic_content) - 1)
	{
		HAL_Printf("payload content length not enough! content size:%d  buf size:%d\n", size, (int)sizeof(topic_content));
		return -3;
	}

	pub_params.payload = topic_content;
	pub_params.payload_len = strlen(topic_content);

    return IOT_MQTT_Publish(client, topicName, &pub_params);
}

/**
 * 订阅关注topic和注册相应回调处理
 *
 */
static int _register_subscribe_topics(void *client)
{
    static char topic_name[128] = {0};
    int size = HAL_Snprintf(topic_name, sizeof(topic_name), "/%s/%s/%s", UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN, "set");
    if (size < 0 || size > sizeof(topic_name) - 1)
    {
        HAL_Printf("topic content length not enough! content size:%d  buf size:%d\n", size, (int)sizeof(topic_name));
        return FAILURE_RET;
    }
    SubscribeParams sub_params = DEFAULT_SUB_PARAMS;
    sub_params.on_message_handler = on_message_callback;
    return IOT_MQTT_Subscribe(client, topic_name, &sub_params);
}
int main(int argc, char **argv) {    
    int rc;

    // to avoid process crash when writing to a broken socket
    signal(SIGPIPE, SIG_IGN);
    
    //init connection
    MQTTInitParams init_params = DEFAULT_MQTT_INIT_PARAMS;
    rc = _setup_connect_init_params(&init_params);
	if (rc != SUCCESS_RET) {
		return rc;
	}
    void *client = IOT_MQTT_Construct(&init_params);
    if (client != NULL) {
        HAL_Printf("Cloud Device Construct Success");
    } else {
        HAL_Printf("Cloud Device Construct Failed");
        return FAILURE_RET;
    }

	//register subscribe topics here
    rc = _register_subscribe_topics(client);
    if (rc < 0) {
        HAL_Printf("Client Subscribe Topic Failed: %d", rc);
        return rc;
    }

	rc = IOT_MQTT_Yield(client, 200);

    do {
		// 等待订阅结果
		if (sg_sub_packet_id > 0) {
			rc = _publish_msg(client);
			if (rc < 0) {
				HAL_Printf("client publish topic failed :%d.", rc);
			}
		}

    	rc = IOT_MQTT_Yield(client, 200);

    } while (sg_sub_packet_id < 0);

    rc = IOT_MQTT_Destroy(&client);    
    return rc;
}
