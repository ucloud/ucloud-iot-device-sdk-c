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
#include "at_client.h"
#include "utils_timer.h"

char ip_addr[20] = {0};

static int urc_cpin_recv_judge(const char *data, uint32_t size)
{
    if(NULL != strstr(data, "READY"))
    {
        return SUCCESS_RET;
    }
    else
    {
        return FAILURE_RET;
    }
}

static void urc_common_recv_func(const char *data, uint32_t size)
{
    return;
}

static int urc_ip_recv_judge(const char *data, uint32_t size)
{
    int num1,num2,num3,num4;
    if((data[size-1] == '\n')
        && (4 == sscanf(data,"%d.%d.%d.%d",&num1,&num2,&num3,&num4)))
    {   
        if(0<=num1 && num1<=255
         && 0<=num2 && num2<=255
         && 0<=num3 && num3<=255
         && 0<=num4 && num4<=255) 
        {
            return SUCCESS_RET;
        }
    }
    else
    {
        return FAILURE_RET;
    }
}

static void urc_ip_recv_func(const char *data, uint32_t size)
{
    strncpy(ip_addr, data, size);
    return;
}

static int urc_tcp_start_judge(const char *data, uint32_t size)
{
    if(0 == strncmp("CONNECT OK", data, 10))
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
        return SUCCESS_RET;
}

static at_custom custom_table[] = {
    {"AT+CPIN?", 14, urc_cpin_recv_judge, urc_common_recv_func},
    {"AT+CIFSR", 10, urc_ip_recv_judge, urc_ip_recv_func},
    {"AT+CIPSTART", 10, urc_tcp_start_judge, urc_common_recv_func},
    {"AT+CIPSEND", 1, urc_send_recv_judge, urc_common_recv_func},
};

IoT_Error_t module_init()
{
	IoT_Error_t ret;
	at_client_t p_client;

	p_client = at_client_get();

	if(NULL == p_client)
	{
		LOG_ERROR("no at client get");
		ret = FAILURE_RET;
		goto exit; 
	}

	if(AT_STATUS_INITIALIZED == p_client->status)
	{
		LOG_ERROR("at client has been initialized");
		ret = FAILURE_RET;
		goto exit;
	}
	
	
    /* initialize AT client */
    ret = at_client_init(p_client);
	if(SUCCESS_RET != ret)
 	{
		LOG_ERROR("at client init fail,ret:%d", ret);
		goto exit;
	}
	else
	{
		LOG_DEBUG("at client init success");
	}

    /* register URC data execution function  */
    at_set_urc_table(p_client, custom_table, sizeof(custom_table) / sizeof(custom_table[0]));

exit:

    return ret;
}


