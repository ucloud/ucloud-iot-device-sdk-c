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
#include <stdarg.h>

#include "uiot_internal.h"
#include "at_client.h"
#include "at_utils.h"
#include "uiot_import.h"

sRingbuff g_ring_buff;    
sRingbuff g_ring_tcp_buff[3];    
static at_client sg_at_client;

/**
 * Create response object.
 *
 * @param buf_size the maximum response buffer size
 * @param line_num the number of setting response lines
 *        = 0: the response data will auto return when received 'OK' or 'ERROR'
 *        != 0: the response data will return when received setting lines number data
 * @param timeout the maximum response time
 *
 * @return != NULL: response object
 *         = NULL: no memory
 */
at_response_t at_create_resp(uint32_t buf_size, uint32_t line_num, uint32_t timeout)
{
    at_response_t resp = NULL;

    resp = (at_response_t) HAL_Malloc(sizeof(at_response));
    if (resp == NULL)
    {
        LOG_ERROR("AT create response object failed! No memory for response object!");
        return NULL;
    }

    resp->buf = (char *) HAL_Malloc(buf_size);
    if (resp->buf == NULL)
    {
        LOG_ERROR("AT create response object failed! No memory for response buffer!");
        HAL_Free(resp);
        return NULL;
    }
    
    resp->buf_size = buf_size;
    resp->line_num = line_num;
    resp->line_counts = 0;
    resp->timeout = timeout;

    return resp;
}


/**
 * Delete and free response object.
 *
 * @param resp response object
 */
void at_delete_resp(at_response_t resp)
{
    if (resp && resp->buf)
    {
        HAL_Free(resp->buf);
    }

    if (resp)
    {
        HAL_Free(resp);
        resp = NULL;
    }
}

static const at_custom *get_urc_obj(at_client_t client)
{
    int i;
    int buf_sz;
    char *buffer = NULL;
    const char *cmd = NULL;
    int cmd_size = 0;

    if (client->urc_table == NULL)
    {
        return NULL;
    }

    buffer = client->recv_buffer;
    buf_sz = client->cur_recv_len;
    cmd = at_get_last_cmd(&cmd_size);

    for (i = 0; i < client->urc_table_size; i++)
    {
        if ((buf_sz >= client->urc_table[i].minlen)
                && (0 == strncmp(cmd, client->urc_table[i].cmd, strlen(client->urc_table[i].cmd)))
                && (SUCCESS_RET == client->urc_table[i].judge(buffer, buf_sz)))
        {
            return &client->urc_table[i];
        }
    }

    return NULL;
}

