//---------------------------------
// Konfiguracija za TIM1 da radi kao IR tajmer

#include "stm32f1xx.h"
#include "TIM1_tbase.h"

void TIM1_Setup_TBASE (void) {
	//ukljuci clock za TIM1
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

	//konfiguracija za TIM1
	TIM1->ARR 	= 0xFFFF/4; //auto reload vrednost
	TIM1->PSC 	= 719; //preskaler
	TIM1->DIER 	|= TIM_DIER_UIE; //enable update interrupt
	TIM1->SR = RESET; //clear all flags
	//TIM1->RCR = 0xFF;
    //TIM1->CNT;

	//Enable interrupt
//	NVIC_EnableIRQ(TIM1_BRK_IRQn); //enable channel
//	NVIC_EnableIRQ(TIM1_CC_IRQn); //enable channel
//	NVIC_EnableIRQ(TIM1_TRG_COM_IRQn); //enable channel
	NVIC_EnableIRQ(TIM1_UP_IRQn); //enable channel
	//NVIC_SetPriority(TIM1_IRQn, 3); //interrupt priority

	//TIM1->SR &=~ TIM_SR_UIF;//clear flag
	//run
	TIM1->CR1		|= TIM_CR1_CEN; //ENABLE tim periferial


}

////---------- TIM1 IRQ 1kHz / 1ms -----------------
//void TIM1_IRQHandler (void){
//	if (TIM4->SR & TIM_SR_UIF) {
//		TIM4->SR &=~ TIM_SR_UIF;//clear flag
//		//GPIOC->ODR ^= GPIO_ODR_ODR13; // Toggle LED
//		//GPIOC->ODR ^= GPIO_ODR_ODR13; // Toggle LED
//	}
//}

