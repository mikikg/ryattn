//
// Created by miki on 23/04/2021.
//

#define MAX_KBD_STRLEN 1024 // this is the maximum string length of our string in characters

//funkcije
void USART3_init (void);
void KBD_dma_transmit_buffer (int len);

#ifndef ENUM_DEBUG
#define ENUM_DEBUG
enum {RECEIVED_PACKET, RECEIVED_BYTE, TRANSMITTED_PACKET, FRAMING_ERROR, NOISE_ERROR, CRC_ERROR, BROADCAST_PACKET, UPTIME_MINS, PARITY_ERROR, TRANSMITTED_BYTE};
#endif


