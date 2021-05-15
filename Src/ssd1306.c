//
// Created by miki on 2019-04-23.
//
/*

    PB12 CS - OUT
    PB13 CLK - OUT
    PB14 D/C - OUT NORMAL GPIO NOT MISO!
    PB15 MOSI - OUT
    PA8 RES - OUT

*/

#include "ssd1306.h"
#include "stm32f1xx.h"

#define RES_on   GPIOA->BSRR=GPIO_BSRR_BS8;
#define RES_off  GPIOA->BSRR=GPIO_BSRR_BR8;
#define DC_on   GPIOB->BSRR=GPIO_BSRR_BS14;
#define DC_off  GPIOB->BSRR=GPIO_BSRR_BR14;
#define CS_on   GPIOB->BSRR=GPIO_BSRR_BS12;
#define CS_off  GPIOB->BSRR=GPIO_BSRR_BR12;

#define command ssd1306_WriteCommand

#if defined(SSD1306_USE_I2C)

void ssd1306_Reset(void) {
	/* for I2C - do nothing */
}

// Send a byte to the command register
void ssd1306_WriteCommand(uint8_t byte) {
	HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, &byte, 1, HAL_MAX_DELAY);
}

// Send data
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
	HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x40, 1, buffer, buff_size, HAL_MAX_DELAY);
}

#elif defined(SSD1306_USE_SPI)

void ssd1306_Reset(void) {
	// CS = High (not selected)
	HAL_GPIO_WritePin(SSD1306_CS_Port, SSD1306_CS_Pin, GPIO_PIN_SET);

	// Reset the OLED
	HAL_GPIO_WritePin(SSD1306_Reset_Port, SSD1306_Reset_Pin, GPIO_PIN_RESET);
	HAL_Delay(10);
	HAL_GPIO_WritePin(SSD1306_Reset_Port, SSD1306_Reset_Pin, GPIO_PIN_SET);
	HAL_Delay(10);
}

// Send a byte to the command register
void ssd1306_WriteCommand(uint8_t byte) {
    HAL_GPIO_WritePin(SSD1306_CS_Port, SSD1306_CS_Pin, GPIO_PIN_RESET); // select OLED
    HAL_GPIO_WritePin(SSD1306_DC_Port, SSD1306_DC_Pin, GPIO_PIN_RESET); // command
    HAL_SPI_Transmit(&SSD1306_SPI_PORT, (uint8_t *) &byte, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SSD1306_CS_Port, SSD1306_CS_Pin, GPIO_PIN_SET); // un-select OLED
}

// Send data
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
    HAL_GPIO_WritePin(SSD1306_CS_Port, SSD1306_CS_Pin, GPIO_PIN_RESET); // select OLED
    HAL_GPIO_WritePin(SSD1306_DC_Port, SSD1306_DC_Pin, GPIO_PIN_SET); // data
    HAL_SPI_Transmit(&SSD1306_SPI_PORT, buffer, buff_size, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SSD1306_CS_Port, SSD1306_CS_Pin, GPIO_PIN_SET); // un-select OLED
}

#else
//#error "You should define SSD1306_USE_SPI or SSD1306_USE_I2C macro"
#endif

