/*

 Relay-attenuator
 Created by miki on 23/04/2021.

 SPI2 (OLED display):
    PB12 CS - OUT
    PB13 CLK - OUT
    PB14 D/C - OUT NORMAL GPIO NOT MISO!
    PB15 MOSI - OUT
    PA8 RES - OUT

 UART3:
    PB10 - TX
    PB11 - RX

 TIM2 (Encoder) Input:
    PA0
    PA1

 Button Input:
    PA2

 Relay OUTs:
    PA3 - Relay 1
    PA4 - Relay 2
    PA5 - Relay 3
    PA6 - Relay 4
    PA7 - Relay 5
    PB0 - Relay 6

 Mute OUT:
    PB1

 */

#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "ssd1306.h"
#include "TIM2_enc.h"
#include "TIM4_tbase.h"
#include "flash.h"
#include "USART3.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

extern volatile uint32_t SW_timers[8];
extern volatile uint32_t SW_timers_enable[8];
extern volatile int16_t MyData[20];
extern volatile bool menu_active;
extern volatile bool edit_active;
extern volatile bool mute_active;
extern volatile bool change_flag;
extern volatile int ENC_IMPS_PER_STEP;
extern volatile int ENC_IMPS_PER_STEP_HALF;

char menu__ [] = ">";
static char* const menu_items[] = {
        "Settings",
        " Relay Attenuator",
        "*Exit",
        "Overlap",
        "Delay",
        "Slope",
        "QEImode",
        "QEIimp/s",
        "EnableIR",
        "SSaverSec",
        "*ResetSettings    ",
        "*forum.yu3ma.net  ",
        "*FW 0.6.0 05-2021 ",
        "*Save & exit      ",
};

void Draw_HLine (int start_x, int start_y, int width) {
    for (int x=start_x; x<width; x++) {
        ssd1306_DrawPixel(x, start_y, 1);
    }
}

void Draw_VU(int start_x, int start_y, int value) {
    Draw_HLine(0, start_y - 2, 128);
    for (int x=0; x<value; x++) {
        ssd1306_DrawPixel(x*2, start_y, 1);
        ssd1306_DrawPixel(x*2, start_y+1, 1);
        ssd1306_DrawPixel(x*2, start_y+2, 1);
        ssd1306_DrawPixel(x*2, start_y+3, 1);
    }
    Draw_HLine(0, start_y + 5, 128);
}

void Save_Settings() {
    TIM2->ARR = ENC_IMPS_PER_STEP = MyData[IMPSSTEP];
    ENC_IMPS_PER_STEP_HALF = ENC_IMPS_PER_STEP / 2; //init
    TIM2->SMCR = MyData[QEIMODE];
    //Save to EE ...
    Flash_Write_MyData();
}

void Reset_Settings() {
    MyData[VOLUME] = 64;
    MyData[MENU] = 0;
    MyData[OVERLAP] = 100;
    MyData[DELAY] = 128;
    MyData[SLOPE] = 255;
    MyData[QEIMODE] = 2;
    MyData[IMPSSTEP] = 200;
    MyData[ENABLEIR] = 0;
    MyData[SSAVER] = 0;
}

