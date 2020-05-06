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
#include <string.h>
#include "at_inf.h"
#include "utils_net.h"
#include "at_ringbuff.h"
#include "at_client.h"
#include "uiot_import.h"
#include "at_utils.h"

#define ESP8266_MAX_TCP_LINK        5

extern sRingbuff g_ring_buff;    
extern sRingbuff g_ring_tcp_buff[3];    
int esp8266_link[ESP8266_MAX_TCP_LINK] = {0};
int esp8266_recv_len[ESP8266_MAX_TCP_LINK] = {0};

#define WIFI_SSID           "WIFI_SSID"

#define WIFI_SECRET         "WIFI_SECRET"

int HAL_AT_Read_Tcp(_IN_ utils_network_pt pNetwork, _IN_ unsigned char *buffer, _IN_ size_t len)
{
    at_response_t resp = NULL;
    
    resp = at_create_resp(256, 0, CMD_TIMEOUT_MS);
    if (resp == NULL)
    {
        LOG_ERROR("No memory for response object!");
        return FAILURE_RET;
    }

    /* 被动接收模式，查询缓存TCP数据的长度 */
    resp->custom_flag = true;
    HAL_SleepMs(10);
    at_exec_cmd(resp, at_command, 0, "AT+CIPRECVLEN?\r\n"); 

    /* 被动接收模式，读取缓存的TCP数据 */
    if(esp8266_recv_len[pNetwork->handle-1] >= len)
    {
        resp->custom_flag = true;
        at_exec_cmd(resp, at_command, 0, "AT+CIPRECVDATA=%d,%d\r\n", pNetwork->handle-1, len); 

        at_delete_resp(resp);
        return HAL_AT_Read(pNetwork, buffer, len);
    }
    else
    {
        at_delete_resp(resp);
        return 0;
    }


}

int HAL_AT_Write_Tcp(_IN_ utils_network_pt pNetwork, _IN_ unsigned char *buffer, _IN_ size_t len)
{
    at_response_t resp = NULL;
    
    resp = at_create_resp(256, 0, CMD_TIMEOUT_MS);
    if (resp == NULL)
    {
        LOG_ERROR("No memory for response object!");
        return FAILURE_RET;
    }

    resp->custom_flag = true;
    at_exec_cmd(resp, at_command, 0, "AT+CIPSEND=%d,%d\r\n", pNetwork->handle-1, len);   
    HAL_SleepMs(10);
    resp->custom_flag = false;
    at_exec_cmd(resp, at_data, len, (const char *)buffer); 
    HAL_SleepMs(10);

    at_delete_resp(resp);
    return len;
}

int HAL_AT_TCP_Disconnect(utils_network_pt pNetwork)
{
    int ret = SUCCESS_RET;

    /* 断开无线链接 */
    if(esp8266_link[pNetwork->handle-1] == eCONNECTED)
    {
        at_response_t resp = NULL;
        
        resp = at_create_resp(256, 0, CMD_TIMEOUT_MS);
        if (resp == NULL)
        {
            LOG_ERROR("No memory for response object!");
            return FAILURE_RET;
        }
    
        resp->custom_flag = true;
        ret = at_exec_cmd(resp, at_command, 0,  "AT+CIPCLOSE=%d\r\n",pNetwork->handle-1);
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("close TCP link fail!\n");
        }
        else
        {
            esp8266_link[pNetwork->handle-1] = eDISCONNECTED;
        }        
        at_delete_resp(resp);
    }

    return ret;
}

static int urc_rst_recv_judge(const char *data, uint32_t size)
{
    if(NULL != strstr(data, "ready"))
    {
        return SUCCESS_RET;
    }
    else
    {
        return FAILURE_RET;
    }
}

static int urc_cwjap_recv_judge(const char *data, uint32_t size)
{
    if(NULL != strstr(data, "WIFI GOT IP"))
    {
        return SUCCESS_RET;
    }
    else
    {
        return FAILURE_RET;
    }
}

static int urc_send_recv_judge(const char *data, uint32_t size)
{
    char temp_char = 0;
    at_client_t client = at_client_get();
    if(NULL != strstr(data, "OK\r\n"))
    {
        at_client_getchar(client, &temp_char, GET_RECEIVE_TIMEOUT_MS);
        if('>' == temp_char)
            return SUCCESS_RET;
        else
            return FAILURE_RET;
    }
    else 
    {
        return FAILURE_RET;
    }
}


