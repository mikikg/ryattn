//
// Created by miki on 23/04/2021.
//

#ifndef MYPRJ_MAIN_H
#define MYPRJ_MAIN_H

#include "stm32f1xx.h"
#include "SPI2.h"

#include "USART3.h"

void MYSYS_init(void) {
    /***********    ----    Clock Setup     -----   ***********/
    //  HSE -> PLL -> SYSCLK
    //  8MHz    *9    72MHz
    //
    RCC->CR |= (1 << 16);            // Enable HSE
    while(!(RCC->CR & (1 << 17)));   // Wait till HSE ready
    RCC->CFGR |= 1;                  // Swich to HSE temporarly
    RCC->CR   &= ~1;                 // Disable HSI
    RCC->CFGR |= (0b111 << 18);      // Set PLL multiplication to 9
    RCC->CFGR |= (1 << 16);          // HSE as PLL_SRC
    FLASH->ACR = 0b10010;            // Enable flash prefetch
    RCC->CR |= (1 << 24);            // Enable PLL
    while(!(RCC->CR & (1 << 25)));   // Wait till PLL ready
    RCC->CFGR = (RCC->CFGR | 0b10) & ~1; // Set PLL as Clock source
    while(!(RCC->CFGR & (1 << 3)));  // Wait till PLL is CLK SRC
    /***********    ----    Clock ready     -----   ***********/

    //ukljuci clock za APB2, on je za sve GPIOx
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; //GPIOA
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; //GPIOB
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN; //GPIOC

    //Alternate Function I/O clock enable - Treba da bi proradile GPIO na JTAG, EXTI, I2C
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    //oslobodi nozice od JTAG, koristimo samo SWD
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE; //JTAG-DP Disabled and SW-DP Enabled

    //konfiguracija za PC13
    //po default svi pinovi su Input
    //ima dva registra CRL za pinove 0-7 i CRH za pinove 8-15
    //definisi pin kao general purpose Output push-pull, 50MHz
    GPIOC->CRH &= ~GPIO_CRH_CNF13_0; //ocisti bit CNF13_0
    GPIOC->CRH &= ~GPIO_CRH_CNF13_1; //ocisti bit CNF13_1
    GPIOC->CRH |= GPIO_CRH_MODE13_0; //setuj bit MODE13_0
    GPIOC->CRH |= GPIO_CRH_MODE13_1; //setuj bit MODE13_1
}

void GPIO_init(void) {

    /*
 Button Input:
    PA2

 Relay OUTs:
    PA3 - Relay 1
    PA4 - Relay 2
    PA5 - Relay 3
    PA6 - Relay 4
    PA7 - Relay 5
    PB0 - Relay 6

 Mute OUT:
    PB1
     */

    //--------------- PA3 - Relay 1 -------------
    //GP function Output Push-Pull
    GPIOA->CRL &= ~GPIO_CRL_CNF3_0; //ocisti bit CNF3_0
    GPIOA->CRL &= ~GPIO_CRL_CNF3_1; //ocisti bit CNF3_1
    //Output MODE 50MHz
    GPIOA->CRL |=  GPIO_CRL_MODE3_0; //setuj bit MODE3_0
    GPIOA->CRL |=  GPIO_CRL_MODE3_1; //setuj bit MODE3_1
    GPIOA->BSRR=GPIO_BSRR_BR3; //PB3 = LOW

    //--------------- PA4 - Relay 2 -------------
    //GP function Output Push-Pull
    GPIOA->CRL &= ~GPIO_CRL_CNF4_0; //ocisti bit CNF4_0
    GPIOA->CRL &= ~GPIO_CRL_CNF4_1; //ocisti bit CNF4_1
    //Output MODE 50MHz
    GPIOA->CRL |=  GPIO_CRL_MODE4_0; //setuj bit MODE4_0
    GPIOA->CRL |=  GPIO_CRL_MODE4_1; //setuj bit MODE4_1
    GPIOA->BSRR=GPIO_BSRR_BR4; //PB4 = LOW

    //--------------- PA5 - Relay 3 -------------
    //GP function Output Push-Pull
    GPIOA->CRL &= ~GPIO_CRL_CNF5_0; //ocisti bit CNF5_0
    GPIOA->CRL &= ~GPIO_CRL_CNF5_1; //ocisti bit CNF5_1
    //Output MODE 50MHz
    GPIOA->CRL |=  GPIO_CRL_MODE5_0; //setuj bit MODE5_0
    GPIOA->CRL |=  GPIO_CRL_MODE5_1; //setuj bit MODE5_1
    GPIOA->BSRR=GPIO_BSRR_BR5; //PB5 = LOW

    //--------------- PA6 - Relay 4 -------------
    //GP function Output Push-Pull
    GPIOA->CRL &= ~GPIO_CRL_CNF6_0; //ocisti bit CNF6_0
    GPIOA->CRL &= ~GPIO_CRL_CNF6_1; //ocisti bit CNF6_1
    //Output MODE 50MHz
    GPIOA->CRL |=  GPIO_CRL_MODE6_0; //setuj bit MODE6_0
    GPIOA->CRL |=  GPIO_CRL_MODE6_1; //setuj bit MODE6_1
    GPIOA->BSRR=GPIO_BSRR_BR6; //PB6 = LOW

    //--------------- PA7 - Relay 5 -------------
    //GP function Output Push-Pull
    GPIOA->CRL &= ~GPIO_CRL_CNF7_0; //ocisti bit CNF7_0
    GPIOA->CRL &= ~GPIO_CRL_CNF7_1; //ocisti bit CNF7_1
    //Output MODE 50MHz
    GPIOA->CRL |=  GPIO_CRL_MODE7_0; //setuj bit MODE7_0
    GPIOA->CRL |=  GPIO_CRL_MODE7_1; //setuj bit MODE7_1
    GPIOA->BSRR=GPIO_BSRR_BR7; //PB7 = LOW

    //--------------- PB0 - Relay 6 -------------
    //GP function Output Push-Pull
    GPIOB->CRL &= ~GPIO_CRL_CNF0_0; //ocisti bit CNF0_0
    GPIOB->CRL &= ~GPIO_CRL_CNF0_1; //ocisti bit CNF0_1
    //Output MODE 50MHz
    GPIOB->CRL |=  GPIO_CRL_MODE0_0; //setuj bit MODE0_0
    GPIOB->CRL |=  GPIO_CRL_MODE0_1; //setuj bit MODE0_1
    GPIOB->BSRR=GPIO_BSRR_BR0; //PB0 = LOW

    //--------------- PB1 - MUTE -------------
    //GP function Output Push-Pull
    GPIOB->CRL &= ~GPIO_CRL_CNF1_0; //ocisti bit CNF1_0
    GPIOB->CRL &= ~GPIO_CRL_CNF1_1; //ocisti bit CNF1_1
    //Output MODE 50MHz
    GPIOB->CRL |=  GPIO_CRL_MODE1_0; //setuj bit MODE1_0
    GPIOB->CRL |=  GPIO_CRL_MODE1_1; //setuj bit MODE1_1
    GPIOB->BSRR=GPIO_BSRR_BR1; //PB1 = LOW

}

#endif //MYPRJ_MAIN_H
