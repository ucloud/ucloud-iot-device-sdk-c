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
#include <stdbool.h>
#include <string.h>
#include <signal.h>

//#include "unit_helper_functions.h"
#include "uiot_export_shadow.h"
#include "uiot_import.h"
#include "shadow_client.h"

/*场景说明:以智能手环为例，该智能手环有统计行走步数的功能。*/

#define UIOT_MY_PRODUCT_SN            "PRODUCT_SN"

#define UIOT_MY_DEVICE_SN             "DEVICE_SN"

#define UIOT_MY_DEVICE_SECRET         "DEVICE_SECRET"

static UIoT_Shadow    *sg_pshadow;
static MQTTInitParams sg_initParams = DEFAULT_MQTT_INIT_PARAMS;

/* 行走步数只能通过设备上报到云端，云端不能控制数值 */
void Walk_Step_Callback(void *pClient, RequestParams *pParams, char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty)
{
    IOT_Shadow_Request_Add_Delta_Property(pClient, pParams,pProperty);
    HAL_Printf("Heart_Rate_RegCallback\n");
    return;
}

static void _update_ack_cb(void *pClient, Method method, RequestAck requestAck, const char *pReceivedJsonDocument, void *pUserdata) 
{
	LOG_DEBUG("requestAck=%d\n", requestAck);

    if (NULL != pReceivedJsonDocument) {
        LOG_DEBUG("Received Json Document=%s\n", pReceivedJsonDocument);
    } else {
        LOG_DEBUG("Received Json Document is NULL\n");
    }

    *((RequestAck *)pUserdata) = requestAck;
    return;
}


/**
 * 设置MQTT connect初始化参数
 *
 * @param initParams MQTT connet初始化参数
 *
 * @return 0: 参数初始化成功  非0: 失败
 */
static int _setup_connect_init_params(MQTTInitParams* initParams)
{
    int ret = SUCCESS_RET;
	initParams->device_sn = (char *)UIOT_MY_DEVICE_SN;
	initParams->product_sn = (char *)UIOT_MY_PRODUCT_SN;
	initParams->device_secret = (char *)UIOT_MY_DEVICE_SECRET;

	initParams->command_timeout = UIOT_MQTT_COMMAND_TIMEOUT;
	initParams->keep_alive_interval = UIOT_MQTT_KEEP_ALIVE_INTERNAL;
	initParams->auto_connect_enable = 1;

    return ret;
}

int main()  
{    
    int ret = SUCCESS_RET;
    ret = _setup_connect_init_params(&sg_initParams);
    if(ret != SUCCESS_RET)
    {
        HAL_Printf("_setup_connect_init_params fail:%d\n", ret);
        return ret;
    }
    
    void *mqtt_client = IOT_MQTT_Construct(&sg_initParams);
    if(mqtt_client == NULL)
    {
        HAL_Printf("IOT_MQTT_Construct fail\n");
        return ERR_PARAM_INVALID;
    }
    
    void *shadow_client = IOT_Shadow_Construct(UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN, mqtt_client);
    if(shadow_client == NULL)
    {
        HAL_Printf("IOT_Shadow_Construct fail\n");
        return ERR_PARAM_INVALID;
    }
    
    sg_pshadow = (UIoT_Shadow *)shadow_client;
    bool isConnected = IOT_MQTT_IsConnected(sg_pshadow->mqtt);
    if(isConnected != true)
    {
        HAL_Printf("IOT_MQTT_IsConnected fail\n");
        return ERR_PARAM_INVALID;
    }
    
    int time_sec = MAX_WAIT_TIME_SEC;
	RequestAck ack_update = ACK_NONE;

    /* 手环统计的行走步数 */
    DeviceProperty *property_walk_step = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    uint32_t walk_step_num = 0;
    char walk_step_str[64] = "walk_step";
    property_walk_step->key= walk_step_str;
    property_walk_step->data = &walk_step_num;
    property_walk_step->type = JUINT32;
    ret = IOT_Shadow_Register_Property(sg_pshadow, property_walk_step, Walk_Step_Callback); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register walk_step fail:%d\n", ret);
        return ret;
    }

    /* 先同步一下版本号和设备掉电期间更新的属性 */
    ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Get Sync fail:%d\n", ret);
        return ret;
    }

	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }


    /* 模拟心率升高产生告警的场景 */
    while(1)
    {
            walk_step_num = walk_step_num + 5;

            ret = IOT_Shadow_Update(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 1, property_walk_step);
            if(SUCCESS_RET != ret)
            {
                HAL_Printf("Update walk_step fail:%d\n", ret);
                return ret;
            }

            ack_update = ACK_NONE;
            while (ACK_NONE == ack_update) {
                IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
            }

            IOT_Shadow_Yield(sg_pshadow, 3000);

            if(walk_step_num >= 100)
            {
                HAL_Printf("today's goal is completed!\n");
                break;
            }

    }

    HAL_Free(property_walk_step);
    IOT_Shadow_Destroy(sg_pshadow);

    return ret;
}


