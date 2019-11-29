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
#include <string.h>

#include "utils_httpc.h"
#include "lite-utils.h"
#include "ca.h"

static int calc_file_len(char *file_path)
{
    FILE *fp;
    fp = fopen(file_path, "rb+");
    if(NULL == fp)
    {
        return 0;
    }

    //计算文件大小，申请内存
    fseek(fp,0L,SEEK_END); 
    int file_len = ftell(fp);
    fclose(fp);
    return file_len;
}

int IOT_GET_URL_AND_AUTH(const char *product_sn, const char *device_sn, const char *device_sercret, char *file_path, char *md5, char *authorization, char *put_url)
{
    int ret = SUCCESS_RET;
    http_client_t *http_client_post = (http_client_t *)HAL_Malloc(sizeof(http_client_t));
    http_client_data_t *http_data_post = (http_client_data_t *)HAL_Malloc(sizeof(http_client_data_t));
    memset(http_client_post, 0, sizeof(http_client_t));
    memset(http_data_post, 0, sizeof(http_client_data_t));

    http_data_post->response_buf = (char *)HAL_Malloc(2046);
    memset(http_data_post->response_buf, 0, 2046);
    http_data_post->response_buf_len = 2046;
    http_data_post->post_content_type = (char *)"application/json";
    http_data_post->post_buf = (unsigned char *)HAL_Malloc(2046);
    memset(http_data_post->post_buf, 0, 2046);
    
    int file_len = calc_file_len(file_path);
    if(0 == file_len)
    {
        LOG_ERROR("File not exist\n");
        ret = ERR_PARAM_INVALID;
        goto end; 
    }
    HAL_Snprintf((char *)http_data_post->post_buf, 2046, "{\"ProductSN\":\"%s\",\"DeviceSN\":\"%s\",\"DeviceSecret\":\"%s\","   \
                                                         "\"FileName\":\"%s\",\"FileSize\":%d,\"MD5\":\"%s\"}",product_sn, device_sn, device_sercret, file_path, file_len, md5);
    http_data_post->post_buf_len = strlen((char *)http_data_post->post_buf);
    
    http_client_post->header = (char *)"Content-Type: application/json\r\n";

    const char *ca_crt = iot_https_ca_get();
    char *url = (char *)"https://file-cn-sh2.iot.ucloud.cn/api/v1/url";

    ret = http_client_common(http_client_post, url, 443, ca_crt, HTTP_POST, http_data_post);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("HTTP_POST error\n");
        goto end;
    }
    
    ret = http_client_recv_data(http_client_post, 5000, http_data_post);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("http_client_recv_data error\n");
        goto end;
    }
    LOG_DEBUG("response_buf:%s\n",http_data_post->response_buf);

    char *temp = LITE_json_value_of((char *)"Authorization", http_data_post->response_buf);
    strncpy(authorization,temp,strlen(temp));
    authorization[strlen(temp)+1] = '\0';
    LOG_DEBUG("authorization:%s\n",authorization);

    temp = LITE_json_value_of((char *)"URL", http_data_post->response_buf);
    strncpy(put_url,temp,strlen(temp));
    put_url[strlen(temp)+1] = '\0';
    LOG_DEBUG("put_url:%s\n",put_url);

end:
    http_client_close(http_client_post);
    HAL_Free(http_client_post);
    
    HAL_Free(http_data_post->response_buf);
    HAL_Free(http_data_post->post_buf);
    HAL_Free(http_data_post);
    return ret;
}

int IOT_UPLOAD_FILE(char *file_path, char *md5, char *authorization, char *url)
{
    int ret = SUCCESS_RET;
    http_client_t *http_client_put = (http_client_t *)HAL_Malloc(sizeof(http_client_t));
    http_client_data_t *http_data_put = (http_client_data_t *)HAL_Malloc(sizeof(http_client_data_t));
    memset(http_client_put, 0, sizeof(http_client_t));
    memset(http_data_put, 0, sizeof(http_client_data_t));

    http_data_put->response_buf = (char *)HAL_Malloc(512);
    memset(http_data_put->response_buf, 0, 512);
    http_data_put->response_buf_len = 512;

    http_client_put->header = (char *)HAL_Malloc(2046);
    memset(http_client_put->header, 0, 2046);
    HAL_Snprintf(http_client_put->header, 2046, "Authorization:%s\r\nContent-Type:plain/text\r\nContent-MD5:%s\r\n",authorization,md5);
                                      
    LOG_DEBUG("header:%s\n", http_client_put->header);

    http_data_put->post_content_type = (char *)"plain/text";

    int file_len = calc_file_len(file_path);
    if(0 == file_len)
    {
        LOG_ERROR("File not exist\n");
        HAL_Free(http_client_put->header);
        HAL_Free(http_client_put);
        HAL_Free(http_data_put);
        return ERR_PARAM_INVALID;
    }
    http_data_put->post_buf = (unsigned char *)HAL_Malloc(file_len +1 );
    if(NULL == http_data_put->post_buf)
    {
        LOG_ERROR("http_data_put->post_buf is null\n");        
        HAL_Free(http_client_put->header);
        HAL_Free(http_client_put);
        HAL_Free(http_data_put);
        return ERR_PARAM_INVALID;
    }
    memset(http_data_put->post_buf, 0, file_len +1);
    
    FILE *fp;
    fp = fopen(file_path, "rb+");
    if(NULL == fp)
    {
        ret = ERR_PARAM_INVALID;
        goto end;
    }

    int count = fread((char *)http_data_put->post_buf, 1, file_len, fp);
    http_data_put->post_buf[file_len]= '\0'; 
    http_data_put->post_buf_len = count;    
    const char *ca_crt = iot_https_ca_get();

    ret = http_client_common(http_client_put, url, 443, ca_crt, HTTP_PUT, http_data_put);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("http_client_common error\n");
        goto end;
    }

    ret = http_client_recv_data(http_client_put, 15000, http_data_put);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("http_client_recv_data error\n");
        goto end;
    }

    LOG_DEBUG("content_len:%d response_received_len:%d\n",http_data_put->response_content_len, http_data_put->response_received_len);
    LOG_DEBUG("response_buf:%s\n",http_data_put->response_buf);

    fclose(fp);
end:
    http_client_close(http_client_put);
    HAL_Free(http_data_put->post_buf);
    HAL_Free(http_client_put->header);
    HAL_Free(http_client_put);
    HAL_Free(http_data_put);
    return ret;
}


