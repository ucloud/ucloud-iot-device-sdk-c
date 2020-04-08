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

extern at_custom custom_table[];
extern int custom_table_num;

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
    at_set_urc_table(p_client, custom_table, custom_table_num);

exit:

    return ret;
}


