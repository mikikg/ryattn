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

typedef struct {
    uint16_t low;
    uint16_t high;
} DataLimits;

volatile DataLimits MyDataLimits[32] = {
        {0, 0},     // Unused
        {0, 0},     // Unused
        {1, 4},     // OPmode
        {0, 500},   // DELAY
        {0, 1},     // DEBUG
        {2, 3},     // QEIMODE
        {1, 2000},  // IMPSSTEP
        {0, 1},     // ENABLEIR
        {0, 8},     // THEME
        {0, 5*60},  // SSAVER
        {1, 255},   // IR-volUP
        {1, 255},   // IR-volDOWN
        {1, 255},   // IR-Mute
        {1, 255},   // IR-CHSW
        {1, 255},   // IR-Power
        {1, 2},     // Input CH
        {0, 1},     // RY Power save
        {0, 9},     // CH1 name
        {0, 9},     // CH2 name
};

volatile uint16_t MyData[32];

#define max_menu 22

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

    if (TIM2->SR & TIM_SR_UIF) {  // Update interrupt
        TIM2->CNT = ENC_IMPS_PER_STEP_HALF; // Put on half for hysteresis
        if (TIM2->CR1 & TIM_CR1_DIR) {  // Koji je smer (direction)
            if (menu_active) {
                if (!edit_active) {
                    if (MyData[MENU] > 0) MyData[MENU]--;  // Pomeri unazad u meniju
                } else {
                    if (MyData[SELMENU] < MyDataLimits[SELMENU].high) {
                        MyData[SELMENU]++;  // Increment data
                        save_change_flag = 1;
                    }
                }
            } else {
                if (!mute_active && MyData[VOLUME] > 0) {
                    MyData[VOLUME]--;
                    save_change_flag = 1;
                    vol_change_flag = 1;
                    SW_timers[T_RY] = 0;
                    current_seq_position = update_seq_up_down ? 0 : seq_position_max;
                }
            }
        } else {
            if (menu_active) {
                if (!edit_active) {
                    if (MyData[MENU] < max_menu) MyData[MENU]++;  // Pomeri napred u meniju
                } else {
                    if (MyData[SELMENU] > MyDataLimits[SELMENU].low) {
                        MyData[SELMENU]--;  // Decrement data
                        save_change_flag = 1;
                    }
                }
            } else {
                if (!mute_active && MyData[VOLUME] < 64) {
                    MyData[VOLUME]++;
                    save_change_flag = 1;
                    vol_change_flag = 1;
                    SW_timers[T_RY] = 0;
                    current_seq_position = update_seq_up_down ? 0 : seq_position_max;
                }
            }
        }

        TIM2->SR &= ~TIM_SR_UIF;  // Clear flag

        // Probudi ekran
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

