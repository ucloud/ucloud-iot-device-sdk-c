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

