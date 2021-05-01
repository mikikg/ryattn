/*

Nativni (CMSIS) drajver za FRAM memoriju preko SPI porta za STM32F103C8

SPI1 je 2x brzi sa istim podesavanjaima zbog Clock

SPI1 pinovi:
	
	PA4 - NSS1 * (OUT)
	PA5 - SCK1 (OUT)
	PA6 - MISO1 (IN)
	PA7 - MOSI1 (OUT)

SPI2 pinovi:
	
	PB12 - NSS2 * (OUT)
	PB13 - SCK2 (OUT)
	PB14 - MISO2 (IN)
	PB15 - MOSI2 (OUT)

* Chip Select je externi bilo koji pin koji se kontrolise iz SW

	FR_SPI1_set32bitWord (0, 0x12345678);
	statusx = FR_SPI1_get32bitWord (0);

	Adreseiranje/organizacija memorije po 4bytes tj Words (32bit)

*/

#include "stm32f1xx.h"
#include "SPI2.h"

extern char My7segBUFF[6];

union unionU32 U32a;
int boots;

//------------------------------------------------ SPI1 -------------------------
#ifdef USE_FR_SPI1

//ovo se prvo poziva za inicializaciju SPI1
void FR_SPI1_init (void) {

	//init pins
	FR_Init_SPI1_pins();
	
	//Dodeli clock za SPI1 koji ide preko APB2
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

	//RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	//RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
	
	//Resetuj SPI1 modul
	RCC->APB2RSTR |= 	RCC_APB2RSTR_SPI1RST;
	//Vrati iz reseta
	RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;
	
	//----- konfiguracija SPI1 porta ------------
	SPI1->CR1  = RESET;
	//SPI1->CR1 &= ~SPI_CR1_BIDIMODE;  //0: 2-line unidirectional data mode selected
	//SPI1->CR1 &= ~SPI_CR1_BIDIOE;  //0: Output disabled (receive-only mode) 
	//SPI1->CR1 &= ~SPI_CR1_CRCEN; //0: CRC calculation disabled
	//SPI1->CR1 &= ~SPI_CR1_CRCNEXT; //0: Data phase (no CRC phase) 
	//SPI1->CR1 &= ~SPI_CR1_DFF; //0: 8-bit data frame format is selected for tx/rx
	//SPI1->CR1 &= ~SPI_CR1_RXONLY; //0: Full duplex (Transmit and receive)
	SPI1->CR1 |=  SPI_CR1_SSM;  //1: Software slave management enabled
	SPI1->CR1 |=  SPI_CR1_SSI;  //1: This bit has an effect only when the SSM bit is set.
	//SPI1->CR1 &= ~SPI_CR1_LSBFIRST;  //0: MSB transmitted first
	SPI1->CR1 |=  SPI_CR1_BR_1; // 010 =  fPCLK/8
	SPI1->CR1 |=  SPI_CR1_MSTR; // 1: Master configuration
	//SPI1->CR1 &= ~SPI_CR1_CPOL; // Clock polarity 0: CK to 0 when idle
	//SPI1->CR1 &= ~SPI_CR1_CPHA ; // Clock phase 0: The first clock transition is the first data capture edge
	
	SPI1->CR1 |=  SPI_CR1_SPE;  //1: SPI Peripheral enabled
	
	//treba nam delay ovde dok se ne smiri napajanje zbog FRAM
	for (int x = 0; x < 10000; x++) {GPIOC->BSRR=GPIO_BSRR_BR13;}
	
	//proveri prilikom inicializacije da li je FRAM memorija ziva
	//testiraj zadnjih 4 bajtova memorije
	//upisi/iscitaj prvo 0 pa onda 0x12345678
	int fram_err;
	fram_err = 1;
	FR_SPI1_set32bitWord (NV_TEST, 0x000000);
	if (FR_SPI1_get32bitWord (NV_TEST) == 0) {
		//ok prvi test, uradi drugi
		FR_SPI1_set32bitWord (NV_TEST, 0x4D494B49);
		if (FR_SPI1_get32bitWord (NV_TEST) == 0x4D494B49) {
			//sve OK
			fram_err = 0;
		} 
	}
	if (fram_err) {
		//imamo gresku sa FRAM, zaglavi se ovde i brzo blinkaj PC13
		while (1){
			for (int x = 0; x < 300000; x++) {GPIOC->BSRR=GPIO_BSRR_BS13;}
			for (int x = 0; x < 300000; x++) {GPIOC->BSRR=GPIO_BSRR_BR13;}
		}
	}
	
	//inkrementuj brojac na svaki reset na predzadnjoj adresi memorije
	FR_SPI1_inc32bitWord (NV_BOOT_CNT);
	
	
}

