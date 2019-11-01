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

// Based on qcloud-iot-sdk-embedded-c
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

#include <stdbool.h>
#include <iostream>

#include <gtest/gtest.h>

#include "uiot_export_shadow.h"
#include "uiot_import.h"
#include "shadow_client.h"

#define UIOT_MY_PRODUCT_SN            "PRODUCT_SN"

#define UIOT_MY_DEVICE_SN             "DEVICE_SN"

#define UIOT_MY_DEVICE_SECRET         "DEVICE_SECRET"

#define MAX_SIZE_OF_TOPIC_CONTENT 100

#define SIZE_OF_JSON_BUFFER 256

static UIoT_Shadow    *sg_pshadow;
static MQTTInitParams     sg_initParams = DEFAULT_MQTT_INIT_PARAMS;

using namespace std;

void RegCallback(void *pClient, RequestParams *pParams, char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty)
{
    HAL_Printf("key:%s val:%s\n",pProperty->key, pJsonValueBuffer);
    //IOT_Shadow_Direct_Update_Value(pJsonValueBuffer, pProperty);
    IOT_Shadow_Request_Add_Delta_Property(pClient, pParams, pProperty);
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


class ShadowTest : public testing::Test
{
protected:
    virtual void SetUp() 
    {
    }

    virtual void TearDown()
    {
    }
};

/**
 * 设置MQTT connet初始化参数
 *
 * @param initParams MQTT connet初始化参数
 *
 * @return 0: 参数初始化成功  非0: 失败
 */
static int _setup_connect_init_params(MQTTInitParams* initParams)
{
    int ret = 0;
	initParams->device_sn = (char *)UIOT_MY_DEVICE_SN;
	initParams->product_sn = (char *)UIOT_MY_PRODUCT_SN;
	initParams->device_secret = (char *)UIOT_MY_DEVICE_SECRET;

	initParams->command_timeout = UIOT_MQTT_COMMAND_TIMEOUT;
	initParams->keep_alive_interval = UIOT_MQTT_KEEP_ALIVE_INTERNAL;
	initParams->auto_connect_enable = 1;

    return ret;
}

TEST_F(ShadowTest, ShadowDocTest_update)
{    
    int ret = SUCCESS_RET;
    ret = _setup_connect_init_params(&sg_initParams);
    ASSERT_TRUE(ret == SUCCESS_RET);
    
    void *mqtt_client = IOT_MQTT_Construct(&sg_initParams);
    ASSERT_TRUE(mqtt_client != NULL);
    
    void *shadow_client = IOT_Shadow_Construct(UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN, mqtt_client);
    ASSERT_TRUE(shadow_client != NULL);
    
    sg_pshadow = (UIoT_Shadow *)shadow_client;
    bool isConnected = IOT_MQTT_IsConnected(sg_pshadow->mqtt);
    ASSERT_EQ(true, isConnected);
    
    int time_sec = MAX_WAIT_TIME_SEC;
	RequestAck ack_update = ACK_NONE;

    DeviceProperty *Property1 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    int num1 = 18;
    char str1[6] = "data1";
    Property1->key= str1;
    Property1->data = &num1;
    Property1->type = JINT32;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property1, RegCallback); 
    ASSERT_TRUE(ret == SUCCESS_RET);
    
    DeviceProperty *Property2 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    float num2 = 20.2;
    char str2[6] = "data2";
    Property2->key= str2;
    Property2->data = &num2;
    Property2->type = JFLOAT;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property2, RegCallback); 
    ASSERT_TRUE(ret == SUCCESS_RET);

    DeviceProperty *Property3 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    double num3 = 22.9;
    char str3[6] = "data3";
    Property3->key= str3;
    Property3->data = &num3;
    Property3->type = JDOUBLE;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property3, RegCallback); 
    ASSERT_TRUE(ret == SUCCESS_RET);
    
    DeviceProperty *Property4 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    char num4[5] = "num4";
    char str4[6] = "data4";
    Property4->key= str4;
    Property4->data = num4;
    Property4->type = JSTRING;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property4, RegCallback); 
    ASSERT_TRUE(ret == SUCCESS_RET);
    
    DeviceProperty *Property5 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    bool num5 = false;
    char str5[6] = "data5";
    Property5->key= str5;
    Property5->data = &num5;
    Property5->type = JBOOL;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property5, RegCallback); 
    ASSERT_TRUE(ret == SUCCESS_RET);

    DeviceProperty *Property6 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    char num6[20] = "{\"temp\":25}";
    char str6[6] = "data6";
    Property6->key= str6;
    Property6->data = num6;
    Property6->type = JOBJECT;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property6, RegCallback); 
    ASSERT_TRUE(ret == SUCCESS_RET);

    /* 先同步一下版本号和设备掉电期间更新的属性 */
    ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);
    ASSERT_TRUE(ret == SUCCESS_RET);

	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }
   
    /* update */    
    ack_update = ACK_NONE;
    ret = IOT_Shadow_Update(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 6, Property1, Property2, Property3, Property4, Property5, Property6);
    ASSERT_TRUE(ret == SUCCESS_RET);
    
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
    ASSERT_TRUE(ret == SUCCESS_RET);
    
	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    /* delete */    
    ack_update = ACK_NONE;
    ret = IOT_Shadow_Delete(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 2, Property1, Property2);
    ASSERT_TRUE(ret == SUCCESS_RET);

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
    ASSERT_TRUE(ret == SUCCESS_RET);


	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    ack_update = ACK_NONE;
    ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);


	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    /* update */    
    num2 = 20.6;
    Property2->data = &num2;

    ack_update = ACK_NONE;
    ret = IOT_Shadow_Update(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 1, Property2);
    ASSERT_TRUE(ret == SUCCESS_RET);
    
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
    ASSERT_TRUE(ret == SUCCESS_RET);

    
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
}

