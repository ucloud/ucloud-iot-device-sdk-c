#ifndef _HAL_FLASH_H_
#define _HAL_FLASH_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "stm32f7xx_hal.h"
#include "uiot_import.h"
#define CURRENT_VERSION_START_ADDR  0x08018000
#define DOWNLOAD_FINISH_FLG_ADDR    0x08018018
#define DOWNLOAD_START_ADDR         0x08100000 


#define DOWNLOAD_SUCESS      0X00
#define DOWNLOAD_FAILED      0X01
#define VERSION_BYTE_NUM       24


/**
 * @brief 设置相应name
 *
 * @param   handle          指向download_name的指针          
 * @return                  指向download_name的指针
 */
void * HAL_Download_Name_Set(void * handle);

/**
* @brief    STM32F767 FLASH的information分区的信息变更以及擦除download分区
            本函数将信息区sector的内容先取出来然后擦除信息区sector
            擦除完毕之后将除了下载状态标志位以外的信息进行写入
            再写入下载状态位为下载失败
            然后擦除整个下载区
 *
 * @param   name              此接口中不使用
 * @return  成功返回指向下载分区地址的指针，失败返回NULL
 */
void * HAL_Download_Init(_IN_ void * name);

/**
 * @brief    将长度为length的buf写入到FLASH中
 *
 * @param    handle          下载分区首地址指针
 * @param    total_length    未用到
 * @param    buffer_read     数据的指针
 * @param    length          数据的长度，单位为字节
 * @return  0-success 其他-fail                  
 */
int HAL_Download_Write(_IN_ void * handle,_IN_ uint32_t total_length,_IN_ uint8_t *buffer_read,_IN_ uint32_t length);

/**
 * @brief STM32F767 FLASH的information分区的下载标志置位成功
 *
 * @param	handle            未用到             
 * @return                    0-success 其他-fail
 */
int HAL_Download_End(_IN_ void * handle);

/**
 * @brief 向FLASH地址中写入一个字节的数据
 *
 * @param addr              FLASH地址
 * @param data              要写入的一个字节的数据
 * @return                  0-success 其他-fail
 */
int HAL_FLASH_Write_Byte(_IN_ uint32_t addr,_IN_ uint32_t data);

/**
 * @brief 擦除FLASH地址的sector
 *
 * @param addr              FLASH sector
 * @return                  0-success 其他-fail
 */
int HAL_FLASH_Erase_Sector(_IN_ uint8_t sector, _IN_ uint32_t sector_num);

/**
 * @brief 读FLASH地址的一个字节数据
 *
 * @param addr              FLASH地址
 * @return                  当前传入地址的一个字节的数据
 */
uint8_t HAL_FLASH_Read_Byte(_IN_ uint8_t addr);

/**
 * @brief 给FLASH地址上锁
 *
 * @param addr              无
 * @return                  无
 */
void HAL_FLASH_Locked(void);

/**
 * @brief 给FLASH地址解锁
 *
 * @param addr              无
 * @return                  无
 */
void HAL_FLASH_Unlocked(void);


/**
 * @brief 获得版本信息
 *
 * @param                   指向版本号字符串的char型指针
 * @return                  无
 */
//void version_get(char * verptr);

#if defined(__cplusplus)
}
#endif
#endif  /* _HAL_FLASH_H_ */