static int urc_common_recv_func(const char *data, uint32_t size)
{
    return SUCCESS_RET;
}

static int urc_tcp_start_judge(const char *data, uint32_t size)
{
    at_client_t client = at_client_get();
    if(NULL != strstr(data, "CONNECT\r\n"))
    {
        at_recv_readline(client);    
        at_recv_readline(client);
        return SUCCESS_RET;
    }
    else
    {
        return FAILURE_RET;
    }
}

static int urc_close_recv_judge(const char *data, uint32_t size)
{
    if(NULL != strstr(data, "CLOSED"))
    {
        return SUCCESS_RET;
    }
    else 
    {
        return FAILURE_RET;
    }
}

static int urc_recvdata_recv_judge(const char *data, uint32_t size)
{
    if(0 == strncmp(data, "+CIPRECVDATA", size))
    {
        return SUCCESS_RET;
    }
    else 
    {
        return FAILURE_RET;
    }
}

static int urc_recvdata_recv_func(const char *data, uint32_t size)
{
    at_client_t client = at_client_get();
    char temp_string[10] = {0};
    char temp_char = 0;
    int loop = 0;
    int ret = 0;
    int recv_data_num = 0;
    int link_num = 0;
    const char *cmd = NULL;
    int cmdsize = 0;
    
    do
    {
        ret = at_client_getchar(client, &temp_string[loop], GET_RECEIVE_TIMEOUT_MS);
        loop++;
    }while(temp_string[loop-1] != ':');
    
    cmd = at_get_last_cmd(&cmdsize);
    
    if(2 == sscanf(cmd,"AT+CIPRECVDATA=%d,%d\r\n",&link_num,&recv_data_num))
    {
        for(loop = recv_data_num; loop > 0; loop--)
        {
            ret = at_client_getchar(client, &temp_char, GET_RECEIVE_TIMEOUT_MS);
            ret |= ring_buff_push_data(&(g_ring_tcp_buff[link_num]), (uint8_t *)&temp_char, 1);
            if(SUCCESS_RET != ret)
            {
                LOG_ERROR("copy data to tcp buff fail\n");
            }
        }
    }
    
    return SUCCESS_RET;
}

static int urc_recvlen_recv_judge(const char *data, uint32_t size)
{
    if(NULL != strstr(data, "+CIPRECVLEN:"))
    {
        return SUCCESS_RET;
    }
    else 
    {
        return FAILURE_RET;
    }
}

static int urc_recvlen_recv_func(const char *data, uint32_t size)
{
    at_client_t client = at_client_get();
    char temp_string[30] = {0};
    int loop = 0;
    
    do
    {
        at_client_getchar(client, &temp_string[loop], GET_RECEIVE_TIMEOUT_MS);
        loop++;
    }while(temp_string[loop-1] != '\n');
        
    if(5 == sscanf(temp_string,"%d,%d,%d,%d,%d\r\n",&esp8266_recv_len[0], &esp8266_recv_len[1], &esp8266_recv_len[2], &esp8266_recv_len[3], &esp8266_recv_len[4]))
    {
        return SUCCESS_RET;
    }
    
    return FAILURE_RET;
}


at_custom custom_table[] = {
    {"AT+RST", 7, urc_rst_recv_judge, urc_common_recv_func},
    //{"AT+CWJAP", 11, urc_cwjap_recv_judge, urc_common_recv_func},
    {"AT+CIPSEND", 4, urc_send_recv_judge, urc_common_recv_func},
    {"AT+CIPSTART", 11, urc_tcp_start_judge, urc_common_recv_func},
    {"AT+CIPCLOSE", 9, urc_close_recv_judge, urc_common_recv_func},
    {"AT+CIPRECVLEN?", 12, urc_recvlen_recv_judge, urc_recvlen_recv_func},
    {"AT+CIPRECVDATA", 12, urc_recvdata_recv_judge, urc_recvdata_recv_func},
};

int custom_table_num = sizeof(custom_table) / sizeof(custom_table[0]);

