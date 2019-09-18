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

#ifndef _AT_UTILS_H_
#define _AT_UTILS_H_
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "at_client.h"

//#define AT_PRINT_RAW_CMD
#define WIDTH_SIZE           32

#ifndef __INT_MAX__
#define __INT_MAX__     2147483647
#endif
#define INT_MAX         (__INT_MAX__)

#define __is_print(ch)       ((unsigned int)((ch) - ' ') < 127u - ' ')

int at_vprintfln(at_data_type data_type, size_t data_size, const char *format, va_list args);
void at_print_raw_cmd(const char *name, const char *cmd, int size);
const char *at_get_last_cmd(int *cmd_size);
int at_req_parse_args(const char *req_args, const char *req_expr, ...);
int at_sscanf(const char * buf, const char * fmt, va_list args);
void at_strip(char *str, const char patten);


#endif
