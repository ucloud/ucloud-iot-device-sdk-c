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

#define EC20_MAX_TCP_LINK        3

extern sRingbuff g_ring_buff;    
extern sRingbuff g_ring_tcp_buff[3];    
int ec20_link[EC20_MAX_TCP_LINK] = {0};
int ec20_recv_len[EC20_MAX_TCP_LINK] = {0};

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
    at_exec_cmd(resp, at_command, 0, "AT+QIRD=%d,0\r\n", pNetwork->handle-1); 

    if(ec20_recv_len[pNetwork->handle-1] >= len)
    {
        resp->custom_flag = true;
        at_exec_cmd(resp, at_command, 0, "AT+QIRD=%d,%d\r\n", pNetwork->handle-1, len); 
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
    int ret = 0;
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE_RET;
    }

    at_response_t resp = NULL;
    
    resp = at_create_resp(2048, 0, CMD_TIMEOUT_MS);
    if (resp == NULL)
    {
        LOG_ERROR("No memory for response object!");
        return FAILURE_RET;
    }

    resp->custom_flag = true;
    at_exec_cmd(resp, at_command, 0, "AT+QISEND=%d,%d\r\n", pNetwork->handle-1, len); 
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
    if(ec20_link[pNetwork->handle-1] == eCONNECTED)
    {
        at_response_t resp = NULL;
        
        resp = at_create_resp(256, 0, CMD_TIMEOUT_MS);
        if (resp == NULL)
        {
            LOG_ERROR("No memory for response object!");
            return FAILURE_RET;
        }
    
        resp->custom_flag = false;
        ret = at_exec_cmd(resp, at_command, 0,  "AT+QICLOSE=%d\r\n",pNetwork->handle-1);
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("close TCP link fail!\n");
        }
        else
        {
            ec20_link[pNetwork->handle-1] = eDISCONNECTED;
        }
        at_delete_resp(resp);
    }
    return ret;
}

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
        if((1 == stat) || (5 == stat))
        {
            return SUCCESS_RET;
        }
    }
    return FAILURE_RET;

}


static int urc_cgreg_recv_judge(const char *data, uint32_t size)
{
    if(NULL != strstr(data, "+CGREG"))
    {
        return SUCCESS_RET;
    }
    else
    {
        return FAILURE_RET;
    }
}

static int urc_cgreg_recv_func(const char *data, uint32_t size)
{
    int n = 0;
    int stat = 0;
    if(2 == sscanf(data,"+CGREG: %d,%d",&n, &stat))
    {
        if((1 == stat) || (5 == stat))
        {
            return SUCCESS_RET;
        }
    }
    
    return FAILURE_RET;
}


static int urc_common_recv_func(const char *data, uint32_t size)
{
    return SUCCESS_RET;
}

static int urc_cereg_recv_judge(const char *data, uint32_t size)
{
    if(NULL != strstr(data, "+CEREG:"))
    {
        return SUCCESS_RET;
    }
    else
    {
        return FAILURE_RET;
    }
}

static int urc_cereg_recv_func(const char *data, uint32_t size)
{
    int n = 0;
    int stat = 0;
    if(2 == sscanf(data,"+CEREG: %d,%d\r\n",&n,&stat))
    {
        if((1 == stat) || (5 == stat))
        {
            return SUCCESS_RET;
        }
    }
    
    return FAILURE_RET;
}

