#include <stdbool.h>
#include "stm32f1xx.h"
#include "TIM2_enc.h"

//---------------------------------
// Konfiguracija za TIM2 da radi u Encoder Modu
// Koristi se:
//
//	PA0 kao Encoder A input
//	PA1 kao Encoder B input
// 	PA2 za Button input
//	
//

volatile int32_t MyData[10];

volatile bool menu_active = 0;
volatile bool edit_active = 0;

extern volatile uint32_t SW_timers[4];
void TIM2_Setup_GPIO (void) {
    //ukljuci clock za TIM4
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    //konfiguracija PA0 - A input
    //definisi ga kao INPUT, pull-down
    GPIOA->CRL &= ~GPIO_CRL_CNF0_0; //ocisti bit
    GPIOA->CRL |=  GPIO_CRL_CNF0_1; //setuj bit
    GPIOA->CRL &= ~GPIO_CRL_MODE0_0; //ocisti bit
    GPIOA->CRL &= ~GPIO_CRL_MODE0_1; //ocisti bit
    GPIOA->BSRR =  GPIO_BSRR_BR0; //BS0=1=pull-up, BR0=0=pull-down

    //konfiguracija PA1 - B input
    //definisi ga kao INPUT, pull-down
    GPIOA->CRL &= ~GPIO_CRL_CNF1_0; //ocisti bit
    GPIOA->CRL |=  GPIO_CRL_CNF1_1; //setuj bit
    GPIOA->CRL &= ~GPIO_CRL_MODE1_0; //ocisti bit
    GPIOA->CRL &= ~GPIO_CRL_MODE1_1; //ocisti bit
    GPIOA->BSRR =  GPIO_BSRR_BR1; //BS0=1=pull-up, BR0=0=pull-down

    //konfiguracija PA2 - Button input
    //definisi ga kao INPUT, pull-down
    GPIOA->CRL &= ~GPIO_CRL_CNF2_0; //ocisti bit
    GPIOA->CRL |=  GPIO_CRL_CNF2_1; //setuj bit
    GPIOA->CRL &= ~GPIO_CRL_MODE2_0; //ocisti bit
    GPIOA->CRL &= ~GPIO_CRL_MODE2_1; //ocisti bit
    GPIOA->BSRR =  GPIO_BSRR_BR2; //BS0=1=pull-up, BR0=0=pull-down

}
void TIM2_Setup_ENC (void) {

    TIM2_Setup_GPIO();

	//konfiguracija za TIM2 kao enkoder
	TIM2->ARR 	= ENC_IMPS_PER_STEP; //auto reload vrednost 200
	TIM2->PSC 	= 0; //preskaler
	TIM2->EGR 	= TIM_EGR_UG; //generisi update event
	TIM2->CNT	= ENC_IMPS_PER_STEP_HALF; //set start count (mora posle EGR!)
	TIM2->CCER	= 0; //input capture on rising edge
	TIM2->CCMR1 |= TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0;//input configuration
    TIM2->CCMR1 |= TIM_CCMR1_IC1F_Msk | TIM_CCMR1_IC2F_Msk;//input MAX FILTER configuration
    //TIM2->CCMR1 |= TIM_CCMR1_IC1PSC_0 | TIM_CCMR1_IC2PSC_0;//input prescaler
	TIM2->SMCR 	= 2; //Encoder Mode 3, update na obe ivice = 4Q mode
	TIM2->DIER 	|= TIM_DIER_UIE; //enable update interupt

	TIM2->SR &=~ TIM_SR_UIF;//clear flag

	//Enable TIM interrupt
	NVIC_EnableIRQ(TIM2_IRQn); //enable channel
	NVIC_SetPriority(TIM2_IRQn, 1); //interupt priority

	//run
	TIM2->CR1		|= TIM_CR1_CEN; //ENABLE tim periferial
}

//---------- TIM2 IRQ -----------------
void TIM2_IRQHandler (void){	

	if (TIM2->SR & TIM_SR_UIF) {//update interupt
		if (TIM2->CR1 & TIM_CR1_DIR) {//koji je smer
            TIM2->CNT = ENC_IMPS_PER_STEP_HALF; //put on half for hysteresis
            if (menu_active) {
                if (!edit_active) {
                    MyData[MENU] --;
                } else {
                    MyData[MyData[MENU]+1] ++;
                }
            } else {
                MyData[VOLUME] ++;
            }
		} else {
            TIM2->CNT = ENC_IMPS_PER_STEP_HALF; //put on half for hysteresis
            if (menu_active) {
                if (!edit_active) {
                    MyData[MENU] ++;
                } else {
                    MyData[MyData[MENU]+1] --;
                }
            } else {
                MyData[VOLUME] --;
            }
		}

        //Limiti
        if (MyData[VOLUME] > 0 ) MyData[VOLUME] = 0;
        if (MyData[VOLUME] < -64 ) MyData[VOLUME] = -64;

        if (MyData[MENU] > 9 ) MyData[MENU] = 9;
        if (MyData[MENU] < 0 ) MyData[MENU] = 0;

        if (MyData[OVERLAP] > 100 ) MyData[OVERLAP] = 100;
        if (MyData[OVERLAP] < 0 ) MyData[OVERLAP] = 0;

        if (MyData[DELAY] > 255 ) MyData[DELAY] = 255;
        if (MyData[DELAY] < 0 ) MyData[DELAY] = 0;

        if (MyData[SLOPE] > 255 ) MyData[SLOPE] = 255;
        if (MyData[SLOPE] < 0 ) MyData[SLOPE] = 0;

        if (MyData[QEIMODE] > 3 ) MyData[QEIMODE] = 3;
        if (MyData[QEIMODE] < 2 ) MyData[QEIMODE] = 2;

        if (MyData[IMPSSTEP] > 2000 ) MyData[IMPSSTEP] = 2000;
        if (MyData[IMPSSTEP] < 1 ) MyData[IMPSSTEP] = 1;

        TIM2->SR &=~ TIM_SR_UIF;//clear flag

        //reset timer for last update
        SW_timers[2] = 0;

    }
}