//inicializuj pinove za SPI1
void FR_Init_SPI1_pins (void) {
	
	//PA4 - NSS1 * (OUT)
	//PA5 - SCK1 (OUT)
	//PA6 - MISO1 (IN)
	//PA7 - MOSI1 (OUT)

	//--------------- PA4 / NSS1 -------------
	//GP function Output Push-Pull
	GPIOA->CRL &= ~GPIO_CRL_CNF4_0; //ocisti bit CNF4_0
	GPIOA->CRL &= ~GPIO_CRL_CNF4_1; //ocisti bit CNF4_1
	//Output MODE 50MHz
	GPIOA->CRL |=  GPIO_CRL_MODE4_0; //setuj bit MODE4_0
	GPIOA->CRL |=  GPIO_CRL_MODE4_1; //setuj bit MODE4_1
	GPIOA->BSRR=GPIO_BSRR_BR4; //PA4 = LOW
	
	//--------------- PA5 / SCK1 -------------
	//Alt function Output Push-Pull
	GPIOA->CRL &= ~GPIO_CRL_CNF5_0; //ocisti bit CNF5_0
	GPIOA->CRL |=	 GPIO_CRL_CNF5_1; //setuj bit CNF5_1
	//Output MODE 50MHz
	GPIOA->CRL |=  GPIO_CRL_MODE5_0; //setuj bit MODE5_0
	GPIOA->CRL |=  GPIO_CRL_MODE5_1; //setuj bit MODE5_1
	
	//--------------- PA6 / MISO4 %%%%%%%-------------
	//Input mode
	GPIOA->CRL &= ~GPIO_CRL_CNF6_0; //ocisti bit CNF6_0
	GPIOA->CRL |=  GPIO_CRL_CNF6_1; //setuj bit CNF6_1
	//Input with pull-up / pull-down
	GPIOA->CRL &= ~GPIO_CRL_MODE6_0; //ocisti bit MODE6_0
	GPIOA->CRL &= ~GPIO_CRL_MODE6_1; //ocisti bit MODE6_1
	//Pull up (preko ODR se podesava!)
	GPIOA->ODR |= GPIO_ODR_ODR6; //setuj bit = Pull-up

	//--------------- PB7 / MOSI1 -------------
	//Alt function Output Push-Pull
	GPIOA->CRL &= ~GPIO_CRL_CNF7_0; //ocisti bit CNF7_0
	GPIOA->CRL |=	 GPIO_CRL_CNF7_1; //setuj bit CNF7_1
	//Output MODE 50MHz
	GPIOA->CRL |=  GPIO_CRL_MODE7_0; //setuj bit MODE7_0
	GPIOA->CRL |=  GPIO_CRL_MODE7_1; //setuj bit MODE7_1
}


//make transfer (send and receive) via SPI
uint8_t FR_SPI1_8_transfer (uint8_t data) {

		//cekaj dok TX buffer ne bude prazan
		while (! (SPI1->SR | SPI_SR_TXE) );
		
		//postavi podatak u data registar za prenos
		SPI1->DR = data;
	
		//cekaj dok se to prenese
		while (! (SPI1->SR & SPI_SR_RXNE) );	
		//while (! (SPI1->SR & SPI_SR_TXE) );
		//while (!  (SPI1->SR & SPI_SR_BSY) );
		//while (!  (SPI1->SR & SPI_SR_RXNE) );
		
		//vrati podatak koji je iscitan
		return SPI1->DR;
}


