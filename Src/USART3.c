/*

USART3 kontrolne funkcije za KBD+LCD

Koriste se pinovi:
PB10 = USART TX - output
PB11 = USART RX - input

*/

#include "stm32f1xx.h"
#include "USART3.h"

//----------- varijable od USAR3 -------------------------

volatile uint8_t UART_KBD_RX_buff [MAX_KBD_STRLEN+1]; // this will hold the received string
volatile char UART_KBD_TX_buff [MAX_KBD_STRLEN+1]; // this will hold the string to transmit
volatile uint8_t UART_KBD_rx_ready;
volatile uint16_t UART_KBD_cntx;
volatile uint16_t UART_KBD_RX_packet_len;
volatile uint32_t GLOBAL_DIAG[10];



//extern volatile uint8_t UART_485_pass_trough_flag;
//extern volatile uint8_t UART_485_TX_buff [MAX_485_TX_STRLEN+1]; // this will hold the string to transmit

//volatile uint32_t GLOABAL_DIAG[10];

//#define RECEIVED_BYTE 1

void USART3_init (void) {

    //enable clock + reset
    RCC->APB1ENR  |=  RCC_APB1ENR_USART3EN; // Turn on USART3
    RCC->APB1RSTR |=  RCC_APB1RSTR_USART3RST; //RESET: USART3
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USART3RST; //UNRESET: USART3

    //DMA1 clock enable
    RCC->AHBENR |=RCC_AHBENR_DMA1EN;

    // Put PB10  (TX) to alternate function output push-pull at 50 MHz
    // Put PB11 (RX) to floating input
    //--------------- PB10  (TX) -------------
    //Alt function Output Push-Pull
    GPIOB->CRH &= ~GPIO_CRH_CNF10_0; //ocisti bit CNF10_0
    GPIOB->CRH |=	 GPIO_CRH_CNF10_1; //setuj bit CNF10_1
    //Output MODE 50MHz
    GPIOB->CRH |=  GPIO_CRH_MODE10_0; //setuj bit MODE10_0
    GPIOB->CRH |=  GPIO_CRH_MODE10_1; //setuj bit MODE10_1

    //--------------- PB11 (RX) %%%%%%%-------------
    //Input mode
    GPIOB->CRH &= ~GPIO_CRH_CNF11_0; //ocisti bit CNF11_0
    GPIOB->CRH |=  GPIO_CRH_CNF11_1; //setuj bit CNF11_1
    //Input with pull-up / pull-down
    GPIOB->CRH &= ~GPIO_CRH_MODE11_0; //ocisti bit
    GPIOB->CRH &= ~GPIO_CRH_MODE11_1; //ocisti bit MODE11_1
    GPIOB->ODR |= GPIO_ODR_ODR11; //setuj bit = Pull-upMODE11_0
    //Pull up (preko ODR se podesava!)

    // Configure BRR by deviding the bus clock with the baud rate

    //default 8 data bits, no flow control and one stop-bit.

    //bus clock je sporiji nego kod USART1 !
    USART3->BRR = 72000000/115200;

    //enable usart, tx, rx, rxne interupt
    USART3->CR1 =
            USART_CR1_UE | //Update Enable
            USART_CR1_TE | //Transmit Enable
            USART_CR1_RE | //Receive Enable
            USART_CR1_RXNEIE | //Enable RX not empty interupt
            USART_CR1_IDLEIE | //Enable IDLE interupt
            USART_CR1_TCIE // Enable Tranmit Complete interupt
            ;

    //enable global USART3 interupt
    NVIC_EnableIRQ(USART3_IRQn);

    //enable DMA for USART3 TX
    USART3->CR3 |= USART_CR3_DMAT;

    DMA1_Channel2->CNDTR = 4; //4 bytes to transfer, menja se po potrebi
    DMA1_Channel2->CMAR = (uint32_t)UART_KBD_TX_buff; //adresa memorije
    DMA1_Channel2->CPAR = (uint32_t)&(USART3->DR); //adresa periferije
    DMA1_Channel2->CCR = RESET; //clear all config
    DMA1_Channel2->CCR =
            //DMA_CCR_CIRC  | //circular mode
            //DMA_CCR_MEM2MEM | //mem2mem
            DMA_CCR_MINC  | //memory increment mode
            DMA_CCR_DIR  | //read from memory
            //DMA_CCR_EN | //activate channel
            DMA_CCR_TCIE //Transfer Complete interupt enable
            ;

    //Enable global DMA stream interrupts
    NVIC_EnableIRQ(DMA1_Channel2_IRQn); //enable channel
    NVIC_SetPriority(DMA1_Channel2_IRQn, 6); //interupt priority
}