void initPinsSD1306() {

    //NEW!!!!
    //+ PB12 CS - OUT
    //+ PB13 CLK - OUT
    // PB14 D/C - OUT NORMAL GPIO NOT MISO!
    //+ PB15 MOSI - OUT
    // PA8 RES - OUT

    //--------------- PB12 / CS -------------
    //GP function Output Push-Pull
    GPIOB->CRH &= ~GPIO_CRH_CNF12_0; //ocisti bit CNF12_0
    GPIOB->CRH &= ~GPIO_CRH_CNF12_1; //ocisti bit CNF12_1
    //Output MODE 50MHz
    GPIOB->CRH |=  GPIO_CRH_MODE12_0; //setuj bit MODE12_0
    GPIOB->CRH |=  GPIO_CRH_MODE12_1; //setuj bit MODE12_1
    GPIOB->BSRR=GPIO_BSRR_BR12; //PB12 = LOW

    //--------------- PB13 / CLK -------------
    //Alt function Output Push-Pull
    GPIOB->CRH &= ~GPIO_CRH_CNF13_0; //ocisti bit CNF13_0
    GPIOB->CRH |=	 GPIO_CRH_CNF13_1; //setuj bit CNF13_1
    //Output MODE 50MHz
    GPIOB->CRH |=  GPIO_CRH_MODE13_0; //setuj bit MODE13_0
    GPIOB->CRH |=  GPIO_CRH_MODE13_1; //setuj bit MODE13_1

    //--------------- PB14 / DC -------------
    //GP function Output Push-Pull
    GPIOB->CRH &= ~GPIO_CRH_CNF14_0; //ocisti bit CNF14_0
    GPIOB->CRH &= ~GPIO_CRH_CNF14_1; //ocisti bit CNF14_1
    //Output MODE 50MHz
    GPIOB->CRH |=  GPIO_CRH_MODE14_0; //setuj bit MODE14_0
    GPIOB->CRH |=  GPIO_CRH_MODE14_1; //setuj bit MODE14_1
    GPIOB->BSRR=GPIO_BSRR_BR14; //PB14 = LOW

    //--------------- PB15 / MOSI -------------
    //Alt function Output Push-Pull
    GPIOB->CRH &= ~GPIO_CRH_CNF15_0; //ocisti bit CNF15_0
    GPIOB->CRH |=	 GPIO_CRH_CNF15_1; //setuj bit CNF15_1
    //Output MODE 50MHz
    GPIOB->CRH |=  GPIO_CRH_MODE15_0; //setuj bit MODE15_0
    GPIOB->CRH |=  GPIO_CRH_MODE15_1; //setuj bit MODE15_1

    //--------------- PA8 / RES -------------
    // GP function Output Push-Pull
    GPIOA->CRH &= ~GPIO_CRH_CNF8_0; //ocisti bit CNF8_0
    GPIOA->CRH &= ~GPIO_CRH_CNF8_1; //ocisti bit CNF8_1
    //Output MODE 50MHz
    GPIOA->CRH |=  GPIO_CRH_MODE8_0; //setuj bit MODE8_0
    GPIOA->CRH |=  GPIO_CRH_MODE8_1; //setuj bit MODE8_1


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

    //treba nam delay ovde dok se ne smiri napajanje
    //for (int x = 0; x < 100000; x++) {GPIOC->BSRR=GPIO_BSRR_BR13;}

}

/*
 *
#define RES_on   GPIOB->BSRR=GPIO_BSRR_BS1;
#define RES_off  GPIOB->BSRR=GPIO_BSRR_BR1;
#define DC_on   GPIOB->BSRR=GPIO_BSRR_BS0;
#define DC_off  GPIOB->BSRR=GPIO_BSRR_BR0;
#define CS_on   GPIOB->BSRR=GPIO_BSRR_BS12;
#define CS_off  GPIOB->BSRR=GPIO_BSRR_BR12;
 * */

void ssd1306_Reset(void) {
    /*
    // CS = High (not selected)
    HAL_GPIO_WritePin(SSD1306_CS_Port, SSD1306_CS_Pin, GPIO_PIN_SET);

    // Reset the OLED
    HAL_GPIO_WritePin(SSD1306_Reset_Port, SSD1306_Reset_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(SSD1306_Reset_Port, SSD1306_Reset_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
     */
    CS_on;
    for (int x = 0; x < 100; x++) { RES_off; }
    for (int x = 0; x < 100; x++) { RES_on; }

}

