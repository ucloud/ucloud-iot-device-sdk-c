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

#ifndef _MODULE_API_INF_H_
#define _MODULE_API_INF_H_
#include "stdint.h"
#include "uiot_defs.h"

typedef enum{
    eDISCONNECTED = 0,  //未连接
    eCONNECTED = 1,     //已连接
}eMqtt_State;

IoT_Error_t module_init();

#endif