//read FRAM SPI1
uint32_t FR_SPI1_get32bitWord (uint32_t adr) {
	
	adr*=4; //Words adrres mode
	
	GPIOA->BSRR=GPIO_BSRR_BR4; //PA4 = LOW
	FR_SPI1_8_transfer (FR_READ); //cmd
	//FR_SPI1_8_transfer (adr >> 16); //24bit adr, hi
	FR_SPI1_8_transfer (adr >> 8); //mid
	FR_SPI1_8_transfer (adr); //low
	
	U32a.bit32 = 0;
	U32a.bit8[3] = FR_SPI1_8_transfer (0); //hi
	U32a.bit8[2] = FR_SPI1_8_transfer (0);
	U32a.bit8[1] = FR_SPI1_8_transfer (0);
	U32a.bit8[0] = FR_SPI1_8_transfer (0); //lo
	
	GPIOA->BSRR=GPIO_BSRR_BS4; //PA4 = HIGH
	
	return U32a.bit32;
}

//write FRAM SPI1
void FR_SPI1_set32bitWord (uint32_t adr, uint32_t data) {

	adr*=4; //Words adrres mode
	
	//set WREN
	GPIOA->BSRR=GPIO_BSRR_BR4; //PA4 = LOW
	FR_SPI1_8_transfer (FR_WREN); //cmd
	GPIOA->BSRR=GPIO_BSRR_BS4; //PA4 = HIGH
	
	//start transfer
	GPIOA->BSRR=GPIO_BSRR_BR4; //PA4 = LOW
	FR_SPI1_8_transfer (FR_WRITE); //cmd
	//FR_SPI1_8_transfer (adr >> 16); //24bit adr, hi
	FR_SPI1_8_transfer (adr >> 8); //mid
	FR_SPI1_8_transfer (adr); //low
	
	//transfer data 4 x byte
	U32a.bit32 = data;
	FR_SPI1_8_transfer (U32a.bit8[3]); //hi
	FR_SPI1_8_transfer (U32a.bit8[2]);
	FR_SPI1_8_transfer (U32a.bit8[1]);
	FR_SPI1_8_transfer (U32a.bit8[0]); //lo
	
	//end transfer
	GPIOA->BSRR=GPIO_BSRR_BS4; //PA4 = HIGH
	
	//clear WREN
	GPIOA->BSRR=GPIO_BSRR_BR4; //PA4 = LOW
	FR_SPI1_8_transfer (FR_WRDI); //cmd
	GPIOA->BSRR=GPIO_BSRR_BS4; //PA4 = HIGH
	
}

//increment data at some adress (couter)
void FR_SPI1_inc32bitWord (uint32_t adr) {
	FR_SPI1_set32bitWord (adr, FR_SPI1_get32bitWord (adr)+1);
}

#endif

//------------------------------------------------ SPI2 -------------------------
#ifdef USE_FR_SPI2