// Send a byte to the command register
void ssd1306_WriteCommand(uint8_t byte) {
    //HAL_GPIO_WritePin(SSD1306_CS_Port, SSD1306_CS_Pin, GPIO_PIN_RESET); // select OLED
    //HAL_GPIO_WritePin(SSD1306_DC_Port, SSD1306_DC_Pin, GPIO_PIN_RESET); // command
    //HAL_SPI_Transmit(&SSD1306_SPI_PORT, (uint8_t *) &byte, 1, HAL_MAX_DELAY);
    //HAL_GPIO_WritePin(SSD1306_CS_Port, SSD1306_CS_Pin, GPIO_PIN_SET); // un-select OLED

    CS_off;
    DC_off;
    //
    //cekaj dok TX buffer ne bude prazan
    while (! (SPI2->SR | SPI_SR_TXE) );

    //postavi podatak u data registar za prenos
    SPI2->DR = byte;

    //cekaj dok se to prenese
    while (! (SPI2->SR & SPI_SR_RXNE) );
    //while (! (SPI2->SR | SPI_SR_TXE) );
    //while (  (SPI2->SR | SPI_SR_BSY) );
    //while (!  (SPI2->SR | SPI_SR_RXNE) );

    //vrati podatak koji je iscitan
    //return SPI2->DR;

    for (int y = 0; y < 10; y++) { CS_on; }

}

// Send data
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
    //HAL_GPIO_WritePin(SSD1306_CS_Port, SSD1306_CS_Pin, GPIO_PIN_RESET); // select OLED
    //HAL_GPIO_WritePin(SSD1306_DC_Port, SSD1306_DC_Pin, GPIO_PIN_SET); // data
    //HAL_SPI_Transmit(&SSD1306_SPI_PORT, buffer, buff_size, HAL_MAX_DELAY);
    //HAL_GPIO_WritePin(SSD1306_CS_Port, SSD1306_CS_Pin, GPIO_PIN_SET); // un-select OLED
    CS_off;
    DC_on;
    //
    for (int x=0; x<buff_size; x++) {
        //cekaj dok TX buffer ne bude prazan
        while (! (SPI2->SR | SPI_SR_TXE) );

        //postavi podatak u data registar za prenos
        SPI2->DR = buffer[x];

        //cekaj dok se to prenese
        while (! (SPI2->SR & SPI_SR_RXNE) );

    }

    for (int y = 0; y < 10; y++) { CS_on; }

}


// Screenbuffer
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// Screen object
static SSD1306_t SSD1306;

void ssd1306_TestFonts() {
    ssd1306_Fill(Black);
    ssd1306_SetCursor(2, 0);
    ssd1306_WriteString("Font 16x26", Font_16x26, White);
    ssd1306_SetCursor(2, 26);
    ssd1306_WriteString("Font 11x18", Font_11x18, White);
    ssd1306_SetCursor(2, 26+18);
    ssd1306_WriteString("Font 7x10", Font_7x10, White);
    ssd1306_UpdateScreen();
}

