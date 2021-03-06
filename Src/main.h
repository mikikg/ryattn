//
// Created by miki on 23/04/2021.
//

#ifndef MYPRJ_MAIN_H
#define MYPRJ_MAIN_H

#include "stm32f1xx.h"

#define bitmask1 (MyData[VOLUME] & 0b00000001)
#define bitmask2 (MyData[VOLUME] & 0b00000010)
#define bitmask3 (MyData[VOLUME] & 0b00000100)
#define bitmask4 (MyData[VOLUME] & 0b00001000)
#define bitmask5 (MyData[VOLUME] & 0b00010000)
#define bitmask6 (MyData[VOLUME] & 0b00100000)

#define RY1_on GPIOA->BSRR=GPIO_BSRR_BS3
#define RY2_on GPIOA->BSRR=GPIO_BSRR_BS4
#define RY3_on GPIOA->BSRR=GPIO_BSRR_BS5
#define RY4_on GPIOA->BSRR=GPIO_BSRR_BS6
#define RY5_on GPIOA->BSRR=GPIO_BSRR_BS7
#define RY6_on GPIOB->BSRR=GPIO_BSRR_BS0
#define RY7_on GPIOB->BSRR=GPIO_BSRR_BS1 //mute

#define RY8_on GPIOC->BSRR=GPIO_BSRR_BS14 //power off/on
#define RY9_on GPIOC->BSRR=GPIO_BSRR_BS15 //ch sw
#define PC13_on GPIOC->BSRR=GPIO_BSRR_BS13 //PC13 LED

#define RY1_off GPIOA->BSRR=GPIO_BSRR_BR3
#define RY2_off GPIOA->BSRR=GPIO_BSRR_BR4
#define RY3_off GPIOA->BSRR=GPIO_BSRR_BR5
#define RY4_off GPIOA->BSRR=GPIO_BSRR_BR6
#define RY5_off GPIOA->BSRR=GPIO_BSRR_BR7
#define RY6_off GPIOB->BSRR=GPIO_BSRR_BR0
#define RY7_off GPIOB->BSRR=GPIO_BSRR_BR1 //mute

#define RY8_off GPIOC->BSRR=GPIO_BSRR_BR14 //power off/on
#define RY9_off GPIOC->BSRR=GPIO_BSRR_BR15 //ch sw

#define IR_PIN (GPIOB->IDR & GPIO_IDR_IDR3) //IR
#define Button (GPIOA->IDR & GPIO_IDR_IDR2) //Button
#define PC13_off GPIOC->BSRR=GPIO_BSRR_BR13 //PC13 LED

/* --- PRINTF_BYTE_TO_BINARY macro's --- */
#define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT8(i)    \
    (((i) & 0x80ll) ? '1' : '0'), \
    (((i) & 0x40ll) ? '1' : '0'), \
    (((i) & 0x20ll) ? '1' : '0'), \
    (((i) & 0x10ll) ? '1' : '0'), \
    (((i) & 0x08ll) ? '1' : '0'), \
    (((i) & 0x04ll) ? '1' : '0'), \
    (((i) & 0x02ll) ? '1' : '0'), \
    (((i) & 0x01ll) ? '1' : '0')

#define PRINTF_BINARY_PATTERN_INT16 \
    PRINTF_BINARY_PATTERN_INT8              PRINTF_BINARY_PATTERN_INT8
#define PRINTF_BYTE_TO_BINARY_INT16(i) \
    PRINTF_BYTE_TO_BINARY_INT8((i) >> 8),   PRINTF_BYTE_TO_BINARY_INT8(i)
#define PRINTF_BINARY_PATTERN_INT32 \
    PRINTF_BINARY_PATTERN_INT16             PRINTF_BINARY_PATTERN_INT16
#define PRINTF_BYTE_TO_BINARY_INT32(i) \
    PRINTF_BYTE_TO_BINARY_INT16((i) >> 16), PRINTF_BYTE_TO_BINARY_INT16(i)
#define PRINTF_BINARY_PATTERN_INT64    \
    PRINTF_BINARY_PATTERN_INT32             PRINTF_BINARY_PATTERN_INT32
