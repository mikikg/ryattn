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

#include <stdbool.h>
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

volatile bool is_screen_sleeping = false;  // Praćenje da li je ekran u sleep modu

//------------------crc32------------------------
// Define the base address of the CRC module
#define CRC_BASE_ADDR (0x40023000UL)  // Base address for the CRC peripheral module (STM32F1)

// Register definitions (low-level access)
#define CRC_DR     (*(volatile uint32_t *)(CRC_BASE_ADDR + 0x00))  // Data Register (DR)
#define CRC_IDR    (*(volatile uint32_t *)(CRC_BASE_ADDR + 0x04))  // Independent Data Register (IDR)
#define CRC_CR     (*(volatile uint32_t *)(CRC_BASE_ADDR + 0x08))  // Control Register (CR)

static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static uint32_t previous_buffer_crc = 0;

// Function to initialize the CRC hardware
void CRC_Init(void) {
    // Enable clock for the CRC module
    RCC->AHBENR |= RCC_AHBENR_CRCEN;  // Enable CRC clock
    // Reset the CRC module
    CRC_CR |= 1U;  // Set the RESET bit in CRC_CR to reset the CRC unit
}

// Function to calculate the CRC for the screen buffer
uint32_t calculate_crc32_hardware(const uint8_t *data, size_t length) {
    CRC_CR |= 1U;  // Set the RESET bit to reset the CRC accumulator
    // Add data to the CRC register, processing 32-bit words
    for (size_t i = 0; i < length; i += 4) {
        CRC_DR = *((uint32_t*)(data + i));;  // Write data to the CRC Data Register
    }
    return CRC_DR;  // Return the CRC result
}
//-------------

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

void ssd1306_Reset(void) {
    CS_on;
    for (int x = 0; x < 100; x++) { RES_off; }
    for (int x = 0; x < 100; x++) { RES_on; }
}

// Send a byte to the command register
void ssd1306_WriteCommand(uint8_t byte) {
    CS_off;
    DC_off;

    //cekaj dok TX buffer ne bude prazan
    while (! (SPI2->SR | SPI_SR_TXE) );

    //postavi podatak u data registar za prenos
    SPI2->DR = byte;

    //cekaj dok se to prenese
    while (! (SPI2->SR & SPI_SR_RXNE) );

    //vrati podatak koji je iscitan
    //return SPI2->DR;

    for (int y = 0; y < 10; y++) { CS_on; }
}

// Send data
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
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

void ssd1306_Init(void) {

    CRC_Init(); // for screen buffer change update

    initPinsSD1306();
    ssd1306_Reset();
    for (int x = 0; x < 100; x++) { RES_on; }

    uint8_t init_sequence[] = {
            0xAE, // Display off
            0x20, // Set Memory Addressing Mode
            0x10, // Set Horizontal Addressing Mode
            0xB0, // Set Page Start Address for Page Addressing Mode, 0-7
#ifdef SSD1306_MIRROR_VERT
            0xC0, // Mirror vertically
#else
            0xC8, // Set COM Output Scan Direction
#endif
            0x00, // Set low column address
            0x10, // Set high column address
            0x40, // Set start line address
            0x81, // Set contrast control register
            0xFF, // Maximum contrast
#ifdef SSD1306_MIRROR_HORIZ
            0xA0, // Mirror horizontally
#else
            0xA1, // Set segment re-map 0 to 127
#endif

#ifdef SSD1306_INVERSE_COLOR
            0xA7, // Set inverse color
#else
            0xA6, // Set normal color
#endif
            0xA8, // Set multiplex ratio (1 to 64)
            0x3F, // 1/64 duty
            0xA4, // Output follows RAM content (normal display)
            0xD3, // Set display offset
            0x00, // No offset
            0xD5, // Set display clock divide ratio/oscillator frequency
            0xF0, // Set divide ratio
            0xD9, // Set pre-charge period
            0x22, // Pre-charge period
            0xDA, // Set com pins hardware configuration
            0x12, // Set com pins configuration value
            0xDB, // Set Vcomh
            0x20, // 0.77xVcc
            0x8D, // Set DC-DC enable
            0x14, // Enable charge pump
            0xAF  // Turn on SSD1306 panel
    };

    for (uint8_t i = 0; i < sizeof(init_sequence); i++) {
        ssd1306_WriteCommand(init_sequence[i]);
    }

    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();

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

// Write the screenbuffer with changes to the screen only if changed by crc32
//void ssd1306_UpdateScreen(void) {
//    uint32_t current_crc = calculate_crc32_hardware(SSD1306_Buffer, sizeof(SSD1306_Buffer));
//
//    // Compare the current CRC with the previous one
//    if (current_crc != previous_buffer_crc) {
//        uint8_t i;
//        for(i = 0; i < 8; i++) {
//            ssd1306_WriteCommand(0xB0 + i);
//            ssd1306_WriteCommand(0x00);
//            ssd1306_WriteCommand(0x10);
//            ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH * i], SSD1306_WIDTH);
//        }
//        // Update the previous CRC
//        previous_buffer_crc = current_crc;
//    }
//}

void ssd1306_UpdateScreen(void) {
    uint32_t current_crc = calculate_crc32_hardware(SSD1306_Buffer, sizeof(SSD1306_Buffer));

    if (current_crc == previous_buffer_crc) {
        return;
    }

    if (is_screen_sleeping) {
        ssd1306_WriteCommand(0xAF);  // Display ON
        is_screen_sleeping = false;
    }

    for (uint8_t i = 0; i < 8; i++) {
        ssd1306_WriteCommand(0xB0 + i);
        ssd1306_WriteCommand(0x00);
        ssd1306_WriteCommand(0x10);
        ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH * i], SSD1306_WIDTH);
    }

    previous_buffer_crc = current_crc;
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
