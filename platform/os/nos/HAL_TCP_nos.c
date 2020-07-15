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

#include "netdb.h"
#include "sockets.h"
#include "uiot_import.h"

uintptr_t HAL_TCP_Connect(_IN_ const char *host, _IN_ uint16_t port) {
    struct addrinfo hints;
    struct addrinfo *addrInfoList = NULL;
    struct addrinfo *cur = NULL;
    int fd = 0;
    int rc = 0;
    char service[6];

    printf("establish tcp connection with server(host='%s', port=[%u])\n", host, port);

    hints.ai_family = AF_INET; /* only IPv4 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;    
    sprintf(service, "%u", port);

    if ((rc = lwip_getaddrinfo(host, service, &hints, &addrInfoList)) != 0) {
        printf("getaddrinfo error(%d), host = '%s', port = [%d]\n", rc, host, port);
        return (uintptr_t) (-1);
    }

    for (cur = addrInfoList; cur != NULL; cur = cur->ai_next) {
        if (cur->ai_family != AF_INET) {
            printf("socket type error\n");
            rc = -1;
            continue;
        }

        fd = lwip_socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
        if (fd < 0) {
            printf("create socket error\n");
            rc = -1;
            continue;
        }

        if (lwip_connect(fd, cur->ai_addr, cur->ai_addrlen) == 0) {
            rc = fd;
            break;
        }

        close(fd);
        printf("connect error\n");
        rc = -1;
    }

    if (-1 == rc) {
        printf("fail to establish tcp\n");
    } else {
        printf("success to establish tcp, fd=%d\n", rc);
    }
    lwip_freeaddrinfo(addrInfoList);

    return (uintptr_t) rc;
}


int32_t HAL_TCP_Disconnect(_IN_ uintptr_t fd) {
    int rc;

    /* Shutdown both send and receive operations. */
    rc = lwip_shutdown((int) fd, 2);
    if (0 != rc) {
        printf("shutdown error\n");
        return FAILURE_RET;
    }

    rc = lwip_close((int) fd);
    if (0 != rc) {
        printf("close socket error\n");
        return FAILURE_RET;
    }

    return SUCCESS_RET;
}


int32_t HAL_TCP_Write(_IN_ uintptr_t fd, _IN_ unsigned char *buf, _IN_ size_t len, _IN_ uint32_t timeout_ms) {
    int ret,tcp_fd;
    size_t len_sent;
    IoT_Error_t net_err = SUCCESS_RET;
    fd_set sets;
    struct timeval tv;

    len_sent = 0;
    ret = 1; /* send one time if timeout_ms is value 0 */

    if (fd >= FD_SETSIZE) {
        return -1;
    }
    tcp_fd = (int)fd;

    do {
        FD_ZERO(&sets);
        FD_SET(tcp_fd, &sets);
    
        tv.tv_sec = 0;
        tv.tv_usec = timeout_ms * 1000;
        ret = select(tcp_fd + 1, NULL, &sets, NULL, &tv);
        if (ret > 0) {
            if (0 == FD_ISSET(tcp_fd, &sets)) {
                printf("Should NOT arrive\n");
                /* If timeout in next loop, it will not sent any data */
                ret = 0;
                continue;
            }
        } else if (0 == ret) {
            printf("select-write timeout %d\n", tcp_fd);
            break;
        } else {
            if (EINTR == errno) {
                printf("EINTR be caught\n");
                continue;
            }

            printf("select-write fail, ret = select() = %d\n", ret);
            net_err = ERR_TCP_WRITE_FAILED;
            break;
        }
        

        if (ret > 0) {
            ret = lwip_send(tcp_fd, buf + len_sent, len - len_sent, 0);
            if (ret > 0) {
                len_sent += ret;
            } else if (0 == ret) {
                printf("No data be sent\n");
            } else {
                if (EINTR == errno) {
                    printf("EINTR be caught\n");
                    continue;
                }

                printf("send fail, ret = send() = %d\n", ret);
                net_err = ERR_TCP_WRITE_FAILED;
                break;
            }
        }
    } while (!net_err && (len_sent < len));

    return net_err != SUCCESS_RET ? net_err : len_sent;
}


int32_t HAL_TCP_Read(_IN_ uintptr_t fd, _OU_ unsigned char *buf, _IN_ size_t len, _IN_ uint32_t timeout_ms) {
    int tcp_fd;
    IoT_Error_t err_code;
    size_t len_recv;
    fd_set sets;
    struct timeval tv;

    len_recv = 0;
    err_code = SUCCESS_RET;

    if (fd >= FD_SETSIZE) {
        return FAILURE_RET;
    }
    tcp_fd = (int)fd;

    do {
        FD_ZERO(&sets);
        FD_SET(tcp_fd, &sets);

        tv.tv_sec = 0;
        tv.tv_usec = timeout_ms * 1000;
        int ret = lwip_select(tcp_fd + 1, &sets, NULL, NULL, &tv);
        if (ret > 0) {
            ret = lwip_recv(tcp_fd, buf + len_recv, len - len_recv, 0);
            if (ret > 0) {
                len_recv += ret;
            } else if (0 == ret) {
                printf("connection is closed\n");
                err_code = ERR_TCP_PEER_SHUTDOWN;
                break;
            } else {
                if (EINTR == errno) {
                    continue;
                }
                printf("recv fail\n");
                err_code = ERR_TCP_READ_FAILED;
                break;
            }
        } else if (0 == ret) {
            break;
        } else {
            if (EINTR == errno) {
                continue;
            }
            printf("select-recv fail\n");
            err_code = ERR_TCP_READ_FAILED;
            break;
        }
    } while ((len_recv < len));

    return (0 != len_recv) ? len_recv : err_code;
}
