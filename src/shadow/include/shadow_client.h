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

#ifndef IOT_SHADOW_CLIENT_H_
#define IOT_SHADOW_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "mqtt_client.h"
#include "shadow_client_json.h"
#include "uiot_export_shadow.h"

/* 在任意给定时间内, 处于appending状态的请求最大个数 */
#define MAX_APPENDING_REQUEST_AT_ANY_GIVEN_TIME                     (10)

/* 一个method的最大长度 */
#define MAX_SIZE_OF_METHOD                                          (10)

/* 一个仅包含method字段的JSON文档的最大长度 */
#define MAX_SIZE_OF_JSON_WITH_METHOD                                (MAX_SIZE_OF_METHOD + 20)

/* 接收云端返回的JSON文档的buffer大小 */
#define CLOUD_IOT_JSON_RX_BUF_LEN                                   (UIOT_MQTT_RX_BUF_LEN + 1)

/* 最大等待时间 */
#define MAX_WAIT_TIME_SEC   1
#define MAX_WAIT_TIME_MS    1000


/**
 * @brief 该结构体用于保存已登记的设备属性及设备属性处理的回调方法
 */
typedef struct {

    void *property;					 // 设备属性

    OnPropRegCallback callback;      // 回调处理函数

} PropertyHandler;

typedef struct _ShadowInnerData {
    uint32_t version;                   //本地维护的影子文档的版本号
    List *request_list;                 //影子文档的修改请求
    List *property_list;                //本地维护的影子文档的属性值,期望值和回调处理函数
} ShadowInnerData;

typedef struct _Shadow {
    void *mqtt;
    const char *product_sn;
    const char *device_sn;
    void *request_mutex;
    void *property_mutex;
    ShadowInnerData inner_data;
} UIoT_Shadow;

/**
 * @brief 设备影子初始化
 *
 * @param pShadow       shadow client
 * @return         		返回SUCCESS, 表示成功
 */
int uiot_shadow_init(UIoT_Shadow *pShadow);

/**
 * @brief 设备影子重置,主要是将设备影子中的队列归零
 *
 * @param pClient       shadow client
 * @return         		返回SUCCESS, 表示成功
 */
void uiot_shadow_reset(void *pClient);

/**
 * @brief 处理请求队列中已经超时的请求
 * 
 * @param pShadow   shadow client
 */
void _handle_expired_request(UIoT_Shadow *pShadow);

/**
 * @brief 所有的云端设备文档操作请求, 通过该方法进行中转分发
 *
 * @param pShadow       shadow client
 * @param pParams  		请求参数
 * @param pJsonDoc 		请求文档
 * @param sizeOfBuffer 	文档缓冲区大小
 * @return         		返回SUCCESS, 表示成功
 */
int uiot_shadow_make_request(UIoT_Shadow *pShadow,char *pJsonDoc, size_t sizeOfBuffer, RequestParams *pParams);

/**
 * @brief 订阅设备影子topic
 *
 * @param pShadow                   shadow client
 * @param topicFilter               topic的名称
 * @param on_message_handler 		topic的消息回调函数
 * @return         		返回SUCCESS, 表示成功
 */
int uiot_shadow_subscribe_topic(UIoT_Shadow *pShadow, char *topicFilter, OnMessageHandler on_message_handler);

/**
 * @brief 初始化一个修改设备影子的请求
 * @param handle        ShadowClient对象
 * @param method        修改类型
 * @param callback      请求回调函数
 * @param timeout_sec   超时时间
 * @return              返回NULL,表示初始化失败
 */
void* uiot_shadow_request_init(Method method, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context);

/**
 * @brief 从服务端获取设备影子文档
 *
 * @param handle        shadow handle
 * @param message  	    返回的消息
 * @param pUserdata     跟随回调函数返回
 */
void topic_request_result_handler(void *pClient, MQTTMessage *message, void *pUserdata);

/**
 * @brief 从服务端获取设备影子文档
 *
 * @param handle        shadow handle
 * @param message  	    返回的消息
 * @param pUserdata     跟随回调函数返回
 */
void topic_sync_handler(void *pClient, MQTTMessage *message, void *pUserdata);


#ifdef __cplusplus
}
#endif

#endif /* IOT_SHADOW_CLIENT_H_ */
