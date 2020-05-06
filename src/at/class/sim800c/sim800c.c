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

#define SIM800C_MAX_TCP_LINK        3
extern sRingbuff g_ring_buff;    
extern sRingbuff g_ring_tcp_buff[3];    
int sim800c_link[SIM800C_MAX_TCP_LINK] = {0};
int HAL_AT_Read_Tcp(_IN_ utils_network_pt pNetwork, _IN_ unsigned char *buffer, _IN_ size_t len)
{
    int ret = 0;
    at_client_t client = at_client_get();
    char last_char = 0;
    char temp_char = 0;
    int temp_read_point = client->pRingBuff->readpoint;
    char temp_string[21] = {0};
    int rec_num = 0;
    int link_num = 0;

    /* clear \r\n */
    ret = at_client_getchar(client, &last_char, GET_RECEIVE_TIMEOUT_MS);
    if(last_char == '\r')
    {
        while(1)
        {
            ret = at_client_getchar(client, &temp_char, GET_RECEIVE_TIMEOUT_MS);
            if(temp_char == '\n')
            {
                temp_read_point = client->pRingBuff->readpoint;
                ret = at_client_getchar(client, &temp_char, GET_RECEIVE_TIMEOUT_MS);
                break;
            }
            else
            {   
                break;
            }

        }
        
        if(temp_char == '+')
        {
            int loop = 0;
            do
            {
                ret = at_client_getchar(client, &temp_string[loop], GET_RECEIVE_TIMEOUT_MS);
                if(loop == 8)
                {   
                    /* divide multi tcp link */
                    if(0 != strncmp(temp_string, "RECEIVE,", 8))
                    {
                        client->pRingBuff->readpoint = temp_read_point;
                        break;
                    }
                }
                loop++;
            }while(temp_string[loop-1] != '\n');
            temp_string[loop] = '\0';
            if(2 == sscanf(temp_string,"RECEIVE,%d,%d:\r\n",&link_num,&rec_num))
            {
                for(loop = rec_num; loop > 0; loop--)
                {
                    ret = at_client_getchar(client, (char *)&temp_char, GET_RECEIVE_TIMEOUT_MS);
                    ret |= ring_buff_push_data(&(g_ring_tcp_buff[link_num]), (uint8_t *)&temp_char, 1);
                    if(SUCCESS_RET != ret)
                    {
                        LOG_ERROR("copy data to tcp buff fail\n");
                    }
                }
            }
        }

    }
    else
    {
        client->pRingBuff->readpoint = temp_read_point;
    }


    ret = HAL_AT_Read(pNetwork, buffer, len);

    return ret;
}


int HAL_AT_Write_Tcp(_IN_ utils_network_pt pNetwork, _IN_ unsigned char *buffer, _IN_ size_t len)
{
    int ret = 0;
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    at_response_t resp = NULL;
    
    resp = at_create_resp(256, 0, CMD_TIMEOUT_MS);
    if (resp == NULL)
    {
        LOG_ERROR("No memory for response object!");
        return FAILURE_RET;
    }

    resp->custom_flag = true;
    at_exec_cmd(resp, at_command, 0, "AT+CIPSEND=%d,%d\r\n", pNetwork->handle-1, len); 
    resp->custom_flag = false;
    ret = at_exec_cmd(resp, at_data, len, (const char *)buffer); 
    HAL_SleepMs(100);

    at_delete_resp(resp);
    if(ret == SUCCESS_RET)
    {
        return len;
    }
    else
    {
        return 0;
    }
}


int HAL_AT_TCP_Disconnect(utils_network_pt pNetwork)
{
    int ret = SUCCESS_RET;
    
    /* 断开无线链接 */
    if(sim800c_link[pNetwork->handle-1] == eCONNECTED)
    {
        at_response_t resp = NULL;
        
        resp = at_create_resp(256, 0, CMD_TIMEOUT_MS);
        if (resp == NULL)
        {
            LOG_ERROR("No memory for response object!");
            return FAILURE_RET;
        }
    
        resp->custom_flag = false;
        ret = at_exec_cmd(resp, at_command, 0,  "AT+CIPCLOSE=%d\r",pNetwork->handle-1);
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("close TCP link fail!\n");
        }
        else
        {
            sim800c_link[pNetwork->handle-1] = eDISCONNECTED;
        }
        at_delete_resp(resp);
    }
    return ret;
}

