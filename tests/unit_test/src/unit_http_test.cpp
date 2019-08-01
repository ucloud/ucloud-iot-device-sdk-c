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

#include <stdio.h>
#include <string.h>
#include <gtest/gtest.h>
#include <limits.h>
#include <iostream>

#include "uiot_import.h"
#include "ca.h"
#include "utils_httpc.h"

class HTTPClientTests : public testing::Test
{
protected:
    virtual void SetUp()
    {
        std::cout << "HTTPClientTests Test Begin \n";
    }
    virtual void TearDown()
    {
        std::cout << "HTTPClientTests Test End \n";
    }
};

TEST_F(HTTPClientTests, HTTPDownload) {
    http_client_t *http_client = (http_client_t *)HAL_Malloc(sizeof(http_client_t));
    http_client_data_t *http_data = (http_client_data_t *)HAL_Malloc(sizeof(http_client_data_t));
    memset(http_client, 0, sizeof(http_client_t));
    memset(http_data, 0, sizeof(http_client_data_t));

    char *buf = (char *)HAL_Malloc(2046);
    memset(buf, 0, 2046);
    http_data->response_buf = buf;
    http_data->response_buf_len = 2046;
    http_client->header = (char *)"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n" \
                         "Accept-Encoding: gzip, deflate\r\n";

    FILE *fp;
    char *file_path = (char*)"download.txt";
    fp = fopen(file_path, "wb+");
    ASSERT_TRUE(NULL != fp);

    char *url = (char *)"https://uiot.cn-sh2.ufileos.com/test.txt";

    const char *ca_crt = iot_https_ca_get();
    uint32_t timeout_ms = 5000;
    int rc;
    int32_t total = 0;
#ifdef SUPPORT_TLS
    http_client_common(http_client, url, 443, ca_crt, HTTP_GET, http_data);
#else
    http_client_common(http_client, url, 80, ca_crt, HTTP_GET, http_data);
#endif
    do {
        int diff = http_data->response_content_len - http_data->retrieve_len;

        http_client_recv_data(http_client, timeout_ms, http_data);

        int32_t len = http_data->response_content_len - http_data->retrieve_len - diff;
        if (len > 0) {
            rc = fwrite(http_data->response_buf, len, 1, fp);
            ASSERT_TRUE(rc == 1);
            total += len;
        }
    } while (http_data->retrieve_len != 0);

    fclose(fp);
    http_client_close(http_client);
    HAL_Free(buf);
    HAL_Free(http_data);
    HAL_Free(http_client);

    ASSERT_TRUE(total == 11358);
}
