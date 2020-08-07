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


