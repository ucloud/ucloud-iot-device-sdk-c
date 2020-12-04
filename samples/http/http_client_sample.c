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

#include "uiot_import.h"
#include "ca.h"
#include "utils_httpc.h"
#include "uiot_export_http.h"

#define UIOT_MY_PRODUCT_SN            "PRODUCT_SN"

#define UIOT_MY_DEVICE_SN             "DEVICE_SN"

#define UIOT_MY_DEVICE_SECRET         "DEVICE_SECRET"

#define UIOT_PUBLISH_TOPIC            "%s/%s/upload/event"

int main(int argc, char **argv) {    
    char *token = (char *)HAL_Malloc(1024);
    memset(token, 0, 1024);
    int ret = SUCCESS_RET;
    char *topic = (char *)HAL_Malloc(256);
    memset(topic, 0, 256);
    HAL_Snprintf((char *)topic, 256, UIOT_PUBLISH_TOPIC,UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN);
    char *data = "{\"test\": \"18\"}";

    ret = IOT_HTTP_Get_Token(UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN, UIOT_MY_DEVICE_SECRET, token);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("get Token fail,ret:%d\r\n", ret);
        return FAILURE_RET;
    }

    HAL_Printf("get token:%s\n", token);
    HAL_Printf("topic:%s\n", topic);
    ret = IOT_HTTP_Publish(token, topic, data, 5000);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Publish fail,ret:%d\r\n", ret);
        return FAILURE_RET;
    }
    HAL_Printf("Publish success\n");
    HAL_Free(token);
    HAL_Free(topic);
    return ret;
}

