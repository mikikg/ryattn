//---------------------------------
// Konfiguracija za TIM2 da radi u Encoder Modu
// Koristi se:
//
//	PA0 kao Encoder A input
//	PA1 kao Encoder B input
// 	PA2 za Button input
//

#include <stdbool.h>
#include "stm32f1xx.h"
#include "TIM2_enc.h"

#define Button GPIOA->IDR & GPIO_IDR_IDR2

volatile uint16_t MyData[32];
volatile uint16_t MyDataLimHI[32] = {
        0, //
        0,  //
        4,  // OPmode
        500, // DELAY
        1, // DEBUG
        3, // QEIMODE
        2000, // IMPSSTEP
        1, // ENABLEIR
        8, // THEME
        5*60, // SSAVER
        255, // IR-volUP
        255, // IR-volDOWN
        255, // IR-Mute
        255, // IR-CHSW
        255, // IR-Power
        2,   // Input CH
        1,   // RY Power save
};
volatile uint16_t MyDataLimLO[32] = {
        0, //
        0,  //
        1,  // OPmode
        0, // DELAY
        0, // SLOPE
        2, // QEIMODE
        1, // IMPSSTEP
        0, // ENABLEIR
        0, // THEME
        0, // SSAVER
        1, // IR-volUP
        1, // IR-volDOWN
        1, // IR-Mute
        1, // IR-CHSW
        1, // IR-Power
        1, // Input CH
        0, // RY Power save
};

#define max_menu 20

volatile bool menu_active = 0;
volatile bool edit_active = 0;
volatile bool mute_active = 0;
volatile bool save_change_flag = 0;
volatile bool vol_change_flag = 0;
volatile int ENC_IMPS_PER_STEP = 2;
volatile int ENC_IMPS_PER_STEP_HALF;

extern volatile uint32_t SW_timers[8];
extern volatile uint32_t SW_timers_enable[8];
extern volatile bool screen_saver_active;
extern volatile bool update_seq_up_down;
extern volatile int current_seq_position;
extern volatile int seq_position_max;

void TIM2_Setup_GPIO (void) {

    //ukljuci clock za TIM2
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
    //GPIOA->BSRR =  GPIO_BSRR_BR2; //BS0=1=pull-up, BR0=0=pull-down

    //Button Input preko interapta ------------------------
    //Konfigurisi EXTI2 za PAx (podrazumeva se PA2)
    AFIO->EXTICR[0] |= AFIO_EXTICR1_EXTI2_PA;

    //unmaskiraj EXTI2
    EXTI->IMR |= EXTI_IMR_MR2;

    //podesi da se aktivira interupt na obe ivice
    //rastuca=EXTI_RTSR, padajuca=EXTI_FTSR
    EXTI->RTSR |= EXTI_RTSR_TR2;
    EXTI->FTSR |= EXTI_FTSR_TR2;

    //Enable NVIC EXTI interrupt za PA2, on je na EXTI2
    NVIC_EnableIRQ(EXTI2_IRQn); //enable channel
    //NVIC_SetPriority(EXTI2_IRQn, 1); //interupt priority

}
void TIM2_Setup_ENC (void) {

    TIM2_Setup_GPIO();
    ENC_IMPS_PER_STEP_HALF = ENC_IMPS_PER_STEP / 2; //init values

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
	//NVIC_SetPriority(TIM2_IRQn, 1); //interupt priority

	//run
	TIM2->CR1		|= TIM_CR1_CEN; //ENABLE tim periferial
}

#define SELMENU MyData[MENU]+1

//---------- TIM2 IRQ za enkoder -----------------
void TIM2_IRQHandler (void){

	if (TIM2->SR & TIM_SR_UIF) {//update interupt
		if (TIM2->CR1 & TIM_CR1_DIR) {//koji je smer
            TIM2->CNT = ENC_IMPS_PER_STEP_HALF; //put on half for hysteresis
            if (menu_active) {
                if (!edit_active) {
                    if (MyData[MENU] > 0) MyData[MENU] --;
                } else {
                    if (MyData[SELMENU] < MyDataLimHI[SELMENU]) {
                        MyData[SELMENU]++; //increment data
                        save_change_flag = 1;
                    }
                }
            } else {
                if (!mute_active && MyData[VOLUME] > 0) {
                    MyData[VOLUME] --;
                    save_change_flag = 1;
                    vol_change_flag = 1;
                    SW_timers[7]=0;
                    current_seq_position = update_seq_up_down ? 0 : seq_position_max;
                }
            }
		} else {
            TIM2->CNT = ENC_IMPS_PER_STEP_HALF; //put on half for hysteresis
            if (menu_active) {
                if (!edit_active) {
                    if (MyData[MENU] < max_menu) MyData[MENU] ++;
                } else {
                    if (MyData[SELMENU] > MyDataLimLO[SELMENU]) {
                        MyData[SELMENU] --; //decrement data
                        save_change_flag = 1;
                    }
                }
            } else {
                if (!mute_active && MyData[VOLUME] < 64) {
                    MyData[VOLUME] ++;
                    save_change_flag = 1;
                    vol_change_flag = 1;
                    SW_timers[7]=0;
                    current_seq_position = update_seq_up_down ? 0 : seq_position_max;
                }
            }
		}

        TIM2->SR &=~ TIM_SR_UIF;//clear flag

        //probudi ekran
        screen_saver_active = false;
        SW_timers[5] = 0;
    }
}

//---------- EXTI PA2 BUTTON interrupt -----------------
void EXTI2_IRQHandler (void){ //button padajuca ivica ...
    if (EXTI->PR & EXTI_PR_PR2) { //jel pending od kanala 2?
        SW_timers_enable[0] = Button ? 1 : 0;
        EXTI->PR |= EXTI_PR_PR2; //flag se cisti tako sto se upise 1

        //probudi ekran i ugasi led
        screen_saver_active = false;
        SW_timers[5] = 0;
        //GPIOC->BSRR = GPIO_BSRR_BS13;
    }
}

