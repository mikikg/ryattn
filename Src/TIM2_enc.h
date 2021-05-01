
//makroi za r/setovnje izlaza
#define OUT0_SET 						GPIOB->BSRR=GPIO_BSRR_BS12; //PB12
#define OUT1_SET 						GPIOB->BSRR=GPIO_BSRR_BS13; //PB13
#define OUT2_SET 						GPIOB->BSRR=GPIO_BSRR_BS14; //PB14
#define OUT3_SET 						GPIOB->BSRR=GPIO_BSRR_BS15; //PB15
#define OUT4_SET 						GPIOB->BSRR=GPIO_BSRR_BS5; //PB5
#define OUT5_SET 						GPIOB->BSRR=GPIO_BSRR_BS6; //PB6
#define OUT6_SET 						GPIOB->BSRR=GPIO_BSRR_BS7; //PB7
#define OUT7_SET 						GPIOB->BSRR=GPIO_BSRR_BS8; //PB8
#define OUT0_RSET 						GPIOB->BSRR=GPIO_BSRR_BR12; //PB12
#define OUT1_RSET 						GPIOB->BSRR=GPIO_BSRR_BR13; //PB13
#define OUT2_RSET 						GPIOB->BSRR=GPIO_BSRR_BR14; //PB14
#define OUT3_RSET 						GPIOB->BSRR=GPIO_BSRR_BR15; //PB15
#define OUT4_RSET 						GPIOB->BSRR=GPIO_BSRR_BR5; //PB5
#define OUT5_RSET 						GPIOB->BSRR=GPIO_BSRR_BR6; //PB6
#define OUT6_RSET 						GPIOB->BSRR=GPIO_BSRR_BR7; //PB7
#define OUT7_RSET 						GPIOB->BSRR=GPIO_BSRR_BR8; //PB8
#define ENC_LT(x) 		(MyEncoder[POSITION] < MyMemory[x])
#define ENC_GT(x) 		(MyEncoder[POSITION] > MyMemory[x])

//#define ENC_PULSES 8000 //od koliko kvadraturinh pulseva je enkoder
//#define ENC_SYNC_FORWARD (ENC_PULSES/2)-3 //SYNC na 3997
//#define ENC_SYNC_BACKWARD (ENC_PULSES/2)+2 //SYNC na 4002

#define ENC_IMPS_PER_STEP 2
#define ENC_IMPS_PER_STEP_HALF ENC_IMPS_PER_STEP / 2

void TIM2_Setup_ENC (void);
enum {VOLUME, MENU, OVERLAP, DELAY, SLOPE, QEIMODE, IMPSSTEP};


