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

#ifndef __RING_BUFF_H__
#define __RING_BUFF_H__

#include "stdbool.h"
#include "stdint.h"



#define RINGBUFF_OK            0    /* No error, everything OK. */
#define RINGBUFF_ERR          -1    /* Out of memory error.     */
#define RINGBUFF_EMPTY        -3    /* Timeout.                        */
#define RINGBUFF_FULL         -4    /* Routing problem.          */
#define RINGBUFF_TOO_SHORT    -5 




typedef struct _ring_buff_
{
  uint32_t  size;
  uint32_t  readpoint;
  uint32_t  writepoint;
  char*  buffer;
  bool full;
} sRingbuff;

typedef sRingbuff*  ring_buff_t;

int ring_buff_init(sRingbuff* ring_buff, char* buff, uint32_t size );
int ring_buff_flush(sRingbuff* ring_buff);
int ring_buff_push_data(sRingbuff* ring_buff, uint8_t *pData, int len);
int ring_buff_pop_data(sRingbuff* ring_buff, uint8_t *pData, int len);
bool ring_buff_is_empty(sRingbuff* ring_buff);

#endif // __ringbuff_h__


