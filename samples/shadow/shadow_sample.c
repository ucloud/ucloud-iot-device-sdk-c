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

#define UIOT_MY_PRODUCT_SN            "PRODUCT_SN"

#define UIOT_MY_DEVICE_SN             "DEVICE_SN"

#define UIOT_MY_DEVICE_SECRET         "DEVICE_SECRET"

#define MAX_SIZE_OF_TOPIC_CONTENT 100

#define SIZE_OF_JSON_BUFFER 256

static UIoT_Shadow    *sg_pshadow;
static MQTTInitParams sg_initParams = DEFAULT_MQTT_INIT_PARAMS;

//当设备直接按照desired字段中的属性值更新时不需要上报
void RegCallback_update(void *pClient, RequestParams *pParams, char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty)
{
    LOG_DEBUG("key:%s val:%s\n",pProperty->key, pJsonValueBuffer);
    IOT_Shadow_Direct_Update_Value(pJsonValueBuffer, pProperty);
    return;
}

//当设备没有完全按照desired字段中的属性更新时,需要将当前真实值上报
void RegCallback_hold(void *pClient, RequestParams *pParams, char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty)
{
    LOG_DEBUG("key:%s val:%s\n",pProperty->key, pJsonValueBuffer);
    int num = 10;
    pProperty->data = &num;
    IOT_Shadow_Request_Add_Delta_Property(pClient, pParams,pProperty);
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

    DeviceProperty *Property1 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    int32_t num1 = 18;
    char str1[6] = "data1";
    Property1->key= str1;
    Property1->data = &num1;
    Property1->type = JINT32;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property1, RegCallback_hold); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register Property1 fail:%d\n", ret);
        return ret;
    }
    
    DeviceProperty *Property2 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    float num2 = 20.2;
    char str2[6] = "data2";
    Property2->key= str2;
    Property2->data = &num2;
    Property2->type = JFLOAT;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property2, RegCallback_update); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register Property2 fail:%d\n", ret);
        return ret;
    }

    DeviceProperty *Property3 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    double num3 = 22.9;
    char str3[6] = "data3";
    Property3->key= str3;
    Property3->data = &num3;
    Property3->type = JDOUBLE;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property3, RegCallback_update); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register Property3 fail:%d\n", ret);
        return ret;
    }
    
    DeviceProperty *Property4 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    char num4[5] = "num4";
    char str4[6] = "data4";
    Property4->key= str4;
    Property4->data = num4;
    Property4->type = JSTRING;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property4, RegCallback_update); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register Property4 fail:%d\n", ret);
        return ret;
    }

    DeviceProperty *Property5 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    bool num5 = false;
    char str5[6] = "data5";
    Property5->key= str5;
    Property5->data = &num5;
    Property5->type = JBOOL;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property5, RegCallback_update); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register Property5 fail:%d\n", ret);
        return ret;
    }

    DeviceProperty *Property6 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    char num6[20] = "{\"temp\":25}";
    char str6[6] = "data6";
    Property6->key= str6;
    Property6->data = num6;
    Property6->type = JOBJECT;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property6, RegCallback_update); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register Property6 fail:%d\n", ret);
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
   
    /* update */    
    ack_update = ACK_NONE;
    ret = IOT_Shadow_Update(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 6, Property1, Property2, Property3, Property4, Property5, Property6);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Update Property1 Property2 Property3 Property4 Property5 Property6 fail:%d\n", ret);
        return ret;
    }
    
	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    ack_update = ACK_NONE;
    ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);

	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    /* update */    
    num1 = 123;
    Property1->data = &num1;

    char num9[5] = "num9";
    Property4->data = num9;

    ack_update = ACK_NONE;
    ret = IOT_Shadow_Update(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 2, Property1, Property4);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Update Property1 Property4 fail:%d\n", ret);
        return ret;
    }
    
	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    /* delete */    
    ack_update = ACK_NONE;
    ret = IOT_Shadow_Delete(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 2, Property1, Property2);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Delete Property1 Property2 fail:%d\n", ret);
        return ret;
    }

	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    ack_update = ACK_NONE;
    ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);


	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    /* delete all */
    ack_update = ACK_NONE;
    ret = IOT_Shadow_Delete_All(sg_pshadow, _update_ack_cb, time_sec, &ack_update);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Delete All fail:%d\n", ret);
        return ret;
    }


	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    ack_update = ACK_NONE;
    ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);


	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    Property1->data = &num1;
    Property4->data = num4;
    Property5->data = &num5;
    Property6->data = num6;

    /* update */    
    ack_update = ACK_NONE;
    ret = IOT_Shadow_Update_And_Reset_Version(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 4, Property1, Property4, Property5, Property6);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Update and Reset Ver fail:%d\n", ret);
        return ret;
    }
    
	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    ack_update = ACK_NONE;
    ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);

	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    HAL_Free(Property1);
    HAL_Free(Property2);
    HAL_Free(Property3);
    HAL_Free(Property4);
    HAL_Free(Property5);
    HAL_Free(Property6);
    IOT_Shadow_Destroy(sg_pshadow);

    return ret;
}


