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

/*场景说明:以智能手环为例，该智能手环有监测心率当心率大于180时报警的功能。*/

#define UIOT_MY_PRODUCT_SN            "PRODUCT_SN"

#define UIOT_MY_DEVICE_SN             "DEVICE_SN"

#define UIOT_MY_DEVICE_SECRET         "DEVICE_SECRET"

#define MAX_SIZE_OF_TOPIC_CONTENT 100

#define SIZE_OF_JSON_BUFFER 256

static UIoT_Shadow    *sg_pshadow;
static MQTTInitParams sg_initParams = DEFAULT_MQTT_INIT_PARAMS;

/* 可以通过云平台控制手环开关 */
void Power_Callback(void *pClient, RequestParams *pParams, char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty)
{
    IOT_Shadow_Direct_Update_Value(pJsonValueBuffer, pProperty);
    HAL_Printf("Power_Callback\n");
    return;
}

/* 心率只能通过设备上报到云端，云端不能控制数值 */
void Heart_Rate_RegCallback(void *pClient, RequestParams *pParams, char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty)
{
    IOT_Shadow_Request_Add_Delta_Property(pClient, pParams,pProperty);
    HAL_Printf("Heart_Rate_RegCallback\n");
    return;
}

/* 告警只能通过心率下降后恢复，云端不能恢复告警 */
void Alarm_RegCallback(void *pClient, RequestParams *pParams, char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty)
{
    IOT_Shadow_Request_Add_Delta_Property(pClient, pParams,pProperty);
    HAL_Printf("Alarm_RegCallback\n");
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
 * 设置MQTT connet初始化参数
 *
 * @param initParams MQTT connet初始化参数
 *
 * @return 0: 参数初始化成功  非0: 失败
 */
static int _setup_connect_init_params(MQTTInitParams* initParams)
{
    int ret = SUCCESS;
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
    int ret = SUCCESS;
    ret = _setup_connect_init_params(&sg_initParams);
    if(ret != SUCCESS)
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

    /* 手环启动状态 */
    DeviceProperty *property_power = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    bool power = true;
    char power_str[64] = "power";
    property_power->key= power_str;
    property_power->data = &power;
    property_power->type = JBOOL;
    ret = IOT_Shadow_Register_Property(sg_pshadow, property_power, Power_Callback); 
    if(SUCCESS != ret)
    {
        HAL_Printf("Register power fail:%d\n", ret);
        return ret;
    }

    /* 手环监测的当前心率 */
    DeviceProperty *property_heart_rate = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    uint32_t heart_rate_num = 90;
    char heart_rate_str[64] = "heart_rate";
    property_heart_rate->key= heart_rate_str;
    property_heart_rate->data = &heart_rate_num;
    property_heart_rate->type = JUINT32;
    ret = IOT_Shadow_Register_Property(sg_pshadow, property_heart_rate, Heart_Rate_RegCallback); 
    if(SUCCESS != ret)
    {
        HAL_Printf("Register heart_rate fail:%d\n", ret);
        return ret;
    }

    /* 手环的心率告警状态 */
    DeviceProperty *property_alarm = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    bool alarm_state = false;
    char alarm_str[64] = "alarm_state";
    property_alarm->key= alarm_str;
    property_alarm->data = &alarm_state;
    property_alarm->type = JBOOL;
    ret = IOT_Shadow_Register_Property(sg_pshadow, property_alarm, Alarm_RegCallback); 
    if(SUCCESS != ret)
    {
        HAL_Printf("Register alarm_state fail:%d\n", ret);
        return ret;
    }

    /* 先同步一下版本号和设备掉电期间更新的属性 */
    ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);
    if(SUCCESS != ret)
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
        if(power == true)
        {
            HAL_Printf("smart_bracelet is power on,heart_rate_num:%d alarm_state:%d\n", heart_rate_num, alarm_state);
            ret = IOT_Shadow_Update(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 3, property_power, property_heart_rate, property_alarm);
            if(SUCCESS != ret)
            {
                HAL_Printf("Update property_power property_heart_rate property_alarm fail:%d\n", ret);
                return ret;
            }

            ack_update = ACK_NONE;
            while (ACK_NONE == ack_update) {
                IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
            }

            IOT_MQTT_Yield(mqtt_client, 10000);
            heart_rate_num = heart_rate_num + 10;
            if(heart_rate_num >= 150)
            {
                alarm_state = true;
            }
            if(heart_rate_num >= 180)
            {
                alarm_state = false;
                heart_rate_num  = 90;
            }
        }
        else
        {   
            ret = IOT_Shadow_Update(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 3, property_power, property_heart_rate, property_alarm);
            if(SUCCESS != ret)
            {
                HAL_Printf("Update property_power property_heart_rate property_alarm fail:%d\n", ret);
                return ret;
            }

            ack_update = ACK_NONE;
            while (ACK_NONE == ack_update) {
                IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
            }
            HAL_Printf("smart_bracelet is power off\n");            
            break;
        }

    }

    HAL_Free(property_power);
    HAL_Free(property_heart_rate);
    HAL_Free(property_alarm);
    IOT_Shadow_Destroy(sg_pshadow);

    return ret;
}