static int esp8266_init()
{
    int ret = 0;
    at_response_t resp = NULL;
    int retry_time = 0;

    HAL_AT_Init();

    resp = at_create_resp(1024, 0, CMD_TIMEOUT_MS);
    if (resp == NULL)
    {
        LOG_ERROR("No memory for response object!");
        return FAILURE_RET;
    }

    for(retry_time = 0; retry_time < 5; retry_time++)
    {
        /* 模块重启 */
        resp->custom_flag = true;
        ret |= at_exec_cmd(resp, at_command, 0, "AT+RST\r\n");
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("AT rst fail!\n");
            goto end;
        }
        
        //延时等待模块启动
        HAL_SleepMs(1000);

        /* 去掉串口回显 */
        resp->custom_flag = false;
        ret |= at_exec_cmd(resp, at_command, 0, "ATE0\r\n");
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("ATE0 set fail!\n");
            goto end;
        }
        
        /* 设置Wi-Fi模式为station */
        resp->custom_flag = false;
        ret |= at_exec_cmd(resp, at_command, 0,  "AT+CWMODE=1\r\n");  
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("set Wi-Fi mode to station fail!\n");
            continue;
        }
        
        /* 设置模式连接方式为多链接 */  
        resp->custom_flag = false;
        ret |= at_exec_cmd(resp, at_command, 0,  "AT+CIPMUX=1\r\n");  
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("set mutil link fail!\n");
            continue;
        }

        /* 连接Wi-Fi   AP */   
        resp->custom_flag = false;   
        ret |= at_exec_cmd(resp, at_command, 0,  "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_SECRET); 
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("link Wi-Fi AP fail!\n");
            continue;
        }

        /* 获取IP地址 */
        resp->custom_flag = false;   
        ret |= at_exec_cmd(resp, at_command, 0,  "AT+CIFSR\r\n"); 
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("get ip addr fail!\n");
            continue;
        }

        if(SUCCESS_RET == ret)
        {
            HAL_SleepMs(1000);
            break;
        }
        
    }

end:
    at_delete_resp(resp);
    return ret;
}

int HAL_AT_TCP_Connect(_IN_ utils_network_pt pNetwork, _IN_ const char *host, _IN_ uint16_t port) 
{
    int ret = 0;
    at_response_t resp = NULL;
    int link_num = 0;
    at_client_t p_client = at_client_get();
    
    resp = at_create_resp(256, 0, CMD_TIMEOUT_MS);
    if (resp == NULL)
    {
        LOG_ERROR("No memory for response object!");        
        goto end;
    }

    for(link_num = 0; link_num < ESP8266_MAX_TCP_LINK; link_num++)
    {
        if(esp8266_link[link_num] == eDISCONNECTED)
        {
            break;
        }
    }

    if(AT_STATUS_INITIALIZED != p_client->status)
    {
        ret = module_init();
        if(ret != SUCCESS_RET)
        {
            LOG_ERROR("module init fail!\n");
            goto end;
        }
        
        ret = esp8266_init();
        if(ret != SUCCESS_RET)
        {
            LOG_ERROR("esp8266 init fail!\n");
            goto end;
        }
        
    }

    ret = at_client_tcp_init(p_client, link_num);
    if(ret != SUCCESS_RET)
    {
        LOG_ERROR("module tcp buffer init fail!\n");
        goto end;
    }


    /* 建立TCP链接 */
    resp->custom_flag = true;
    ret = at_exec_cmd(resp, at_command, 0,  "AT+CIPSTART=%d,\"TCP\",\"%s\",%d\r\n", link_num, pNetwork->pHostAddress, pNetwork->port);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("build TCP link fail!\n");
        goto end;
    }
    else
    {
        esp8266_link[link_num] = eCONNECTED;
        /* handle can't be zero */
        pNetwork->handle = link_num + 1;
    }

    /* 设置TCP连接的数据接收方式为被动模式 */
    resp->custom_flag = false;
    ret = at_exec_cmd(resp, at_command, 0,  "AT+CIPRECVMODE=1\r\n");
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("set tcp receive data mode fail!\n");
        goto end;
    }

end:    
    at_delete_resp(resp);
    return ret;
}