static void client_parser(void *userContex)
{
    int resp_buf_len = 0;
    const at_custom *urc = NULL;
    int line_counts = 0;
    at_client_t client = at_client_get();
    Timer timer;
    HAL_Timer_Countdown_ms(&timer, CMD_RESPONSE_INTERVAL_MS);

    do
    {
        if (at_recv_readline(client) > 0)
        {

#ifdef     AT_PRINT_RAW_CMD    
            const char *cmd = NULL;
            int cmdsize = 0;
            cmd = at_get_last_cmd(&cmdsize);
            LOG_DEBUG("last_cmd:(%.*s), readline:%s",  cmdsize, cmd, client->recv_buffer);
#endif            
            if ((urc = get_urc_obj(client)) != NULL)
            {
                /* current receive is request, try to execute related operations */
                if (urc->func != NULL)
                {
                    if(SUCCESS_RET == urc->func(client->recv_buffer, client->cur_recv_len))
                    {
                        client->resp_status = AT_RESP_OK;
                    }   
                    else
                    {
                        client->resp_status = AT_RESP_ERROR;
                    }
                }
                client->resp_notice = true;
                break;
            }
            else if (client->resp != NULL)
            {
                /* current receive is response */
                client->recv_buffer[client->cur_recv_len - 1] = '\0';
                if (resp_buf_len + client->cur_recv_len < client->resp->buf_size)
                {
                    /* copy response lines, separated by '\0' */
                    memcpy(client->resp->buf + resp_buf_len, client->recv_buffer, client->cur_recv_len);
                    resp_buf_len += client->cur_recv_len;

                    line_counts++;                    
                }
                else
                {
                    client->resp_status = AT_RESP_BUFF_FULL;
                    LOG_ERROR("Read response buffer failed. The Response buffer size is out of buffer size(%d)!", client->resp->buf_size);
                }
                /* check response result */
                if ((memcmp(client->recv_buffer, AT_RESP_END_OK, strlen(AT_RESP_END_OK)) == 0)
                        && client->resp->line_num == 0 && client->resp->custom_flag == false)
                {
                    /* get the end data by response result, return response state END_OK. */
                    client->resp_status = AT_RESP_OK;
                }
                else if (strstr(client->recv_buffer, AT_RESP_END_ERROR)
                        || (memcmp(client->recv_buffer, AT_RESP_END_FAIL, strlen(AT_RESP_END_FAIL)) == 0))
                {
                    client->resp_status = AT_RESP_ERROR;
                }
                else if ((strstr(client->recv_buffer, AT_RESP_END_SEND1)) 
                    || (strstr(client->recv_buffer, AT_RESP_END_SEND3)))
                {
                    client->resp_status = AT_RESP_OK;
                }        
                else if (line_counts == client->resp->line_num && client->resp->line_num)
                {
                    /* get the end data by response line, return response state END_OK.*/
                    client->resp_status = AT_RESP_OK;
                }
                else
                {
                    continue;
                }
                client->resp->line_counts = line_counts;

                client->resp = NULL;
                client->resp_notice = true;
                resp_buf_len = 0;
                line_counts = 0;
                break;
            }
        }        
    }while (!HAL_Timer_Expired(&timer));
    return;
}

/**
 * Send commands to AT server and wait response.
 *
 * @param client current AT client object
 * @param resp AT response object, using NULL when you don't care response
 * @param custom_flag, using true when you need custom the response
 * @param data_type
 * @param data_size, using when data_type is at_data
 * @param 
 * @param cmd_expr AT commands expression
 *
 * @return 0 : success
 *          -1 : response status error
 *          -2 : wait timeout
 */
IoT_Error_t at_obj_exec_cmd(at_response_t resp, at_data_type data_type, size_t data_size, const char *cmd_expr, ...)
{
    POINTER_VALID_CHECK(cmd_expr, ERR_PARAM_INVALID);

    va_list args;
    int cmd_size = 0;
    Timer timer;
    IoT_Error_t result = SUCCESS_RET;
    const char *cmd = NULL;
    at_client_t client = at_client_get();
    

    if (client == NULL)
    {
        LOG_ERROR("input AT Client object is NULL, please create or get AT Client object!");
        return FAILURE_RET;
    }

    HAL_MutexLock(client->lock);

    resp->line_counts = 0;
    client->resp_status = AT_RESP_OK;
    client->resp = resp;

    va_start(args, cmd_expr);
    at_vprintfln(data_type, data_size, cmd_expr, args);
    va_end(args);

    if (resp != NULL)
    {        
        HAL_Timer_Countdown_ms(&timer, resp->timeout);
        do
        {
            client_parser(NULL);
         
            if(client->resp_notice)
            {
                if (client->resp_status != AT_RESP_OK)
                {
                    cmd = at_get_last_cmd(&cmd_size);
                    LOG_ERROR("execute command (%.*s) failed!", cmd_size, cmd);
                    result = FAILURE_RET;
                    goto __exit;
                }
                break;
            }
        }while (!HAL_Timer_Expired(&timer));

        if(HAL_Timer_Expired(&timer))
        {
            cmd = at_get_last_cmd(&cmd_size);
            LOG_ERROR("execute command (%.*s) timeout %dms!", cmd_size, cmd, resp->timeout);
            client->resp_status = AT_RESP_TIMEOUT;
            result = FAILURE_RET;
        }
    }

__exit:
    client->resp = NULL;
    client->resp_notice = false;

    HAL_MutexUnlock(client->lock);
    return result;
}