//---------- Usart main IRQ -----------------
void USART3_IRQHandler(void){

    //GPIOC->ODR ^= GPIO_ODR_ODR13;
    uint16_t IIR = USART3->SR; //remember state

    //------------- IDLE interupt ----------
    if (IIR & USART_SR_IDLE) {
        UART_KBD_RX_packet_len = UART_KBD_cntx; //remember lenght
        UART_KBD_cntx = 0; //reset counter
        USART3->DR; //fake read to clear flag
		GLOBAL_DIAG[RECEIVED_PACKET] ++; //za diagnostiku

        //if (UART_485_pass_trough_flag){
            //kraj pass-trough, napunjen buffer
            //UART_485_pass_trough_flag = 0;
            //zvizni to nazad na 485
            //RS485_dma_transmit_buffer(UART_KBD_RX_packet_len);
        //} else {
            UART_KBD_rx_ready=1; //kraj prenosa sa KBD
        //}
    }

    //------------- Transfer Complete interupt ----------
    if (IIR & USART_SR_TC) {  //tc interupt
        USART3->SR &= ~USART_SR_TC;	//ocisti TC flag
		GLOBAL_DIAG[TRANSMITTED_PACKET] ++; //za diagnostiku
    }

    //------------- READ interupt ----------
    if (IIR & USART_SR_RXNE) {          // read interrupt
        USART3->SR &= ~USART_SR_RXNE;	    // clear interrupt
        if ((IIR & USART_SR_FE) == 2)  { 	//is framing error
            GLOBAL_DIAG[FRAMING_ERROR] ++; //za diagnostiku
        } else {
            if (IIR & USART_SR_NE) { //noise flag
                GLOBAL_DIAG[NOISE_ERROR] ++; //za diagnostiku
            }

            //if (UART_485_pass_trough_flag){
                //primaj direktno u TX bufer od 485 - max 8k
                //UART_485_TX_buff[UART_KBD_cntx++] = USART3->DR;
                //if (UART_KBD_cntx >= MAX_485_TX_STRLEN) UART_KBD_cntx = 0; 	//nema vece
            //} else {
                //ok, smesti karakter u prijemni buffer
                UART_KBD_RX_buff[UART_KBD_cntx++] = USART3->DR;
                if (UART_KBD_cntx >= MAX_KBD_STRLEN) UART_KBD_cntx = 0; 	//nema vece od MAX_KBD_STRLEN
            //}

            GLOBAL_DIAG[RECEIVED_BYTE] ++; //za diagnostiku
        }
    }
}

//------------- DMA transfer complete interupt -------------
void DMA1_Channel2_IRQHandler (void) {
    if (DMA1->ISR & DMA_ISR_TCIF2) {    // IF Transfer Complete interrupt
        DMA1->IFCR  |= DMA_IFCR_CGIF2; //global clear all flags for channel 4
    }
}

//------------- Function to initiate transmission ------------
void KBD_dma_transmit_buffer (int len) {
    DMA1_Channel2->CCR &= ~ DMA_CCR_EN; //must disable first!
    DMA1_Channel2->CNDTR = len; //len bytes to transfer
    DMA1_Channel2->CMAR = (uint32_t)UART_KBD_TX_buff; //adresa memorije, ponovo posto se inkrementuje!
    //DMA1_Channel2->CPAR = (uint32_t)&(USART3->DR); //adresa periferije, ne menja se
    DMA1_Channel2->CCR |= DMA_CCR_EN; //ukljuci, to startuje transfer
    GLOBAL_DIAG[TRANSMITTED_BYTE] += len; //za diagnostiku
}

