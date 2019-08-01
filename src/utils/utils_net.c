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

//based on Alibaba c-sdk
/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <string.h>
#include "uiot_defs.h"
#include "utils_net.h"
#include "uiot_import.h"


#ifdef SUPPORT_TLS

static int read_ssl(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE;
    }

    return HAL_TLS_Read((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}

static int write_ssl(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE;
    }

    return HAL_TLS_Write((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}

static int disconnect_ssl(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE;
    }

    HAL_TLS_Disconnect((uintptr_t)pNetwork->handle);
    pNetwork->handle = 0;

    return SUCCESS;
}

static int connect_ssl(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE;
    }

    if (0 != (pNetwork->handle = (intptr_t)HAL_TLS_Connect(
            pNetwork->pHostAddress,
            pNetwork->port,
            pNetwork->ca_crt,
            pNetwork->ca_crt_len))) {
        return SUCCESS;
    }
    else {
        return FAILURE;
    }
}

#else

/*** TCP connection ***/
static int read_tcp(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE;
    }

    return HAL_TCP_Read((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}


static int write_tcp(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE;
    }

    return HAL_TCP_Write((uintptr_t)pNetwork->handle, buffer, len, timeout_ms);
}

static int disconnect_tcp(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE;
    }

    HAL_TCP_Disconnect(pNetwork->handle);
    pNetwork->handle = (uintptr_t)(-1);
    return SUCCESS;
}

static int connect_tcp(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        LOG_ERROR("network is null");
        return FAILURE;
    }

    pNetwork->handle = HAL_TCP_Connect(pNetwork->pHostAddress, pNetwork->port);
    if (pNetwork->handle == (uintptr_t)(-1)) {
        return FAILURE;
    }

    return SUCCESS;
}
#endif  /* #ifdef SUPPORT_TLS */

/****** network interface ******/
int utils_net_read(utils_network_pt pNetwork, unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    int ret;
#ifdef SUPPORT_TLS
    if (NULL != pNetwork->ca_crt) {
        ret = read_ssl(pNetwork, buffer, len, timeout_ms);
    }
#else
    if (NULL == pNetwork->ca_crt) {
        ret = read_tcp(pNetwork, buffer, len, timeout_ms);
    }
#endif
    else {
        ret = FAILURE;
        LOG_ERROR("no method match!");
    }

    return ret;
}

int utils_net_write(utils_network_pt pNetwork,unsigned char *buffer, size_t len, uint32_t timeout_ms)
{
    IoT_Error_t ret;
#ifdef SUPPORT_TLS
    if (NULL != pNetwork->ca_crt) {
        ret = write_ssl(pNetwork, buffer, len, timeout_ms);
    }
#else
    if (NULL == pNetwork->ca_crt) {
        ret = write_tcp(pNetwork, buffer, len, timeout_ms);
    }
#endif

    else {
        ret = FAILURE;
        LOG_ERROR("no method match!");
    }

    return ret;
}

int utils_net_disconnect(utils_network_pt pNetwork)
{
    int ret;
#ifdef SUPPORT_TLS
    if (NULL != pNetwork->ca_crt) {
        ret = disconnect_ssl(pNetwork);
    }
#else
    if (NULL == pNetwork->ca_crt) {
        ret = disconnect_tcp(pNetwork);
    }
#endif
    else {
        ret = FAILURE;
        LOG_ERROR("no method match!");
    }

    return  ret;
}

int utils_net_connect(utils_network_pt pNetwork)
{
    int ret;
#ifdef SUPPORT_TLS
    if (NULL != pNetwork->ca_crt) {
        ret = connect_ssl(pNetwork);
    }
#else
    if (NULL == pNetwork->ca_crt) {
        ret = connect_tcp(pNetwork);
    }
#endif
    else {
        ret = FAILURE;
        LOG_ERROR("no method match!");
    }

    return ret;
}

int utils_net_init(utils_network_pt pNetwork, const char *host, uint16_t port, const char *ca_crt)
{
    if (!pNetwork || !host) {
        LOG_ERROR("parameter error! pNetwork=%p, host = %p", pNetwork, host);
        return FAILURE;
    }
    pNetwork->pHostAddress = host;
    pNetwork->port = port;
    pNetwork->ca_crt = ca_crt;

    if (NULL == ca_crt) {
        pNetwork->ca_crt_len = 0;
    } else {
        pNetwork->ca_crt_len = strlen(ca_crt);
    }

    pNetwork->handle = 0;
    pNetwork->read = utils_net_read;
    pNetwork->write = utils_net_write;
    pNetwork->disconnect = utils_net_disconnect;
    pNetwork->connect = utils_net_connect;

    return SUCCESS;
}