// Initialize the oled screen
void ssd1306_Init(void) {

    initPinsSD1306();

    // Reset OLED
    ssd1306_Reset();

    // Wait for the screen to boot
    for (int x = 0; x < 100; x++) { RES_on; }

    // Init OLED
    ssd1306_WriteCommand(0xAE); //display off

    ssd1306_WriteCommand(0x20); //Set Memory Addressing Mode
    ssd1306_WriteCommand(0x10); // 00,Horizontal Addressing Mode; 01,Vertical Addressing Mode;
    // 10,Page Addressing Mode (RESET); 11,Invalid

    ssd1306_WriteCommand(0xB0); //Set Page Start Address for Page Addressing Mode,0-7

#ifdef SSD1306_MIRROR_VERT
    ssd1306_WriteCommand(0xC0); // Mirror vertically
#else
    ssd1306_WriteCommand(0xC8); //Set COM Output Scan Direction
#endif

    ssd1306_WriteCommand(0x00); //---set low column address
    ssd1306_WriteCommand(0x10); //---set high column address

    ssd1306_WriteCommand(0x40); //--set start line address - CHECK

    ssd1306_WriteCommand(0x81); //--set contrast control register - CHECK
    ssd1306_WriteCommand(0xFF);

#ifdef SSD1306_MIRROR_HORIZ
    ssd1306_WriteCommand(0xA0); // Mirror horizontally
#else
    ssd1306_WriteCommand(0xA1); //--set segment re-map 0 to 127 - CHECK
#endif

#ifdef SSD1306_INVERSE_COLOR
    ssd1306_WriteCommand(0xA7); //--set inverse color
#else
    ssd1306_WriteCommand(0xA6); //--set normal color
#endif

    ssd1306_WriteCommand(0xA8); //--set multiplex ratio(1 to 64) - CHECK
    ssd1306_WriteCommand(0x3F); //

    ssd1306_WriteCommand(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content

    ssd1306_WriteCommand(0xD3); //-set display offset - CHECK
    ssd1306_WriteCommand(0x00); //-not offset

    ssd1306_WriteCommand(0xD5); //--set display clock divide ratio/oscillator frequency
    ssd1306_WriteCommand(0xF0); //--set divide ratio

    ssd1306_WriteCommand(0xD9); //--set pre-charge period
    ssd1306_WriteCommand(0x22); //

    ssd1306_WriteCommand(0xDA); //--set com pins hardware configuration - CHECK
    ssd1306_WriteCommand(0x12);

    ssd1306_WriteCommand(0xDB); //--set vcomh
    ssd1306_WriteCommand(0x20); //0x20,0.77xVcc

    ssd1306_WriteCommand(0x8D); //--set DC-DC enable
    ssd1306_WriteCommand(0x14); //
    ssd1306_WriteCommand(0xAF); //--turn on SSD1306 panel

    // Clear screen
    ssd1306_Fill(Black);

    // Flush buffer to screen
    ssd1306_UpdateScreen();

    // Set default values for screen object
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;

    SSD1306.Initialized = 1;
}

// Fill the whole screen with the given color
void ssd1306_Fill(SSD1306_COLOR color) {
    /* Set memory */
    uint32_t i;

    for(i = 0; i < sizeof(SSD1306_Buffer); i++) {
        SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
    }
}

// Write the screenbuffer with changed to the screen
void ssd1306_UpdateScreen(void) {
    uint8_t i;
    for(i = 0; i < 8; i++) {
        ssd1306_WriteCommand(0xB0 + i);
        ssd1306_WriteCommand(0x00);
        ssd1306_WriteCommand(0x10);
        ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH*i],SSD1306_WIDTH);
    }
}

//    Draw one pixel in the screenbuffer
//    X => X Coordinate
//    Y => Y Coordinate
//    color => Pixel color
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        // Don't write outside the buffer
        return;
    }

    // Check if pixel should be inverted
    if(SSD1306.Inverted) {
        color = (SSD1306_COLOR)!color;
    }

    // Draw in the right color
    if(color == White) {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

// Draw 1 char to the screen buffer
// ch         => char om weg te schrijven
// Font     => Font waarmee we gaan schrijven
// color     => Black or White
char ssd1306_WriteChar(char ch, FontDef Font, SSD1306_COLOR color) {
    uint32_t i, b, j;

    // Check remaining space on current line
    if (SSD1306_WIDTH <= (SSD1306.CurrentX + Font.FontWidth) ||
        SSD1306_HEIGHT <= (SSD1306.CurrentY + Font.FontHeight))
    {
        // Not enough space on current line
        return 0;
    }

    // Use the font to write
    for(i = 0; i < Font.FontHeight; i++) {
        b = Font.data[(ch - 32) * Font.FontHeight + i];
        for(j = 0; j < Font.FontWidth; j++) {
            if((b << j) & 0x8000)  {
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR) color);
            } else {
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
            }
        }
    }

    // The current space is now taken
    SSD1306.CurrentX += Font.FontWidth;

    // Return written char for validation
    return ch;
}

// Write full string to screenbuffer
char ssd1306_WriteString(char* str, FontDef Font, SSD1306_COLOR color) {
    // Write until null-byte
    while (*str) {
        if (ssd1306_WriteChar(*str, Font, color) != *str) {
            // Char could not be written
            return *str;
        }
        // Next char
        str++;
    }
    // Everything ok
    return *str;
}

// Position the cursor
void ssd1306_SetCursor(uint8_t x, uint8_t y) {
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}
