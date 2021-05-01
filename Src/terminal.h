//
// Created by miki on 11/4/21.
//
#include "USART3.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char myInfo[] =
        "\n"
        "/--------------------------------------\\\n"
        "| Programmable Relay Attenuator V1.0   |\n"
        "| By YU3MA 2021                        |\n"
        "| https://forum.yu3ma.net              |\n"
        "| Type 'h' for help                    |\n"
        "\\--------------------------------------/\n";

const char myHelp[] =
        "\n"
        "/--------------------------------------\\\n"
        "| Programmable Relay Attenuator V1.0   |\n"
        "\\--------------------------------------/\n"
        "Command overview \n"
        "h H        - Help\n"
        "D          - Dump vars\n"
        "d          - Dump vars and names\n"
        "i I        - Machine Info\n"
        "c          - Main loop counts per 100ms\n"
        "x          - Dump NVRAM in RAW mode\n"
        "X          - Dump NVRAM in HEX mode\n"
        "r          - Read USART3 diag data\n"
        "R          - Clear USART3 diag data\n"
        "q          - Set QUICK BOOT flag\n"
        "+          - Increment menu/value\n"
        "-          - Decrement menu/value\n"
        "e          - Toggle view/edit mode\n"
        "s          - Toggle start/stop mode\n"
        "b          - Turn off boot info\n"
        "B          - Turn on boot info\n"
        "~          - Reset controller\n"
        "r[xxxx]    - Read MCU register at ADR !?\n"
        "a          - Read current A/D value\n"
        "A          - Read integrated A/D value\n"
        "\n";

const char * setting_names[] =
        {
                "MS_prgno  ",
                "MS_size   ",
                "MS_speed  ",
                "MS_ramp   ",
                "MS_ustep  ",
                "MS_pre_d  ",
                "MS_hold   ",
                "MS_pos_d  ",
                "MS_null   ",
                "MS_total  ",
                "MS_count  ",
                "MS_bidir  ",
                "MS_t_pul  ",
                "MS_adval  ",
                "MS_adcal  ",
        };


//volatile uint32_t GLOABAL_DIAG[10];

char itoa_buffer[16];
#define maxMenu 15
extern uint32_t myMachineSetting[maxMenu];

void dumpMachineSettings(char *UART_KBD_TX_buff, int items, int include_names) {
    strcat(UART_KBD_TX_buff, "--- MachineSetting 0-15 ---\n");
    for (int idx = 0; idx < items; idx++) {
        if (include_names) {
            strcat(UART_KBD_TX_buff, setting_names[idx]);
        }
        itoa(myMachineSetting[idx], itoa_buffer, 10);
        strcat(UART_KBD_TX_buff, itoa_buffer);
        strcat(UART_KBD_TX_buff, "\n"); //1040
    }
    strcat(UART_KBD_TX_buff, "---------------------------\n");
}

void serialDecoderTask (void );