//ovo se prvo poziva za inicializaciju SPI2
void SPI2_init (void) {

	//init pins
	FR_Init_SPI2_pins();
	
	//Dodeli clock za SPI2 koji ide preko APB1
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

	//RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	//RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
	
	//Resetuj SPI2 modul
	RCC->APB1RSTR |= 	RCC_APB1RSTR_SPI2RST;
	//Vrati iz reseta
	RCC->APB1RSTR &= ~RCC_APB1RSTR_SPI2RST;
	
	//----- konfiguracija SPI porta ------------
	SPI2->CR1  = RESET;
	//SPI2->CR1 &= ~SPI_CR1_BIDIMODE;  //0: 2-line unidirectional data mode selected
	//SPI2->CR1 &= ~SPI_CR1_BIDIOE;  //0: Output disabled (receive-only mode) 
	//SPI2->CR1 &= ~SPI_CR1_CRCEN; //0: CRC calculation disabled
	//SPI2->CR1 &= ~SPI_CR1_CRCNEXT; //0: Data phase (no CRC phase) 
	//SPI2->CR1 &= ~SPI_CR1_DFF; //0: 8-bit data frame format is selected for tx/rx
	//SPI2->CR1 &= ~SPI_CR1_RXONLY; //0: Full duplex (Transmit and receive)
	SPI2->CR1 |=  SPI_CR1_SSM;  //1: Software slave management enabled
	SPI2->CR1 |=  SPI_CR1_SSI;  //1: This bit has an effect only when the SSM bit is set.
	//SPI2->CR1 &= ~SPI_CR1_LSBFIRST;  //0: MSB transmitted first
	SPI2->CR1 |=  SPI_CR1_BR_0; // 001 =  fPCLK/4
	SPI2->CR1 |=  SPI_CR1_MSTR; // 1: Master configuration
	//SPI2->CR1 &= ~SPI_CR1_CPOL; // Clock polarity 0: CK to 0 when idle
	//SPI2->CR1 &= ~SPI_CR1_CPHA ; // Clock phase 0: The first clock transition is the first data capture edge
	
	SPI2->CR1 |=  SPI_CR1_SPE;  //1: SPI Peripheral enabled
	
	//treba nam delay ovde dok se ne smiri napajanje zbog FRAM
	for (int x = 0; x < 10001; x++) {GPIOC->BSRR=GPIO_BSRR_BR13;}
	
	//proveri prilikom inicializacije da li je FRAM memorija ziva
	//testiraj zadnjih 4 bajtova memorije
	//upisi/iscitaj prvo 0 pa onda 0x12345678
	int fram_err;
	fram_err = 1;
	FR_SPI2_set32bitWord (NV_TEST, 0x000000);
	if (FR_SPI2_get32bitWord (NV_TEST) == 0) {
		//ok prvi test, uradi drugi
		FR_SPI2_set32bitWord (NV_TEST, 0x4D494B49);
		if (FR_SPI2_get32bitWord (NV_TEST) == 0x4D494B49) {
			//sve OK
			fram_err = 0;
		} 
	}
	if (fram_err) {
		//imamo gresku sa FRAM, zaglavi se ovde i brzo blinkaj PC13
        //My7segBUFF[0]='F';
        //My7segBUFF[1]='r';
        //My7segBUFF[2]='E';
        //My7segBUFF[3]='r';
        //My7segBUFF[4]='r';
		
		while (1){
			boots++; //samo koristimo privremeno kao tmp za blinkanje
			for (int x = 0; x < 8000; x++) {
				if (boots > 0 && boots < 15) {
					GPIOC->BSRR=GPIO_BSRR_BS13;
				}
			}
			for (int x = 0; x < 8000; x++) {
				if (boots > 15 && boots < 30) {
					GPIOC->BSRR=GPIO_BSRR_BR13;
				}
			}
			
			if (boots > 30) boots = 0;
			
			//handler_led();
		}
	}
	
	//inkrementuj brojac na svaki reset na predzadnjoj adresi memorije
	FR_SPI2_inc32bitWord (NV_BOOT_CNT);
	
	//for debugging
	//boots = FR_SPI2_get32bitWord (NV_BOOT_CNT);
	//boots ++;
	
	
}



void FR_Init_SPI2_pins (void) {
	
	//PB12 - NSS2 * (OUT)
	//PB13 - SCK2 (OUT)
	//PB14 - MISO2 (IN)
	//PB15 - MOSI2 (OUT)

	//--------------- PB12 / NSS2 -------------
	//GP function Output Push-Pull
	GPIOB->CRH &= ~GPIO_CRH_CNF12_0; //ocisti bit CNF12_0
	GPIOB->CRH &= ~GPIO_CRH_CNF12_1; //ocisti bit CNF12_1
	//Output MODE 50MHz
	GPIOB->CRH |=  GPIO_CRH_MODE12_0; //setuj bit MODE12_0
	GPIOB->CRH |=  GPIO_CRH_MODE12_1; //setuj bit MODE12_1
	GPIOB->BSRR=GPIO_BSRR_BR12; //PB12 = LOW
	
	//--------------- PB13 / SCK2 -------------
	//Alt function Output Push-Pull
	GPIOB->CRH &= ~GPIO_CRH_CNF13_0; //ocisti bit CNF13_0
	GPIOB->CRH |=	 GPIO_CRH_CNF13_1; //setuj bit CNF13_1
	//Output MODE 50MHz
	GPIOB->CRH |=  GPIO_CRH_MODE13_0; //setuj bit MODE13_0
	GPIOB->CRH |=  GPIO_CRH_MODE13_1; //setuj bit MODE13_1
	
	//--------------- PB14 / MISO2 %%%%%%%-------------
	//Input mode
	GPIOB->CRH &= ~GPIO_CRH_CNF14_0; //ocisti bit CNF14_0
	GPIOB->CRH |=  GPIO_CRH_CNF14_1; //setuj bit CNF14_1
	//Input with pull-up / pull-down
	GPIOB->CRH &= ~GPIO_CRH_MODE14_0; //ocisti bit MODE14_0
	GPIOB->CRH &= ~GPIO_CRH_MODE14_1; //ocisti bit MODE14_1
	//Pull up (preko ODR se podesava!)
	GPIOB->ODR |= GPIO_ODR_ODR14; //setuj bit = Pull-up

	//--------------- PB15 / MOSI2 -------------
	//Alt function Output Push-Pull
	GPIOB->CRH &= ~GPIO_CRH_CNF15_0; //ocisti bit CNF15_0
	GPIOB->CRH |=	 GPIO_CRH_CNF15_1; //setuj bit CNF15_1
	//Output MODE 50MHz
	GPIOB->CRH |=  GPIO_CRH_MODE15_0; //setuj bit MODE15_0
	GPIOB->CRH |=  GPIO_CRH_MODE15_1; //setuj bit MODE15_1
}


