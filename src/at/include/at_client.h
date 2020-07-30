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

  
#ifndef __AT_CLIENT_H__
#define __AT_CLIENT_H__

#include "at_ringbuff.h"
#include "at_inf.h"


#ifdef __cplusplus
extern "C" {
#endif

#define AT_CMD_MAX_LEN                 1024
#define RING_BUFF_LEN                  6144     

#define GET_CHAR_TIMEOUT_MS            100
#define CMD_TIMEOUT_MS                 10000
#define CMD_RESPONSE_INTERVAL_MS       1000
#define GET_RECEIVE_TIMEOUT_MS         1000


#define AT_RESP_END_OK                 "OK"
#define AT_RESP_END_ERROR              "ERROR"
#define AT_RESP_END_FAIL               "FAIL"
#define AT_RESP_END_SEND1              "DATA ACCEPT:"
#define AT_RESP_END_SEND2              "Recv "
#define AT_RESP_END_SEND3              "SEND OK"
#define AT_END_CR_LF                   "\r\n"

typedef void (*ParserFunc)(void *userContex);

typedef enum 
{
    AT_STATUS_UNINITIALIZED = 0,
    AT_STATUS_INITIALIZED,
    AT_STATUS_BUSY,
}at_status;


enum at_resp_status
{
     AT_RESP_OK = 0,                   /* AT response end is OK */
     AT_RESP_ERROR = -1,               /* AT response end is ERROR */
     AT_RESP_TIMEOUT = -2,             /* AT response is timeout */
     AT_RESP_BUFF_FULL= -3,            /* AT response buffer is full */
};
typedef enum at_resp_status at_resp_status_t;

typedef struct _at_response_
{
    /* response buffer */
    char *buf;
    /* the maximum response buffer size */
    int buf_size;
    /* the number of setting response lines
     * == 0: the response data will auto return when received 'OK' or 'ERROR'
     * != 0: the response data will return when received setting lines number data */
    int line_num;
    /* the count of received response lines */
    int line_counts;
    bool custom_flag;
    /* the maximum response time */
    uint32_t timeout;
}at_response;


typedef  at_response * at_response_t;

typedef struct _at_custom_
{
    const char *cmd;
    const int minlen;
    int (*judge)(const char *data, uint32_t size);
    int (*func)(const char *data, uint32_t size);
}at_custom;

typedef at_custom *at_custom_t;

typedef struct _at_client_
{
    at_status status;
    char end_sign;
    
    ring_buff_t pRingBuff;
    ring_buff_t pRingTcpBuff[3];
    char *recv_buffer;
    uint32_t recv_bufsz;
    uint32_t cur_recv_len;
    void *lock;      //pre cmd take the lock wait for resp , another cmd need wait for unlock
    
    at_response_t resp;
    at_resp_status_t resp_status;
    bool resp_notice;

    const at_custom *urc_table;
    uint16_t urc_table_size;
}at_client;

typedef at_client *at_client_t;

typedef enum
{
    at_command = 0,
    at_data,
}at_data_type;

/* AT client tcp buffer init */
IoT_Error_t at_client_tcp_init(at_client_t client, int link_num);

/* AT client initialize and start*/
IoT_Error_t at_client_init(at_client_t pClient);

/* get AT client handle*/
at_client_t at_client_get(void);

/* ========================== multiple AT client function ============================ */
/* set AT client a line end sign */
void at_set_end_sign(char ch);

/* Set URC(Unsolicited Result Code) table */
void at_set_urc_table(at_client_t client, const at_custom_t table, uint32_t size);

/* AT client send commands to AT server and waiter response */
int at_client_send(at_client_t client, char *buf, int size);

IoT_Error_t at_obj_exec_cmd(at_response_t resp, at_data_type data_type, size_t data_size, const char *cmd_expr, ...);
#define at_exec_cmd(resp, data_type, data_size, ...)                   at_obj_exec_cmd(resp, data_type, data_size, __VA_ARGS__)

/* AT response object create and delete */
at_response_t at_create_resp(uint32_t buf_size, uint32_t line_num, uint32_t timeout);
void at_delete_resp(at_response_t resp);

int at_recv_readline(at_client_t client);
IoT_Error_t at_client_getchar(at_client_t client, char *pch, uint32_t timeout);

/* ========================== single AT client function ============================ */
#ifdef __cplusplus
}
#endif

#endif /* __AT_H__ */