IoT_Error_t at_client_getchar(at_client_t client, char *pch, uint32_t timeout)
{
    IoT_Error_t ret = SUCCESS_RET;
    Timer timer;

    HAL_Timer_Countdown_ms(&timer, timeout);
    do   
    {
        if(0 == ring_buff_pop_data(client->pRingBuff, (uint8_t *)pch, 1))
        {        
            HAL_SleepMs(10);
            continue;
        }
        else
        {
            break;
        }    
    } while (!HAL_Timer_Expired(&timer));

    if(HAL_Timer_Expired(&timer))
    {
        ret = ERR_TCP_NOTHING_TO_READ;
    }
    

    return ret;
}

/**
 * AT client receive fixed-length data.
 *
 * @param client current AT client object
 * @param buf    receive data buffer
 * @param size    receive fixed data size
 * @param timeout  receive data timeout (ms)
 *
 * @note this function can only be used in execution function of URC data
 *
 * @return >0: receive data size
 *           =0: receive failed
 */
int at_client_obj_recv(at_client_t client, char *buf, int size, int timeout)
{
    int read_idx = 0;
    IoT_Error_t result = SUCCESS_RET;
    char ch;

    POINTER_VALID_CHECK(buf, 0);

    if (client == NULL)
    {
        LOG_ERROR("input AT Client object is NULL, please create or get AT Client object!");
        return 0;
    }

    while (1)
    {
        if (read_idx < size)
        {
            result = at_client_getchar(client, &ch, timeout);
            if (result != SUCCESS_RET)
            {
                LOG_ERROR("AT Client receive failed, uart device get data error(%d)", result);
                return 0;
            }

            buf[read_idx++] = ch;
        }
        else
        {
            break;
        }
    }

#ifdef AT_PRINT_RAW_CMD
    at_print_raw_cmd("urc_recv", buf, size);
#endif

    return read_idx;
}

/**
 *    AT client set end sign.
 *
 * @param client current AT client object
 * @param ch the end sign, can not be used when it is '\0'
 */
void at_set_end_sign(char ch)
{
    at_client_t client = at_client_get();
    
    if (client == NULL)
    {
        LOG_ERROR("input AT Client object is NULL, please create or get AT Client object!");
        return;
    }

    client->end_sign = ch;
}

/**
 * set URC(Unsolicited Result Code) table
 *
 * @param client current AT client object
 * @param table URC table
 * @param size table size
 */
void at_set_urc_table(at_client_t client, const at_custom_t urc_table, uint32_t table_sz)
{
    if (client == NULL)
    {
        LOG_ERROR("input AT Client object is NULL, please create or get AT Client object!");
        return;
    }

    client->urc_table = urc_table;
    client->urc_table_size = table_sz;
}


at_client_t at_client_get(void)
{
    return &sg_at_client;
}

int at_recv_readline(at_client_t client)
{
    int read_len = 0;
    char ch = 0, last_ch = 0;
    bool is_full = false;
    IoT_Error_t ret;

    memset(client->recv_buffer, 0x00, client->recv_bufsz);
    client->cur_recv_len = 0;

    while (1)
    {
        ret = at_client_getchar(client, &ch, GET_CHAR_TIMEOUT_MS);
        if(SUCCESS_RET != ret)
        {
            return ret;
        }

        if (read_len < client->recv_bufsz)
        {
            client->recv_buffer[read_len++] = ch;
            client->cur_recv_len = read_len;
        }
        else
        {
            is_full = true;
        }

        /* is newline or custom data */
        if ((ch == '\n' && last_ch == '\r') || (client->end_sign != 0 && ch == client->end_sign)
                || get_urc_obj(client))
        {
            if (is_full)
            {
                LOG_ERROR("read line failed. The line data length is out of buffer size(%d)!", client->recv_bufsz);
                memset(client->recv_buffer, 0x00, client->recv_bufsz);
                client->cur_recv_len = 0;
                ring_buff_flush(client->pRingBuff);
                return FAILURE_RET;
            }
            break;
        }
        last_ch = ch;
    }

#ifdef AT_PRINT_RAW_CMD
    at_print_raw_cmd("recvline", client->recv_buffer, read_len);
#endif

    return read_len;
}