char ip_addr[20] = {0};

static int urc_cpin_recv_judge(const char *data, uint32_t size)
{
    if(NULL != strstr(data, "+CPIN"))
    {
        return SUCCESS_RET;
    }
    else
    {
        return FAILURE_RET;
    }
}

static int urc_cpin_recv_func(const char *data, uint32_t size)
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

static int urc_csq_recv_judge(const char *data, uint32_t size)
{
    if(NULL != strstr(data, "+CSQ"))
    {
        return SUCCESS_RET;
    }
    else
    {
        return FAILURE_RET;
    }
}

static int urc_csq_recv_func(const char *data, uint32_t size)
{
    int rssi = 0;
    int ber = 0;
    if(2 == sscanf(data,"+CSQ: %d,%d", &rssi, &ber))
    {
        if((99 != rssi) && (99 != ber))
        {
            return SUCCESS_RET;
        }
    }
    return FAILURE_RET;

}

static int urc_creg_recv_judge(const char *data, uint32_t size)
{
    if(NULL != strstr(data, "+CREG"))
    {
        return SUCCESS_RET;
    }
    else
    {
        return FAILURE_RET;
    }
}

static int urc_creg_recv_func(const char *data, uint32_t size)
{
    int n = 0;
    int stat = 0;
    if(2 == sscanf(data,"+CREG: %d,%d", &n, &stat))
    {
        if(1 == stat)
        {
            return SUCCESS_RET;
        }
    }
    return FAILURE_RET;

}


static int urc_cgatt_recv_judge(const char *data, uint32_t size)
{
    if((data[size-1] == '\n')
        && (NULL != strstr(data, "+CGATT")))
    {
        return SUCCESS_RET;
    }
    else
    {
        return FAILURE_RET;
    }
}

static int urc_cgatt_recv_func(const char *data, uint32_t size)
{
    at_client_t client = at_client_get();
    int result = 0;
    if(1 == sscanf(data,"+CGATT: %d",&result))
    {
        if(1 == result)
        {
            at_recv_readline(client);
            at_recv_readline(client);
            return SUCCESS_RET;
        }
    }
    
    return FAILURE_RET;
}


static int urc_common_recv_func(const char *data, uint32_t size)
{
    return SUCCESS_RET;
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

    return FAILURE_RET;   
}

static int urc_ip_recv_func(const char *data, uint32_t size)
{
    strncpy(ip_addr, data, size);
    return SUCCESS_RET;
}

static int urc_tcp_start_judge(const char *data, uint32_t size)
{
    if(NULL != strstr(data, "CONNECT OK\r\n"))
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
    if(NULL != strstr(data, ">"))
    {
        return SUCCESS_RET;
    }
    else 
    {
        return FAILURE_RET;
    }
}

static int urc_close_recv_judge(const char *data, uint32_t size)
{
    if(NULL != strstr(data, "CLOSE OK"))
    {
        return SUCCESS_RET;
    }
    else 
    {
        return FAILURE_RET;
    }
}


at_custom custom_table[] = {
    {"AT+CPIN?", 12, urc_cpin_recv_judge, urc_cpin_recv_func},
    {"AT+CSQ", 10, urc_csq_recv_judge, urc_csq_recv_func},
    {"AT+CREG?", 10, urc_creg_recv_judge, urc_creg_recv_func},
    {"AT+CGATT?", 9, urc_cgatt_recv_judge, urc_cgatt_recv_func},
    {"AT+CIFSR", 10, urc_ip_recv_judge, urc_ip_recv_func},
    {"AT+CIPSTART", 14, urc_tcp_start_judge, urc_common_recv_func},
    {"AT+CIPSEND", 1, urc_send_recv_judge, urc_common_recv_func},
    {"AT+CIPCLOSE", 12, urc_close_recv_judge, urc_common_recv_func},
};

