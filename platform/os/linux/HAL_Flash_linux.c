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
#include "uiot_import.h"

static int _utils_parse_name(const char *url, char *name) {
    char *host_ptr = (char *) strstr(url, "://");
    uint32_t name_len = 0;
    char *path_ptr;
    char *name_ptr;

    if (host_ptr == NULL) {
        return -1; /* URL is invalid */
    }
    host_ptr += 3;

    path_ptr = strchr(host_ptr, '/');
    if (NULL == path_ptr) {
        return -2;
    }

    name_ptr = strchr(path_ptr, '?');
    if (NULL == name_ptr) {
        return -2;
    }

    name_len = name_ptr - path_ptr;

    memcpy(name, path_ptr + 1, name_len - 1);
    name[name_len] = '\0';

    return SUCCESS_RET;
}

void * HAL_Download_Name_Set(void * handle)
{
    char *url_str = (char *)handle;
    char *name_str;
    if(NULL == (name_str = HAL_Malloc(strlen(url_str))))
    {
        printf("malloc url_str failed");
        return NULL;
    }
    if(SUCCESS_RET == _utils_parse_name(url_str, name_str))
        return name_str;
    else
        return NULL;
}

void * HAL_Download_Init(_IN_ void * name)
{
    char * file_name =(char *)name;
    return (void *)(fopen(file_name, "wb+"));
}

int HAL_Download_Write(_IN_ void * handle,_IN_ uint32_t total_length,_IN_ char *buffer_read,_IN_ uint32_t length)
{
    FILE *fp =(FILE *)handle;
    if(NULL == fp)
        return FAILURE_RET;
    if(1 == fwrite(buffer_read, length, 1, fp))
        return SUCCESS_RET;
    else
        return FAILURE_RET;
}

int  HAL_Download_End(_IN_ void * handle)
{
    FILE *fp =(FILE *)handle;
    if(NULL == fp)
        return FAILURE_RET;
    fclose(fp);
    return SUCCESS_RET;
}