//----------------------------------------
// Main
//----------------------------------------
int main(void) {

    MYSYS_init();
    GPIO_init();
    USART3_init();
    TIM2_Setup_ENC();
    TIM4_Setup_TBASE();

    ssd1306_Init();
    ssd1306_Fill(0);
    ssd1306_UpdateScreen();
    //ssd1306_TestFonts();
    //for (int y = 0; y < 5000000; y++) { GPIOC->BSRR = GPIO_BSRR_BR13; } //some delay
    //ssd1306_Fill(0);

    //some vars
    char buffer[32];
    bool my_blink;
    int tmp_volume, cnt;

    //load saved data from EE
    Flash_Read_MyData();

    //restore back TMR2 settings
    TIM2->ARR = ENC_IMPS_PER_STEP = MyData[IMPSSTEP];
    ENC_IMPS_PER_STEP_HALF = ENC_IMPS_PER_STEP / 2; //init
    TIM2->SMCR = MyData[QEIMODE];

    //---------------------
    //endless loop!
    while (1){

        //---------------------------------------------
        //Crtanje po ekranu ---------------------------
        //---------------------------------------------
        ssd1306_Fill(Black);
        ssd1306_SetCursor(0, 0);

        if (menu_active) {
            //Settings menu
            sprintf(buffer, "%s %d", menu_items[0], MyData[MENU]); //
            ssd1306_WriteString(buffer, Font_11x18, White); //11px font

            //draw menu items
            if (MyData[MENU] > 3) { //vertical menu scroll
                int offset = MyData[MENU] - 3;
                for (int i = 0; i < 4; ++i) {
                    sprintf(buffer, "%s=%d", menu_items[i + 2 + offset], MyData[i + 1 + offset]); //
                    ssd1306_SetCursor(7, 18 + i * 10);
                    ssd1306_WriteString(buffer, Font_7x10, White);
                }
                //set cursor for draw fixed selection 3 ">"
                ssd1306_SetCursor(0, 18 + 30);
            } else {
                for (int i = 0; i < 4; ++i) {
                    if (i == 0) {//first item is different
                        sprintf(buffer, "%s", menu_items[i + 2]); //
                    } else {
                        sprintf(buffer, "%s=%d", menu_items[i + 2], MyData[i + 1]); //
                    }
                    ssd1306_SetCursor(7, 18 + i * 10);
                    ssd1306_WriteString(buffer, Font_7x10, White);
                }
                //set cursor for draw selection 0-3 ">"
                ssd1306_SetCursor(0, 18 + MyData[MENU] * 10);
            }

            if (SW_timers[1] > 200) {//200ms edit blink
                my_blink = !my_blink;
                SW_timers[1] = 0;
            }
            if (edit_active) {
                ssd1306_WriteString(my_blink ? menu__:"", Font_7x10, White); //with blink

                //auto turn off edit mode after 2 seconds (spare one click!)
                if (SW_timers[2] > 3000 && my_blink) {
                    edit_active = 0;
                    SW_timers[2]=0;
                }
            } else {
                ssd1306_WriteString(menu__, Font_7x10, White);
            }

        } else {
            //Main screen with Volume
            ssd1306_WriteString(menu_items[1], Font_7x10, White);

            if (MyData[VOLUME] == 64) {
                sprintf(buffer, "MUTE"); //
                ssd1306_SetCursor(30, 20);
            } else {
                sprintf(buffer, " -%ddB", MyData[VOLUME]); //
                ssd1306_SetCursor(5, 20);
            }

            //ssd1306_WriteString(buffer, Font_7x10, White); //7px font
            //ssd1306_WriteString(buffer, Font_11x18, White); //11px font
            ssd1306_WriteString(buffer, Font_16x26, White); //16px font

            //Level scale at bottom
            if (mute_active) {
                Draw_VU(0, 58, 64 - tmp_volume);
            } else {
                Draw_VU(0, 58, 64 - MyData[VOLUME]);
            }
        }

        //---------------------------------------------
        // Button -------------------------------------
        //---------------------------------------------
        //Button kratak klick 10-500ms
        if (SW_timers[0] > 50 && SW_timers[0] < 500 && SW_timers_enable[0] == 0) {
            if (menu_active) {
                switch (MyData[MENU]) {
                    case 0:
                        menu_active = 0; //izlazimo iz menu
                        break;

                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                        edit_active = !edit_active; //edit mode
                        break;

                    case 8: //reset settings
                        change_flag = 1;
                        Reset_Settings();
                        break;

                    case 11: //save settings
                        menu_active = 0; //izlazimo iz menu
                        MyData[MENU] = 0;
                        if (change_flag) Save_Settings();
                        break;

                }
            } else {
                //glavni ekran  = mute
                mute_active = !mute_active;
                if (mute_active) {
                    tmp_volume = MyData[VOLUME];
                    MyData[VOLUME] = 64;
                } else {
                    MyData[VOLUME] = tmp_volume;
                }
            }
            SW_timers[0] = 0;
        }

        //Button dugacak klick > 1000ms aktivira menu
        if (SW_timers[0] > 1000 && !menu_active) {
            menu_active = 1;
            SW_timers[0] = 0;
            SW_timers_enable[0] = 0;
        }

        //osvezi ceo ekran
        ssd1306_UpdateScreen();


    } // end while (0)
} //end main

#pragma clang diagnostic pop