#define PRINTF_BYTE_TO_BINARY_INT64(i) \
    PRINTF_BYTE_TO_BINARY_INT32((i) >> 32), PRINTF_BYTE_TO_BINARY_INT32(i)
/* --- end macros --- */

void MYSYS_init(void) {
    /***********    ----    Clock Setup     -----   ***********/
    //  HSI -> PLL -> SYSCLK
    //  8MHz    *9    72MHz
    //
    RCC->CR |= RCC_CR_HSEON;// Enable HSE
    //RCC->CR |= RCC_CR_HSION;// Enable HSI (RC Osc)

    while(!(RCC->CR & (RCC_CR_HSERDY)));   // Wait till HSE ready
    RCC->CFGR |= RCC_CFGR_SW_HSE;                  // Swich to HSE temporarly
    //RCC->CR   &= ~RCC_CR_HSION;                 // Disable HSI
    RCC->CFGR |= RCC_CFGR_PLLMULL9;      // Set PLL multiplication to 9
    RCC->CFGR |= RCC_CFGR_PLLSRC;          // HSE as PLL_SRC
    FLASH->ACR = 0b10010;            // Enable flash prefetch
    RCC->CR |= RCC_CR_PLLON;            // Enable PLL
    while(!(RCC->CR & (RCC_CR_PLLRDY)));   // Wait till PLL ready
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

 IR Input (EXTI3)
    PB3
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

    //--------------- PC14 - Power Off/On -------------
    GPIOC->CRH &= ~GPIO_CRH_CNF14_0; //ocisti bit CNF14_0
    GPIOC->CRH &= ~GPIO_CRH_CNF14_1; //ocisti bit CNF14_1
    GPIOC->CRH |= GPIO_CRH_MODE14_0; //setuj bit MODE14_0
    GPIOC->CRH |= GPIO_CRH_MODE14_1; //setuj bit MODE14_1

    //--------- PC15 - Input CH1/CH2 switch -------------
    GPIOC->CRH &= ~GPIO_CRH_CNF15_0; //ocisti bit CNF15_0
    GPIOC->CRH &= ~GPIO_CRH_CNF15_1; //ocisti bit CNF15_1
    GPIOC->CRH |= GPIO_CRH_MODE15_0; //setuj bit MODE15_0
    GPIOC->CRH |= GPIO_CRH_MODE15_1; //setuj bit MODE15_1

    //-------------- IR INPUT ------------------------
    //konfiguracija PB3 - IR EXTI3 input
    //definisi ga kao INPUT, pull-down
    GPIOB->CRL &= ~GPIO_CRL_CNF3_0; //ocisti bit
    GPIOB->CRL |=  GPIO_CRL_CNF3_1; //setuj bit
    GPIOB->CRL &= ~GPIO_CRL_MODE3_0; //ocisti bit
    GPIOB->CRL &= ~GPIO_CRL_MODE3_1; //ocisti bit
    GPIOB->BSRR =  GPIO_BSRR_BS3; //BS0=1=pull-up, BR0=0=pull-down

    //IR Input preko interapta ------------------------
    //Konfigurisi EXTI3 za PBx (podrazumeva se PB3)
    AFIO->EXTICR[0] |= AFIO_EXTICR1_EXTI3_PB;

    //unmaskiraj EXTI3
    EXTI->IMR |= EXTI_IMR_MR3;

    //podesi da se aktivira interupt na obe ivice
    //rastuca=EXTI_RTSR, padajuca=EXTI_FTSR
    EXTI->RTSR |= EXTI_RTSR_TR3;
    EXTI->FTSR |= EXTI_FTSR_TR3;

    //Enable NVIC EXTI interrupt za PB3, on je na EXTI3
    NVIC_EnableIRQ(EXTI3_IRQn); //enable channel
    //NVIC_SetPriority(EXTI3_IRQn, 1); //interupt priority

    EXTI->PR |= EXTI_PR_PR3; //flag se cisti tako sto se upise 1


}

#endif //MYPRJ_MAIN_H