char ringBuff[RING_BUFF_LEN] = {0};
char ringTcp0Buff[RING_BUFF_LEN] = {0};
char ringTcp1Buff[RING_BUFF_LEN] = {0};
char recvBuff_test[RING_BUFF_LEN] = {0};

IoT_Error_t at_client_tcp_init(at_client_t client, int link_num)
{   

    if(link_num == 0)
    {
        ring_buff_init(&(g_ring_tcp_buff[link_num]), ringTcp0Buff,  RING_BUFF_LEN);
        client->pRingTcpBuff[link_num] = &(g_ring_tcp_buff[link_num]);
    }
    else if(link_num == 1)
    {
        ring_buff_init(&(g_ring_tcp_buff[link_num]), ringTcp1Buff,  RING_BUFF_LEN);
        client->pRingTcpBuff[link_num] = &(g_ring_tcp_buff[link_num]);
    }

    /*
    char * ringBuff = HAL_Malloc(RING_BUFF_LEN);
    if(NULL == ringBuff)
    {
        LOG_ERROR("malloc ringbuff err");
        return FAILURE_RET;
    }

    ring_buff_init(&(g_ring_tcp_buff[link_num]), ringBuff,  RING_BUFF_LEN);
    client->pRingTcpBuff[link_num] = &(g_ring_tcp_buff[link_num]);
    */
    return SUCCESS_RET;
}
/* initialize the client parameters */
IoT_Error_t at_client_para_init(at_client_t client)
{
    int loop = 0;
    client->status = AT_STATUS_UNINITIALIZED;

    client->lock = HAL_MutexCreate();
    if(NULL == client->lock)
    {
        LOG_ERROR("create lock err");
        return FAILURE_RET;
    }

    /*
    char * ringBuff = HAL_Malloc(RING_BUFF_LEN);
    if(NULL == ringBuff)
    {
        LOG_ERROR("malloc ringbuff err");
        return FAILURE_RET;
    }
    */
    ring_buff_init(&g_ring_buff, ringBuff,  RING_BUFF_LEN);

    /*
    char * recvBuff = HAL_Malloc(CLINET_BUFF_LEN);
    if(NULL == recvBuff)
    {
        LOG_ERROR("malloc recvbuff err");
        return FAILURE_RET;
    }
    */
    client->recv_buffer = recvBuff_test;

    client->pRingBuff = &g_ring_buff;

    for(loop = 0; loop < 3; loop++)
    {
        client->pRingTcpBuff[loop] = NULL;
    }
    
    client->recv_bufsz = RING_BUFF_LEN;    
    client->cur_recv_len = 0;

    client->resp = NULL;
    client->resp_notice = false;

    client->urc_table = NULL;
    client->urc_table_size = 0;
    
    return SUCCESS_RET;
}


/**
 * AT client initialize.
 *
 * @param pClient pinter of at client which to be inited
 * @return @see eTidResault
 */
IoT_Error_t at_client_init(at_client_t pClient)
{
    POINTER_VALID_CHECK(pClient, ERR_PARAM_INVALID);     
    IoT_Error_t result;
    
    result = at_client_para_init(pClient);
    if (result == SUCCESS_RET)
    {
        LOG_DEBUG("AT client initialize success.");
        pClient->status = AT_STATUS_INITIALIZED;
        
    }
    else
    {
        LOG_ERROR("AT client initialize failed(%d).", result);
    }

    return result;
}


