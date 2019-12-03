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
#include "uiot_export_file_upload.h"

#define UIOT_MY_PRODUCT_SN            "PRODUCT_SN"

#define UIOT_MY_DEVICE_SN             "DEVICE_SN"

#define UIOT_MY_DEVICE_SECRET         "DEVICE_SECRET"

#define FILE_PATH                     "FILE_PATH"

int main(int argc, char **argv) {    
    char md5[100];    
    char *authorization = (char *)malloc(1024);
    memset(authorization, 0, 1024);
    char *put_url = (char *)malloc(1024);
    memset(put_url, 0, 1024);
    int ret = SUCCESS_RET;

    http_client_file_md5(FILE_PATH, md5);
    HAL_Printf("MD5:%s\n", md5);
    
    ret = IOT_GET_URL_AND_AUTH(UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN, UIOT_MY_DEVICE_SECRET, FILE_PATH, md5, authorization, put_url);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("get url and auth fail,ret:%d\r\n", ret);
        return FAILURE_RET;
    }

    HAL_Printf("get MD5:%s\n", md5);
    HAL_Printf("get authorization:%s\n", authorization);
    HAL_Printf("get put_url:%s\n", put_url);

    //上传文件超时时间与文件长度相关
    ret = IOT_UPLOAD_FILE(FILE_PATH, md5, authorization, put_url, 40000);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("upload file fail,ret:%d\r\n", ret);
        return FAILURE_RET;
    }
    HAL_Printf("upload success\n");
    HAL_Free(authorization);
    HAL_Free(put_url);
}

