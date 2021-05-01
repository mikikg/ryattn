//
// Created by miki on 23/04/2021.
//

#include "stm32f1xx.h"
#include "terminal.h"
#include "SPI2.h"
/*
void serialDecoderTask (void ) {

    //-----------------------------------------------------------
    //---------------------- serial decoder ---------------------
    //-----------------------------------------------------------
    if (UART_KBD_rx_ready) {

        if (UART_KBD_RX_packet_len > 1) {//multiple bytes commands

            //clear out buffer
            UART_KBD_TX_buff[0]=0;

            switch (UART_KBD_RX_buff[0]) {
                uint8_t in_adr;
                case 'r': //read MCU register at absolute 32bit addr :)

                    //
                    break;

                case 'w'://write into NV value at word addr
                    break;

                case 0x0D: //\n toggle edit
                case 0x0A: //\r
                    GPIOA->ODR ^= GPIO_ODR_ODR11;
                    goto without_transmit;

                    //default:
                    //    sprintf(UART_KBD_TX_buff, "ERR %02X %02X %02X %02X %02X\n", UART_KBD_RX_buff[0], UART_KBD_RX_buff[1], UART_KBD_RX_buff[2], UART_KBD_RX_buff[3], UART_KBD_RX_buff[4]);
            }

        } else {
            //single byte commands

            SysTick->VAL = 0;
            int time_start = SysTick->VAL;
            //clear out buffer
            UART_KBD_TX_buff[0] = 0;

            switch (UART_KBD_RX_buff[0]) {

                case 'h':
                case 'H': //help
                    strcpy(UART_KBD_TX_buff, myHelp);
                    break;

                case 'q':
                    FR_SPI2_set32bitWord(NV_QUICK_BOOT, 1);
                    break;
                case 'b':
                    FR_SPI2_set32bitWord(NV_PERMANENT_QUICK_BOOT, 0);
                    break;
                case 'B':
                    FR_SPI2_set32bitWord(NV_PERMANENT_QUICK_BOOT, 1);
                    break;

                case 'e': //toggle view/edit (force value on input pin via ODR)
                case 0x0D: //\n
                case 0x0A: //\r
                    GPIOA->ODR ^= GPIO_ODR_ODR11;
                    goto without_transmit;

                case 's': //toggle start/stop (force value on input pin via ODR)
                    GPIOA->ODR ^= GPIO_ODR_ODR10;
                    goto without_transmit;

                case '~': //reset controller
                    NVIC_SystemReset();
                    break;

                case '+': //increment menu/val
                case 0x1D: // arrow up
                case 0x1E: //arrow right
//                    if (myKBtime[KBS_edit] > 200) {
//                        myMachineSetting[TSC]++;
//                    } else {
//                        TSC++;
//                        if (TSC >= maxMenu) TSC = 0;
//                    }
                    goto without_transmit;

                case '-': //decrement menu/val
                case 0x1C: // arrow down
                case 0x1F: //arrow left
//                    if (myKBtime[KBS_edit] > 200) {
//                        myMachineSetting[TSC]--;
//                    } else {
//                        if (TSC == 0) TSC = maxMenu;
//                        TSC--;
//                    }
                    goto without_transmit;

                case 'i':
                case 'I': //info
                    sprintf(UART_KBD_TX_buff, "---- Info ---\nBoots=%lu\nMinutes=%lu\nTotal=%lu\n-------------\n",
                            FR_SPI2_get32bitWord(NV_BOOT_CNT),
                            FR_SPI2_get32bitWord(NV_MINUTE_CNT),
                            FR_SPI2_get32bitWord(NV_TOTAL_CNT)
                    );
                    break;

                case 'a': //Read A/D value
                case 'A': //Read A/D value
//                    sprintf(UART_KBD_TX_buff, "ADC1->DR = %05d   final_ad_val=%05d\n",
//                            ADC1->DR, final_ad_val
//                    );
                    break;

                case 'D': //dump vars
                    UART_KBD_TX_buff[0] = 0; //reset string buffer
                    //dumpMachineSettings(UART_KBD_TX_buff, maxMenu, 0);
                    break;
                case 'd': //dump vars and names
                    UART_KBD_TX_buff[0] = 0; //reset string buffer
                    //dumpMachineSettings(UART_KBD_TX_buff, maxMenu, 1);
                    break;

                case 'R'://reset diag
                    GLOBAL_DIAG[RECEIVED_PACKET] = 0;
                    GLOBAL_DIAG[TRANSMITTED_PACKET] = 0;
                    GLOBAL_DIAG[FRAMING_ERROR] = 0;
                    GLOBAL_DIAG[NOISE_ERROR] = 0;
                    GLOBAL_DIAG[RECEIVED_BYTE] = 0;
                    GLOBAL_DIAG[TRANSMITTED_BYTE] = 0;
                    break;

                case 'r'://read diag
                    sprintf(UART_KBD_TX_buff, "RXb=%lu RXp=%lu TXb=%lu TXp=%lu FRERR=%lu NSERR=%lu\n",
                            GLOBAL_DIAG[RECEIVED_BYTE],
                            GLOBAL_DIAG[RECEIVED_PACKET],
                            GLOBAL_DIAG[TRANSMITTED_BYTE],
                            GLOBAL_DIAG[TRANSMITTED_PACKET],
                            GLOBAL_DIAG[FRAMING_ERROR],
                            GLOBAL_DIAG[NOISE_ERROR]
                    );
                    break;

                case 'x':
                    for (int adr = 0; adr < 2048; adr++) { //2048 full NVM
                        uint32_t bla;

                        bla = FR_SPI2_get32bitWord(adr);

                        for (int dd=0; dd<3000; dd++){
                            GPIOC->BSRR=GPIO_BSRR_BR13;
                        }

                        UART_KBD_TX_buff[0] =  bla >> 24;
                        UART_KBD_TX_buff[1] =  bla >> 16;
                        UART_KBD_TX_buff[2] =  bla >> 8;
                        UART_KBD_TX_buff[3] =  bla ;
                        KBD_dma_transmit_buffer(4);

                        for (int dd=0; dd<100; dd++){
                            GPIOC->BSRR=GPIO_BSRR_BR13;
                        }
                    }
                    goto without_transmit;

                case 'X':
                    for (int adr = 0; adr < 2048; adr++) { //2048 full NVM
                        KBD_dma_transmit_buffer(sprintf(UART_KBD_TX_buff, "%08lX", FR_SPI2_get32bitWord(adr)));
                        for (int dd=0; dd<7000; dd++){
                            GPIOC->BSRR=GPIO_BSRR_BR13;
                        }
                    }
                    goto without_transmit;

                default:
                    //UART_KBD_TX_buff[0] = UART_KBD_RX_buff[0];
                    //KBD_dma_transmit_buffer(1);
                    goto without_transmit;

            }
        }

        goto without_time; //skip time

//        time_end = SysTick->VAL;
//        itoa((time_start - time_end) ,itoa_buffer,10);
//        strcat(UART_KBD_TX_buff, ">Command took ");
//        strcat(UART_KBD_TX_buff, itoa_buffer);
//        strcat(UART_KBD_TX_buff, " clocks or ");
//        itoa((time_start - time_end) / 72, itoa_buffer, 10);
//        strcat(UART_KBD_TX_buff, itoa_buffer);
//        strcat(UART_KBD_TX_buff, "us\n");

        without_time:
        KBD_dma_transmit_buffer(sizeof (UART_KBD_TX_buff));

        without_transmit:
        UART_KBD_rx_ready = 0;

    }
}
*/