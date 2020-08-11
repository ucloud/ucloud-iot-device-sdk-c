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

uint32_t download_addr = DOWNLOAD_START_ADDR;

void * HAL_Download_Name_Set(void * handle)
{
    return NULL;
}

void * HAL_Download_Init(_IN_ void * name)
{
    int i,ret;
    char version_info[VERSION_BYTE_NUM]; 
    uint32_t info_addr = CURRENT_VERSION_START_ADDR;
    for(i = 0; i < VERSION_BYTE_NUM; i++)
    {
        version_info[i] = HAL_FLASH_Read_Byte(info_addr);
        info_addr++;
    }
    if(FAILURE_RET == HAL_FLASH_Erase_Sector(FLASH_SECTOR_3, 1))
        return NULL;
    info_addr = CURRENT_VERSION_START_ADDR;
    HAL_FLASH_Unlocked();    
    for(i = 0; i < VERSION_BYTE_NUM; i++)
    {
        ret = HAL_FLASH_Write_Byte(info_addr++,version_info[i]);
        if(SUCCESS_RET != ret)
            return NULL;
    }
    ret = HAL_FLASH_Write_Byte(DOWNLOAD_FINISH_FLG_ADDR, DOWNLOAD_FAILED);
    if(SUCCESS_RET != ret)
        return NULL;
    ret = HAL_FLASH_Erase_Sector(FLASH_SECTOR_8, 4);
    if(SUCCESS_RET != ret)
        return NULL;    
    HAL_FLASH_Locked();
    download_addr = DOWNLOAD_START_ADDR;
    return &download_addr;
}

int HAL_Download_Write(_IN_ void * handle,_IN_ uint32_t total_length,_IN_ uint8_t *buffer_read,_IN_ uint32_t length)
{
    uint8_t *readptr = buffer_read;
    int i = 0,ret;    
    HAL_FLASH_Unlocked();
    for(i = length; i > 0; i--){
        ret = HAL_FLASH_Write_Byte(download_addr++, *readptr++);
        if(FAILURE_RET == ret){
            printf("HAL_Download_Write failed!\r\n");
            return FAILURE_RET;
        }
    }
    HAL_FLASH_Locked();

    return SUCCESS_RET;
}

int HAL_Download_End(_IN_ void * handle)
{
    HAL_FLASH_Write_Byte(DOWNLOAD_FINISH_FLG_ADDR,DOWNLOAD_SUCESS);
    
    //reset here
    __set_FAULTMASK(1);
    HAL_NVIC_SystemReset();
    return SUCCESS_RET;
}


int HAL_FLASH_Write_Byte(_IN_ uint32_t sddr,_IN_ uint32_t data)
{
    HAL_StatusTypeDef ret;
    ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, sddr, data);
    if(ret == HAL_ERROR){
        printf("write data byte to flash failed!\r\n");
        return FAILURE_RET;
    }
    else
        return SUCCESS_RET;
}
int HAL_FLASH_Erase_Sector(_IN_ uint8_t sector, _IN_ uint32_t sector_num)
{
    uint32_t flash_error;
    HAL_StatusTypeDef ret;
    FLASH_EraseInitTypeDef flash_erase;    
    HAL_FLASH_Unlock();
    flash_erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    flash_erase.Sector = sector; 
    flash_erase.NbSectors = sector_num;                        
    flash_erase.VoltageRange = FLASH_VOLTAGE_RANGE_3; 
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_ERSERR | FLASH_FLAG_BSY);
    ret = HAL_FLASHEx_Erase(&flash_erase, &flash_error);
    FLASH_WaitForLastOperation(50000);    
    HAL_FLASH_Lock();
    if(HAL_ERROR == ret){
        printf("erase sector in flash failed!\r\n");
        return FAILURE_RET;
    }
    else
        return SUCCESS_RET;
}
uint8_t HAL_FLASH_Read_Byte(_IN_ uint8_t addr)
{
    uint8_t data;
    data=*(__IO uint8_t*)( addr);
    return data;
}

void HAL_FLASH_Locked(void)
{
    HAL_FLASH_Lock();
}

void HAL_FLASH_Unlocked(void)
{
    HAL_FLASH_Unlock();	
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_ERSERR | FLASH_FLAG_BSY);
}