static char cops[20];
static int urc_cops_recv_judge(const char *data, uint32_t size)
{
    at_client_t client = at_client_get();
    int mode;
    int format;
    int act;
    if((data[size-1] == '\n')&& (sscanf(data,"+COPS: %d,%d,%s,%d\r\n", &mode, &format, cops, &act)))
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

static int urc_tcp_start_judge(const char *data, uint32_t size)
{
    int connect_id = 0;
    int err = 0;
    if(2 == sscanf(data,"+QIOPEN:%d,%d\r\n",&connect_id,&err))
    {
        if(0 == err)
        {
            return SUCCESS_RET;
        }
    }
    
    return FAILURE_RET;

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

static int urc_qird_recv_judge(const char *data, uint32_t size)
{
    if(0 == strncmp(data, "+QIRD:", size))
    {
        return SUCCESS_RET;
    }
    else 
    {
        return FAILURE_RET;
    }
}

static int urc_qird_recv_func(const char *data, uint32_t size)
{
    at_client_t client = at_client_get();
    char temp_string[10] = {0};
    char temp_char = 0;
    int loop = 0;
    int ret = 0;
    int recv_data_num = 0;
    int actual_len = 0;
    int link_num = 0;
    const char *cmd = NULL;
    int cmdsize = 0;
    int total_len = 0;
    int read_len = 0;
    int unread_len = 0;
    
    do
    {
        ret = at_client_getchar(client, &temp_string[loop], GET_RECEIVE_TIMEOUT_MS);
        loop++;
    }while(temp_string[loop-1] != '\n');
    
    cmd = (const char *)at_get_last_cmd(&cmdsize);

    //读取收到的数据
    if(2 == sscanf(cmd,"AT+QIRD=%d,%d\r\n",&link_num,&recv_data_num))
    {
        if(recv_data_num > 0)
        {
            if(1 == sscanf(temp_string," %d\r\n",&actual_len))
            {   
                if(0 == actual_len)
                    return SUCCESS_RET;
            }
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
        else            //查询收到的数据长度
        {
            if(3 == sscanf(temp_string," %d,%d,%d\r\n", &total_len, &read_len, &unread_len))
            {   
                ec20_recv_len[link_num] = unread_len;
            }
        }
    } 
    
    return SUCCESS_RET;
}



at_custom custom_table[] = {
    {"AT+CPIN?", 12, urc_cpin_recv_judge, urc_cpin_recv_func},
    {"AT+CSQ", 10, urc_csq_recv_judge, urc_csq_recv_func},
    {"AT+CREG?", 10, urc_creg_recv_judge, urc_creg_recv_func},
    {"AT+CGREG?", 11, urc_cgreg_recv_judge, urc_cgreg_recv_func},
    {"AT+CEREG?", 12, urc_cereg_recv_judge, urc_cereg_recv_func},
    {"AT+COPS?", 20, urc_cops_recv_judge, urc_common_recv_func},
    {"AT+QIOPEN", 12, urc_tcp_start_judge, urc_common_recv_func},
    {"AT+QISEND", 1, urc_send_recv_judge, urc_common_recv_func},
    {"AT+QIRD", 6, urc_qird_recv_judge, urc_qird_recv_func},
};

int custom_table_num = sizeof(custom_table) / sizeof(custom_table[0]);

static int ec20_init()
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
    HAL_SleepMs(5000);

    /* 去掉串口回显 */
    resp->custom_flag = false;
    ret = at_exec_cmd(resp, at_command, 0, "ATE0\r\n");
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
        ret |= at_exec_cmd(resp, at_command, 0,  "AT+CPIN?\r\n");  
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("please check SIM card status!\n");
            continue;
        }
        
        /* 检查网络强度 */  
        resp->custom_flag = true;
        ret |= at_exec_cmd(resp, at_command, 0,  "AT+CSQ\r\n");  
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("bad signal!\n");
            continue;
        }
        
        /* 检查网络注册状态 */   
        resp->custom_flag = true;   
        ret |= at_exec_cmd(resp, at_command, 0,  "AT+CREG?\r\n"); 
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("not network regist!\n");
            continue;
        }

        /*检查GPRS附着状态*/
        resp->custom_flag = true;   
        ret |= at_exec_cmd(resp, at_command, 0,  "AT+CGREG?\r\n"); 
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("not Attach GPRS!\n");
            continue;
        }

        /* check TCPIP mode is set */
        resp->custom_flag = true;
        ret = at_exec_cmd(resp, at_command, 0,  "AT+CEREG?\r\n");
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("check TCPIP mode fail!\n");
            goto end;
        }

        /* query current Network Operator */
        resp->custom_flag = true;
        ret = at_exec_cmd(resp, at_command, 0,  "AT+COPS?\r\n");
        if(SUCCESS_RET != ret)
        {
            LOG_ERROR("query current Network Operator fail!\n");
            goto end;
        }

        if(0 == strncmp(cops, "\"CHN-UNICOM\"", strlen("\"CHN-UNICOM\"")))
        {
            resp->custom_flag = false;
            ret = at_exec_cmd(resp, at_command, 0, "AT+QICSGP=1,1,\"UNINET\",\"\",\"\",0\r\n");
            if(SUCCESS_RET != ret)
            {
                LOG_ERROR("AT+QICSGP CHN-UNICOM fail!\n");
                goto end;
            }
        }
        else if(0 == strncmp(cops, "\"CHINA MOBILE\"", strlen("\"CHINA MOBILE\"")))
        {
            resp->custom_flag = false;
            ret = at_exec_cmd(resp, at_command, 0, "AT+QICSGP=1,1,\"CMNET\",\"\",\"\",0\r\n");
            if(SUCCESS_RET != ret)
            {
                LOG_ERROR("AT+QICSGP CHINA MOBILE fail!\n");
                goto end;
            }       
        }
        else if(0 == strncmp(cops, "\"CHN-CT\"", strlen("\"CHN-CT\"")))
        {
            resp->custom_flag = false;
            ret = at_exec_cmd(resp, at_command, 0, "AT+QICSGP=1,1,\"CTNET\",\"\",\"\",0\r\n");
            if(SUCCESS_RET != ret)
            {
                LOG_ERROR("AT+QICSGP CHN-CT fail!\n");
                goto end;
            }   
        }
        else
        {
            ret = ERR_PARAM_INVALID;
            goto end;
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

    /* Enable automatic time zone update via NITZ and update LOCAL time to RTC */
    resp->custom_flag = false;
    ret = at_exec_cmd(resp, at_command, 0,  "AT+CTZU=3\r\n");
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("Enable automatic time zone update via NITZ and update LOCAL time to RTC fail!\n");
        goto end;
    }

    /* Deactivate context profile */    
    resp->custom_flag = false;
    ret = at_exec_cmd(resp, at_command, 0,  "AT+QIDEACT=1\r\n");
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("Deactivate context profile fail!\n");
        goto end;
    }

    /* Activate context profile */    
    resp->custom_flag = false;
    ret = at_exec_cmd(resp, at_command, 0,  "AT+QIACT=1\r\n");
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("Activate context profile fail!\n");
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

    for(link_num = 0; link_num < EC20_MAX_TCP_LINK; link_num++)
    {
        if(ec20_link[link_num] == eDISCONNECTED)
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
        
        ret = ec20_init();
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
    ret = at_exec_cmd(resp, at_command, 0,  "AT+QIOPEN=1,%d,\"TCP\",\"%s\",%d,0,0\r\n", link_num, pNetwork->pHostAddress, pNetwork->port);
    
    if(SUCCESS_RET != ret)
    {
        LOG_ERROR("build TCP link fail!\n");
        goto end;
    }
    else
    {
        ec20_link[link_num] = eCONNECTED;
        /* handle can't be zero */
        pNetwork->handle = link_num + 1;
    }
end:
    at_delete_resp(resp);
    return ret;
}