#if 0
TEST_F(ShadowTest, ShadowDocTest_delete_control)
{    
    int ret = SUCCESS_RET;
    ret = _setup_connect_init_params(&sg_initParams);
    ASSERT_TRUE(ret == SUCCESS_RET);
    
    void *mqtt_client = IOT_MQTT_Construct(&sg_initParams);
    ASSERT_TRUE(mqtt_client != NULL);
    
    void *shadow_client = IOT_Shadow_Construct(mqtt_client);
    ASSERT_TRUE(shadow_client != NULL);
    
    sg_pshadow = (UIoT_Shadow *)shadow_client;
    bool isConnected = IOT_MQTT_IsConnected(sg_pshadow->mqtt);
    ASSERT_EQ(true, isConnected);
    
    int time_sec = MAX_WAIT_TIME_SEC;
	RequestAck ack_update = ACK_NONE;

    DeviceProperty Property1;
    int num1 = 18;
    char str1[6] = "data1";
    Property1.key= str1;
    Property1.data = &num1;
    Property1.type = JINT32;
    IOT_Shadow_Register_Property(sg_pshadow, &Property1, RegCallback); 
    
    DeviceProperty Property2;
    float num2 = 20.2;
    char str2[6] = "data2";
    Property2.key= str2;
    Property2.data = &num2;
    Property2.type = JFLOAT;
    IOT_Shadow_Register_Property(sg_pshadow, &Property2, RegCallback); 

    DeviceProperty Property3;
    double num3 = 22.9;
    char str3[6] = "data3";
    Property3.key= str3;
    Property3.data = &num3;
    Property3.type = JDOUBLE;
    IOT_Shadow_Register_Property(sg_pshadow, &Property3, RegCallback); 
    
    DeviceProperty Property4;
    char num4[5] = "num4";
    char str4[6] = "data4";
    Property4.key= str4;
    Property4.data = num4;
    Property4.type = JSTRING;
    IOT_Shadow_Register_Property(sg_pshadow, &Property4, RegCallback); 

    DeviceProperty Property5;
    bool num5 = 0;
    char str5[6] = "data5";
    Property5.key= str5;
    Property5.data = &num5;
    Property5.type = JBOOL;
    IOT_Shadow_Register_Property(sg_pshadow, &Property5, RegCallback); 

    DeviceProperty Property6;
    char num6[20] = "{\"temp\":25}";
    char str6[6] = "data6";
    Property6.key= str6;
    Property6.data = num6;
    Property6.type = JOBJECT;
    IOT_Shadow_Register_Property(sg_pshadow, &Property6, RegCallback); 
    shadow_print_property(sg_pshadow);
   
    /* update */    
    ack_update = ACK_NONE;
    IOT_Shadow_Update(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 6, &Property1, &Property2, &Property3, &Property4, &Property5, &Property6);
    
	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    ack_update = ACK_NONE;
    IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);

	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
    }

    shadow_print_property(sg_pshadow);
    
}
#endif
int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
