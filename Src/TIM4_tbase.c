//---------------------------------
// Konfiguracija za TIM4 da radi kao 1kHz (1ms) timer
// Iskoriscen ovaj HW tajmer za jos 8 64bit SW tajmera 
// i osvezavanje LED

#include "stm32f1xx.h"
#include "TIM4_tbase.h"

volatile uint32_t SW_timers[4];

void TIM4_Setup_TBASE (void) {
	//ukljuci clock za TIM3
	RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

	//konfiguracija za TIM3 kao timer
	TIM4->ARR 	= 36000 * MY_TIM4_TIME_SCALE + 2; //auto reload vrednost za 1kHz
	TIM4->PSC 	= 0x01; //preskaler
	TIM4->DIER 	|= TIM_DIER_UIE; //enable update interrupt
	TIM4->SR = RESET; //clear all flags
    //TIM4->CNT;

	//Enable interrupt
	NVIC_EnableIRQ(TIM4_IRQn); //enable channel 
	NVIC_SetPriority(TIM4_IRQn, 3); //interrupt priority

	//run
	TIM4->CR1		|= TIM_CR1_CEN; //ENABLE tim periferial
}

//---------- TIM4 IRQ 1kHz / 1ms -----------------
void TIM4_IRQHandler (void){	
	
	if (TIM4->SR & TIM_SR_UIF) {
		TIM4->SR &=~ TIM_SR_UIF;//clear flag

		//GPIOC->ODR ^= GPIO_ODR_ODR13; // Toggle LED 

		//inkrementuj 10 32bit SW tajmera, svako po potrebi resetuje
		SW_timers[0] ++;
		SW_timers[1] ++;
		SW_timers[2] ++;
		SW_timers[3] ++;
//		SW_timers[4] ++;
//		SW_timers[5] ++;
//		SW_timers[6] ++;
//		SW_timers[7] ++;
//		SW_timers[8] ++;
//		SW_timers[9] ++;
		
		//GPIOC->ODR ^= GPIO_ODR_ODR13; // Toggle LED 

		//handler_led();

	} 
}