//make transfer (send and receive) via SPI
uint8_t FR_SPI2_8_transfer (uint8_t data) {

		//cekaj dok TX buffer ne bude prazan
		while (! (SPI2->SR | SPI_SR_TXE) );
		
		//postavi podatak u data registar za prenos
		SPI2->DR = data;
	
		//cekaj dok se to prenese
		while (! (SPI2->SR & SPI_SR_RXNE) );		
		//while (! (SPI2->SR | SPI_SR_TXE) );
		//while (  (SPI2->SR | SPI_SR_BSY) );
		//while (!  (SPI2->SR | SPI_SR_RXNE) );
		
		//vrati podatak koji je iscitan
		return SPI2->DR;
}


//read FRAM SPI2
uint32_t FR_SPI2_get32bitWord (uint32_t adr) {
	
	adr*=4; //Words adrres mode
	
	union unionU32 U32a;
	U32a.bit32 = 0;
	
	GPIOB->BSRR=GPIO_BSRR_BR12; //PB12 = LOW
	FR_SPI2_8_transfer (FR_READ); //cmd
	//FR_SPI2_8_transfer (adr >> 16); //24bit adr, hi
	FR_SPI2_8_transfer (adr >> 8); //mid
	FR_SPI2_8_transfer (adr); //low
	
	U32a.bit8[3] = FR_SPI2_8_transfer (0); //hi
	U32a.bit8[2] = FR_SPI2_8_transfer (0);
	U32a.bit8[1] = FR_SPI2_8_transfer (0);
	U32a.bit8[0] = FR_SPI2_8_transfer (0); //lo
	
	GPIOB->BSRR=GPIO_BSRR_BS12; //PB12 = HIGH
	
	return U32a.bit32;
}

//increment data at some adress (couter)
void FR_SPI2_inc32bitWord (uint32_t adr) {
	FR_SPI2_set32bitWord (adr, FR_SPI2_get32bitWord (adr)+1);
}

//write FRAM SPI2
void FR_SPI2_set32bitWord (uint32_t adr, uint32_t data) {

	adr*=4; //Words adrres mode
	
	union unionU32 U32a;
	U32a.bit32 = data;
	
	//set WREN
	GPIOB->BSRR=GPIO_BSRR_BR12; //PB12 = LOW
	FR_SPI2_8_transfer (FR_WREN); //cmd
	GPIOB->BSRR=GPIO_BSRR_BS12; //PB12 = HIGH
	
	//start transfer
	GPIOB->BSRR=GPIO_BSRR_BR12; //PB12 = LOW
	FR_SPI2_8_transfer (FR_WRITE); //cmd
	//FR_SPI2_8_transfer (adr >> 16); //24bit adr, hi
	FR_SPI2_8_transfer (adr >> 8); //mid
	FR_SPI2_8_transfer (adr); //low
	
	//data 4 x byte
	FR_SPI2_8_transfer (U32a.bit8[3]); //hi
	FR_SPI2_8_transfer (U32a.bit8[2]);
	FR_SPI2_8_transfer (U32a.bit8[1]);
	FR_SPI2_8_transfer (U32a.bit8[0]); //lo
	
	//end transfer
	GPIOB->BSRR=GPIO_BSRR_BS12; //PB12 = HIGH
	
	//clear WREN
	GPIOB->BSRR=GPIO_BSRR_BR12; //PB12 = LOW
	FR_SPI2_8_transfer (FR_WRDI); //cmd
	GPIOB->BSRR=GPIO_BSRR_BS12; //PB12 = HIGH
	
	
}
#endif



