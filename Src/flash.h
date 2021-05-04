//
// Created by miki on 04/05/2021.
//
// flash.h
//
// CMSIS function to write MyData into program flash memory of BluePill
//

#define ADDR_FLASH_PAGE_62    ((uint32_t)0x0800F800) /* Base @ of Page 62, 1 Kbyte */
#define ADDR_FLASH_PAGE_63    ((uint32_t)0x0800FC00) /* Base @ of Page 63, 1 Kbyte */  // << STM32F103C8 0x10000 and one sector is 1k 0x400 !!!!!!!
//keys as described in the data-sheets
#define FLASH_FKEY1      ((uint32_t)0x45670123)
#define FLASH_FKEY2      ((uint32_t)0xCDEF89AB)

void Flash_Write_MyData();
void Flash_Read_MyData();