int custom_table_num = sizeof(custom_table) / sizeof(custom_table[0]);

static int sim800c_init()
{
    int ret = 0;
    at_response_t resp = NULL;
    int retry_time = 0;

    HAL_AT_Init();

    resp = at_create_resp(256, 0, CMD_TIMEOUT_MS);
    if (resp == NULL)
    {
        LOG_ERROR("No memory for response object!");
        return FAILURE_RET;
    }

    //延时等待模块启动
    HAL_SleepMs(8000);

    /* 去掉串口回显 */
    resp->custom_flag = false;
    ret = at_exec_cmd(resp, at_command, 0, "ATE0\r");
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("ATE0 set fail!\n");
        goto end;
    }

    for(retry_time = 0; retry_time < 10; retry_time++)
    {
        //延时等待模块启动
        HAL_SleepMs(2000);

        ret = SUCCESS_RET;
        
        /* 检查SIM卡状态 */
        resp->custom_flag = true;
        ret |= at_exec_cmd(resp, at_command, 0,  "AT+CPIN?\r");  
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("please check SIM card status!\n");
            continue;
        }
        
        /* 检查网络强度 */  
        resp->custom_flag = true;
        ret |= at_exec_cmd(resp, at_command, 0,  "AT+CSQ\r");  
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("bad signal!\n");
            continue;
        }
        
        /* 检查网络注册状态 */   
        resp->custom_flag = true;   
        ret |= at_exec_cmd(resp, at_command, 0,  "AT+CREG?\r"); 
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("not network regist!\n");
            continue;
        }

        /*检查GPRS附着状态*/
        resp->custom_flag = true;   
        ret |= at_exec_cmd(resp, at_command, 0,  "AT+CGATT?\r"); 
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("not Attach GPRS!\n");
            continue;
        }

        if(SUCCESS_RET == ret)
        {
            break;
        }
        
    }

    if(retry_time == 10)
    {
        LOG_ERROR("sim800c init fail!\n");
        goto end;
    }

    /* 设置多链路模式 */ 
    resp->custom_flag = false;
    ret = at_exec_cmd(resp, at_command, 0,  "AT+CIPMUX=1\r");
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("set multi link mode fail!\n");
        goto end;
    }
    
    /* 设置APN */    
    resp->custom_flag = false;
    ret = at_exec_cmd(resp, at_command, 0,  "AT+CSTT=\"3GNET\"\r");
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("set APN fail!\n");
        goto end;
    }
    
    /* 建立无线链接 */ 
    resp->custom_flag = false;
    ret = at_exec_cmd(resp, at_command, 0,  "AT+CIICR\r");
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("build wireless link fail!\n");
        goto end;
    }
    
    /* 获取本地IP地址 */  
    resp->custom_flag = true;
    ret = at_exec_cmd(resp, at_command, 0,  "AT+CIFSR\r");
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("fetch local IP address fail!\n");
        goto end;
    }

    /* 快速发送模式 */  
    resp->custom_flag = false;
    ret = at_exec_cmd(resp, at_command, 0,  "AT+CIPQSEND=1\r");
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("fast send mode fail!\n");
        goto end;
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

    for(link_num = 0; link_num < SIM800C_MAX_TCP_LINK; link_num++)
    {
        if(sim800c_link[link_num] == eDISCONNECTED)
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
        
        ret = sim800c_init();
        if(ret != SUCCESS_RET)
        {
            LOG_ERROR("sim800c init fail!\n");
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
    ret = at_exec_cmd(resp, at_command, 0,  "AT+CIPSTART=%d,\"TCP\",\"%s\",\"%d\"\r", link_num, pNetwork->pHostAddress, pNetwork->port);
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("build TCP link fail!\n");
        goto end;
    }
    else
    {
        sim800c_link[link_num] = eCONNECTED;
        /* handle can't be zero */
        pNetwork->handle = link_num + 1;
    }
end:
    at_delete_resp(resp);
    return ret;
}



