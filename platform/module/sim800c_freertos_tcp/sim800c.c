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
#include "stm32f7xx_hal.h"
#include "at_inf.h"
#include "utils_net.h"
#include "at_ringbuff.h"
#include "at_client.h"
#include "uiot_import.h"

extern UART_HandleTypeDef huart2;
static UART_HandleTypeDef *pAtUart = &huart2;
extern sRingbuff g_ring_buff;	

int HAL_AT_Read(_IN_ void * pNetwork, _OU_ unsigned char *buffer, _IN_ size_t len)
{
    return ring_buff_pop_data(&g_ring_buff, buffer, len);
}

int HAL_AT_Write(_IN_ unsigned char *buffer, _IN_ size_t len)
{   
    return HAL_UART_Transmit_IT(pAtUart, buffer, len);
}

int HAL_AT_TCP_Disconnect()
{
    int ret = 0;
	at_response_t resp = NULL;
    
	resp = at_create_resp(256, 0, CMD_TIMEOUT_MS);
	if (resp == NULL)
	{
		LOG_ERROR("No memory for response object!");
		return FAILURE_RET;
	}

    /* 断开无线链接 */    
    ret = at_exec_cmd(resp, false, at_command, 0,  "AT+CIPCLOSE\r");
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("close TCP link fail!\n");
    }
    
    at_delete_resp(resp);
    return SUCCESS_RET;
}

int HAL_AT_TCP_Connect(_IN_ void * pNetwork, _IN_ const char *host, _IN_ uint16_t port) 
{
    int ret = 0;
    utils_network_pt pNet = (utils_network_pt)pNetwork;
	at_response_t resp = NULL;
    
	resp = at_create_resp(256, 0, CMD_TIMEOUT_MS);
	if (resp == NULL)
	{
		LOG_ERROR("No memory for response object!");
		return FAILURE_RET;
	}
    ret = module_init();

    /* 配置串口接收buf的存储位置 */    
    HAL_UART_Receive_IT(pAtUart, g_ring_buff.buffer, 1);

    //延时等待模块启动
    //HAL_SleepMs(3000);

    /* 去掉串口回显 */
    ret = at_exec_cmd(resp, false, at_command, 0, "ATE0\r");
      
    /* 检查SIM卡状态 */    
    ret = at_exec_cmd(resp, false, at_command, 0,  "AT+CPIN?\r");  
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("check SIM card status!\n");
        return ret;
    }
    
    /* 检查网络强度 */    
    ret = at_exec_cmd(resp, false, at_command, 0,  "AT+CSQ\r");  
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("bad signal!\n");
        return ret;
    }
    
    /* 检查网络注册状态 */    
    ret = at_exec_cmd(resp, false, at_command, 0,  "AT+CREG?\r"); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("not network regist!\n");
        return ret;
    }

    /*检查GPRS附着状态*/
    ret = at_exec_cmd(resp, false, at_command, 0,  "AT+CGATT?\r"); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("not Attach GPRS!\n");
        return ret;
    }
    
    /* 设置APN */    
    ret = at_exec_cmd(resp, false, at_command, 0,  "AT+CSTT=\"3GNET\"\r");
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("set APN fail!\n");
        return ret;
    }
    
    /* 建立无线链接 */    
    ret = at_exec_cmd(resp, false, at_command, 0,  "AT+CIICR\r");
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("build wireless link fail!\n");
        return ret;
    }
    
    /* 获取本地IP地址 */    
    ret = at_exec_cmd(resp, true, at_command, 0,  "AT+CIFSR\r");
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("fetch local IP address!\n");
        return ret;
    }
    
    /* 建立TCP链接 */
    ret = at_exec_cmd(resp, true, at_command, 0,  "AT+CIPSTART=\"TCP\",\"%s\",\"%d\"\r", pNet->pHostAddress, pNet->port);
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("build TCP link fail!\n");
        return ret;
    }

    at_delete_resp(resp);
    return ret;
}



