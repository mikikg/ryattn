/*

 Relay-attenuator
 Created by miki on 23/04/2021.

 SPI2 (OLED display):
	PB12 - NSS2 * (OUT)
	PB13 - SCK2 (OUT)
	PB14 - MISO2 (IN)
	PB15 - MOSI2 (OUT)

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

#define Button GPIOA->IDR & GPIO_IDR_IDR2
extern volatile uint32_t SW_timers[4];
extern volatile int32_t MyData[10];

extern volatile bool menu_active;
extern volatile bool edit_active;

char menu__ [] = ">";
char menu__e [] = "=";
static const char* const menu_items[] = {
        "Settings",
        " Relay Attenuator",
        "Back",
        "Overlap",
        "Delay",
        "Slope",
        "QEImode",
        "QEIimp/s",
        "EnableIR",
        "ScreenSaver",
        "FW Version 0.3.0   ",
        "forum.yu3ma.net    "
};

void drawHLine (int start_x, int start_y, int width) {
    for (int x=start_x; x<width; x++) {
        ssd1306_DrawPixel(x, start_y, 1);
    }
}

void drawVU(int start_x, int start_y, int value) {
    drawHLine(0, start_y - 2, 128);
    for (int x=0; x<value; x++) {
        ssd1306_DrawPixel(x*2, start_y, 1);
        ssd1306_DrawPixel(x*2, start_y+1, 1);
        ssd1306_DrawPixel(x*2, start_y+2, 1);
        ssd1306_DrawPixel(x*2, start_y+3, 1);
    }
    drawHLine(0, start_y + 5, 128);
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
    int cnt;
    bool my_blink;

    MyData[VOLUME] = -64;
    MyData[MENU] = 0;
    MyData[OVERLAP] = 100;
    MyData[DELAY] = 128;
    MyData[SLOPE] = 255;
    MyData[QEIMODE] = 2;
    MyData[IMPSSTEP] = 200;

    //---------------------
    //endless loop!
    while (1){

        cnt++;

        ssd1306_Fill(Black);
        ssd1306_SetCursor(0, 0);

        if (Button) {
            if (SW_timers[0] > 20 && SW_timers[0] < 100) { //mali filter
                if (menu_active) {//
                    switch (MyData[MENU]) {
                        case 0:
                            menu_active = !menu_active; //izlazimo iz menu
                            break;

                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                            edit_active = !edit_active; //edit mode
                            break;
                    }
                } else {
                    menu_active = !menu_active; //ulazimo u menu
                }
                SW_timers[0]=1000; //hack :)
            }
        } else {
            SW_timers[0] = 0;
        }

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
                //auto turn off edit mode after 3 seconds (spare one click!)
                if (SW_timers[2] > 3000) edit_active = 0;
            } else {
                ssd1306_WriteString(menu__, Font_7x10, White);
            }

        } else {
            //Main screen with Volume
            ssd1306_WriteString(menu_items[1], Font_7x10, White);

            if (MyData[VOLUME] == -64) {
                sprintf(buffer, "MUTE"); //
                ssd1306_SetCursor(30, 20);
            } else {
                sprintf(buffer, " %ddB", MyData[VOLUME]); //
                ssd1306_SetCursor(5, 20);
            }

            //ssd1306_WriteString(buffer, Font_7x10, White); //7px font
            //ssd1306_WriteString(buffer, Font_11x18, White); //11px font
            ssd1306_WriteString(buffer, Font_16x26, White); //16px font

            //Level scale at bottom
            drawVU(0, 58, 64 - MyData[VOLUME]*-1);
        }

        ssd1306_UpdateScreen();
    }
}

