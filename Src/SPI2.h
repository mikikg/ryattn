//
// Created by miki on 23/04/2021.
//
//FRAM DRIVER
//default je SPI1 port
//Organizacija memorije je 4 byte

//FRAM commands
#define FR_WREN 6
#define FR_WRDI 4
#define FR_RDSR 5
#define FR_WRSR 1
#define FR_READ 3
#define FR_WRITE 2

#define FRAM_SIZE_KBITS 64 //8 Kbytes
#define FRAM_SIZE_WORDS (FRAM_SIZE_KBITS*1024/32) //2048 32bit Words
#define FRAM_BACKUP_ADR (FRAM_SIZE_WORDS/2) //Backup je na pola memorije
#define FRAM_WORDS_TO_BACKUP 64 //toliko trenutno koristimo

//adrese NV registra koje nam trebaju, na kraju memorije
#define NV_TEST 						FRAM_SIZE_WORDS-1 //zadnja adr
#define NV_BOOT_CNT 				FRAM_SIZE_WORDS-2 //prezadnja ...
#define NV_MINUTE_CNT 			FRAM_SIZE_WORDS-3
#define NV_SIGNAL_VAL 		 	FRAM_SIZE_WORDS-4
#define NV_PAUSE_VAL 				FRAM_SIZE_WORDS-5
#define NV_TOTAL_CNT 				FRAM_SIZE_WORDS-6
#define NV_QUICK_BOOT 				FRAM_SIZE_WORDS-7
#define NV_PERMANENT_QUICK_BOOT 				FRAM_SIZE_WORDS-8

//#define USE_FR_SPI1 //uncoment this to enable SPI1 functions
#define USE_FR_SPI2 //uncoment this to enable SPI2 functions

union unionU32 {
	uint32_t 	bit32;
	uint16_t 	bit16[2];
	uint8_t 	bit8[4];
}  ;

#ifdef USE_FR_SPI1
void FR_SPI1_init (void);
void FR_Init_SPI1_pins (void);
uint8_t FR_SPI1_8_transfer (uint8_t data);
uint32_t FR_SPI1_get32bitWord (uint32_t adr);
void FR_SPI1_set32bitWord (uint32_t adr, uint32_t data);
void FR_SPI1_inc32bitWord (uint32_t adr);
#endif

#ifdef USE_FR_SPI2
void SPI2_init (void);
void FR_Init_SPI2_pins (void);
uint8_t FR_SPI2_8_transfer (uint8_t data);
uint32_t FR_SPI2_get32bitWord (uint32_t adr);
void FR_SPI2_set32bitWord (uint32_t adr, uint32_t data);
void FR_SPI2_inc32bitWord (uint32_t adr);
#endif

