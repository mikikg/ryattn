//
// Created by miki on 04/05/2021.
//
// flash.c
//
// CMSIS function to write MyData into program flash memory of BluePill
//

#include <string.h>
#include <stdbool.h>
#include "stm32f1xx.h"
#include "flash.h"

extern volatile int ENC_IMPS_PER_STEP;
extern volatile int ENC_IMPS_PER_STEP_HALF;
extern volatile uint16_t MyData[20];
extern volatile bool change_flag;

void Flash_Write_MyData() {

    GPIOC->BSRR = GPIO_BSRR_BR13; //LED ON

    //ponovo enablujemo RC jer treba za programiranje internog FLASH-a u run-time!!!
    RCC->CR |= RCC_CR_HSION;// Enable HSI (RC Osc)
    while(!(RCC->CR & (RCC_CR_HSIRDY)));   // Wait till HSI ready

    //UNLOCK ...
    // Wait for the flash memory not to be busy
    while ((FLASH->SR & FLASH_SR_BSY) != 0 );
    // Check if the controller is unlocked already
    if ((FLASH->CR & FLASH_CR_LOCK) != 0 ){
        // Write the first key
        FLASH->KEYR = FLASH_FKEY1;
        // Write the second key
        FLASH->KEYR = FLASH_FKEY2;
    }

    //ERASE ...
    FLASH->CR |= FLASH_CR_PER; // Page erase operation
    FLASH->AR = ADDR_FLASH_PAGE_62;     // Set the address to the page to be written


    // Wait until page erase is done
    while ((FLASH->SR & FLASH_SR_BSY) != 0);

    FLASH->CR |= FLASH_CR_STRT;// Start the page erase

    // If the end of operation bit is set...
    if ((FLASH->SR & FLASH_SR_EOP) != 0){
        // Clear it, the operation was successful
        FLASH->SR |= FLASH_SR_EOP;
    }
        //Otherwise there was an error
    else{
        // Manage the error cases
    }
    // Get out of page erase mode
    FLASH->CR &= ~FLASH_CR_PER;

    //WRITE ...
    FLASH->CR |= FLASH_CR_PG;                   // Programing mode

    //copy MyData to EE
    for (int a=0; a<20; a++) {//20 * 16bit
        *(__IO uint16_t *) (ADDR_FLASH_PAGE_62 + 16 * a) = MyData[a];       // Write data
    }

    // Wait until the end of the operation
    while ((FLASH->SR & FLASH_SR_BSY) != 0);
    // If the end of operation bit is set...
    if ((FLASH->SR & FLASH_SR_EOP) != 0){
        // Clear it, the operation was successful
        FLASH->SR |= FLASH_SR_EOP;
    }
        //Otherwise there was an error
    else{
        // Manage the error cases
    }
    FLASH->CR &= ~FLASH_CR_PG;

    //LOCK ...
    FLASH->CR |= FLASH_CR_LOCK;

    GPIOC->BSRR = GPIO_BSRR_BS13; //LED OFF

    change_flag = 0;

}

void Flash_Read_MyData() {
    /*
    //Read this from EE ...
    MyData[VOLUME] = 64;
    MyData[MENU] = 0;
    MyData[OVERLAP] = 100;
    MyData[DELAY] = 128;
    MyData[SLOPE] = 255;
    MyData[QEIMODE] = 2;
    MyData[IMPSSTEP] = 200;
    MyData[ENABLEIR] = 0;
    MyData[SSAVER] = 0;
     */

    GPIOC->BSRR = GPIO_BSRR_BR13; //LED ON

    //copy data from EE to MyData
    for (int a=0; a< sizeof(MyData)/ sizeof(uint16_t); a++) {//20 * 16bit
        MyData[a] = *(__IO uint16_t *) (ADDR_FLASH_PAGE_62 + 16 * a) ;       // Read data
    }

    GPIOC->BSRR = GPIO_BSRR_BS13; //LED OFF
}
