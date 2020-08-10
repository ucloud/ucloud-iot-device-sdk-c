#ifndef _HAL_FLASH_H_
#define _HAL_FLASH_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "uiot_import.h"



/**
 * @brief 设置相应name
 *
 * @param   handle          指向download_name的指针          
 * @return                  指向download_name的指针
 */
void * HAL_Download_Name_Set(void * handle);

/**
 * @brief    
 *
 * @param name              文件名
 * @return                  文件描述符
 */
void * HAL_Download_Init(_IN_ void * name);

/**
* @brief 将长度为length的buffer_read的数据写入到FLASH中
 *
 * @param    handle          文件描述符
 * @param    total_length    未用到
 * @param    buffer_read     数据的指针
 * @param    length          数据的长度，单位为字节
 * @return                  
 */
int HAL_Download_Write(_IN_ void * handle,_IN_ uint32_t total_length,_IN_ char *buffer_read,_IN_ uint32_t length);

/**
 * @brief STM32F767 FLASH的information分区的下载标志置位成功
 *
 * @param    handle          文件描述符
 * @return                   -1失败 0成功
 */
int HAL_Download_End(_IN_ void * handle);


#if defined(__cplusplus)
}
#endif
#endif  /* _HAL_FLASH_H_ */


