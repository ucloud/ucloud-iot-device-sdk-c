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
//#include "stm32f7xx_hal.h"
#include "utils_net.h"
#include "at_ringbuff.h"
#include "at_client.h"
#include "uiot_import.h"

//extern UART_HandleTypeDef huart2;
//static UART_HandleTypeDef *pAtUart = &huart2;
//extern sRingbuff g_ring_tcp_buff[];    

void HAL_AT_Init()
{
    /* 配置串口接收buf的存储位置 */    
    //HAL_UART_Receive_IT(pAtUart, g_ring_buff.buffer, 1);
    return;
}


int HAL_AT_Read(_IN_ utils_network_pt pNetwork, _OU_ unsigned char *buffer, _IN_ size_t len)
{
    //return ring_buff_pop_data(&(g_ring_tcp_buff[pNetwork->handle-1]), buffer, len);
    return 0;
}

int HAL_AT_Write(_IN_ unsigned char *buffer, _IN_ size_t len)
{   
    //return HAL_UART_Transmit_IT(pAtUart, buffer, len);
    return 0;
}

