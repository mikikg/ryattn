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
#include <stdlib.h>
#include "main.h"
#include "ssd1306.h"
#include "TIM2_enc.h"
#include "flash.h"
#include "USART3.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

#define theme_enable_title 1
#define theme_enable_vu 2

//some vars
volatile bool update_seq_up_down = 1;
bool my_blink;
volatile int current_seq_position = 1;
volatile int seq_position_max = 6;
int tmp_volume, cnt;
int fps, fps_last;
char buffer[32];

volatile uint32_t SW_timers[8];
volatile uint32_t SW_timers_enable[8];
volatile bool screen_saver_active;

extern volatile int16_t MyData[20];
extern volatile bool menu_active;
extern volatile bool edit_active;
extern volatile bool mute_active;
extern volatile bool save_change_flag;
extern volatile bool vol_change_flag;
extern volatile int ENC_IMPS_PER_STEP;
extern volatile int ENC_IMPS_PER_STEP_HALF;

//---------------------------------------------------------------------------
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

void Update_Encoder_Settings() {
    TIM2->ARR = ENC_IMPS_PER_STEP = MyData[IMPSSTEP];
    ENC_IMPS_PER_STEP_HALF = ENC_IMPS_PER_STEP / 2; //init
    TIM2->SMCR = MyData[QEIMODE];
}

void Save_Settings() {
    Update_Encoder_Settings();
    //Save to EE ...
    Flash_Write_MyData();
}

void Reset_Settings() {
    MyData[VOLUME] = 64;
    MyData[MENU] = 0;
    MyData[OPMODE] = 1;
    MyData[DELAY] = 10;
    MyData[DEBUG] = 0;
    MyData[QEIMODE] = 2;
    MyData[IMPSSTEP] = ENC_IMPS_PER_STEP;
    MyData[ENABLEIR] = 0;
    MyData[THEME] = 3;
    MyData[SSAVER] = 0;
    Update_Encoder_Settings();
}

const struct {
    char *text;
    bool editable;
} FullMenu[] = {
        {"Settings",            0},//0
        {" Relay Attenuator",   0},//1
        {"*Exit",               0},//2
        {"OPmode",              1},//3
        {"Delay",               1},//4
        {"Debug",               1},//5
        {"QEImode",             1},//6
        {"QEIimps/s",           1},//7
        {"EnableIR",            0},//8
        {"Theme",               1},//9
        {"SSaverMin",           1},//10
        {"*ResetSettings!",     0},//11
        {"*forum.yu3ma.net",    0},//12
        {"*FW 0.9.6 05-2021",   0},//13
        {"*Save & Exit",        0},//14
};

//--------------------------------------------------------
//------------------------ RELAYS ------------------------
//--------------------------------------------------------
void Handle_Relays_mode1() {
    //Mode1 (Goran) prvo palimo releje koji trebaju pa posle one koji se gase
    if (vol_change_flag) {
        if (SW_timers[T_RY] == 1) { //prvi prolaz postavljamo sve koje trebaju
            if (bitmask1 != 0) RY1_on; //PA3 -1dB
            if (bitmask2 != 0) RY2_on; //PA4 -2dB
            if (bitmask3 != 0) RY3_on; //PA5 -4dB
            if (bitmask4 != 0) RY4_on; //PA6 -8dB
            if (bitmask5 != 0) RY5_on; //PA7 -16dB
            if (bitmask6 != 0) RY6_on; //PB0 -32dB
            if (MyData[DELAY]==0) goto odradi_ostalo; //nema delay odradi i ostalo
        } else if (SW_timers[T_RY] >= (MyData[DELAY]+1)) {//kada istekne gasimo koje trebaju
            odradi_ostalo:
            SW_timers[T_RY] = 0;
            vol_change_flag = 0;
            if (bitmask1 == 0) RY1_off; //PA3 -1dB
            if (bitmask2 == 0) RY2_off; //PA4 -2dB
            if (bitmask3 == 0) RY3_off; //PA5 -4dB
            if (bitmask4 == 0) RY4_off; //PA6 -8dB
            if (bitmask5 == 0) RY5_off; //PA7 -16dB
            if (bitmask6 == 0) RY6_off; //PB0 -32dB
        }
    }
}

void Handle_Relays_mode2() {
    //Mode2 (Braca), sve prvo OFF pa oni koji trebaju ON
    if (vol_change_flag) {
        if (SW_timers[T_RY] == 1) { //prvi prolaz sve off
            RY1_off; RY2_off; RY3_off; RY4_off; RY5_off; RY6_off;
            if (MyData[DELAY]==0) goto odradi_ostalo1; //nema delay odradi i ostalo
        } else if (SW_timers[T_RY] >= (MyData[DELAY]+1)) {//delay
            odradi_ostalo1:
            if (bitmask1 != 0) RY1_on; //PA3 -1dB
            if (bitmask2 != 0) RY2_on; //PA4 -2dB
            if (bitmask3 != 0) RY3_on; //PA5 -4dB
            if (bitmask4 != 0) RY4_on; //PA6 -8dB
            if (bitmask5 != 0) RY5_on; //PA7 -16dB
            if (bitmask6 != 0) RY6_on; //PB0 -32dB
        }
    }
}

void Handle_Relays_mode3() {
    //Mode3 (Miki) - sekvencialno jedan za drugim ili sve odmah
    if (MyData[DELAY] == 0) {
        if (bitmask1 != 0) RY1_on; else RY1_off; //PA3 -1dB
        if (bitmask2 != 0) RY2_on; else RY2_off; //PA4 -2dB
        if (bitmask3 != 0) RY3_on; else RY3_off; //PA5 -4dB
        if (bitmask4 != 0) RY4_on; else RY4_off; //PA6 -8dB
        if (bitmask5 != 0) RY5_on; else RY5_off; //PA7 -16dB
        if (bitmask6 != 0) RY6_on; else RY6_off; //PB0 -32dB
    } else if (SW_timers[T_RY] >= MyData[DELAY]) { SW_timers[T_RY] = 0;
        switch (current_seq_position) {
            case 1: if (bitmask1 != 0) RY1_on; else RY1_off; break; //PA3 -1dB
            case 2: if (bitmask2 != 0) RY2_on; else RY2_off; break; //PA4 -2dB
            case 3: if (bitmask3 != 0) RY3_on; else RY3_off; break; //PA5 -4dB
            case 4: if (bitmask4 != 0) RY4_on; else RY4_off; break; //PA6 -8dB
            case 5: if (bitmask5 != 0) RY5_on; else RY5_off; break; //PA7 -16dB
            case 6: if (bitmask6 != 0) RY6_on; else RY6_off; break; //PB0 -32dB
        }
        if (update_seq_up_down) {
            if (current_seq_position++ >= seq_position_max) current_seq_position = 1;
        } else {
            if (current_seq_position-- <= 1) current_seq_position = seq_position_max ;
        }
    }
}

// Konfiguracija (72000) za SysTick da radi kao 1kHz (1ms) timer
// Iskoriscen SysTick HW tajmer za jos X SW tajmera + Handle_relays()
void SysTick_Handler (void) {
    if (SW_timers_enable[T_BUTTON] == 1) SW_timers[T_BUTTON] ++;
    SW_timers[T_EDIT_BLINK] ++;
    SW_timers[T_EDIT_OFF] ++;
    SW_timers[T_UN_3] ++;
    SW_timers[T_UN_4] ++;
    SW_timers[T_SSAVER] ++;
    SW_timers[T_FPS] ++;
    SW_timers[T_RY] ++;

    //switch operation mode
    switch (MyData[OPMODE]) {
        case 1:
            Handle_Relays_mode1(); //Function call every 1ms
            break;
        case 2:
            Handle_Relays_mode2(); //Function call every 1ms
            break;
        case 3:
            Handle_Relays_mode3(); //Function call every 1ms
            break;
    }

    //mute
    if (mute_active || MyData[VOLUME]==64) RY7_on; else RY7_off;
}

//----------------------------------------
// Main
//----------------------------------------
int main(void) {

    MYSYS_init();
    GPIO_init();
    USART3_init();
    TIM2_Setup_ENC();
    SysTick_Config(72000); //1ms

    ssd1306_Init();

    ssd1306_Fill(0);
    ssd1306_UpdateScreen();
    //ssd1306_TestFonts();
    //for (int y = 0; y < 5000000; y++) { GPIOC->BSRR = GPIO_BSRR_BR13; } //some delay
    //ssd1306_Fill(0);

    //load saved data from EE, but reset to default before load in case od EE non-init error
    Reset_Settings();
    Flash_Read_MyData();
    Update_Encoder_Settings();

    //restore back TMR2 settings
    TIM2->ARR = ENC_IMPS_PER_STEP = MyData[IMPSSTEP];
    ENC_IMPS_PER_STEP_HALF = ENC_IMPS_PER_STEP / 2; //init
    TIM2->SMCR = MyData[QEIMODE];

    //---------------------
    //endless loop!
    while (1){

        fps++;
        if (SW_timers[T_FPS] >= 1000) {
            SW_timers[T_FPS] = 0;
            fps_last = fps;
            fps = 0;
        }

        //---------------------------------------------
        //Crtanje po ekranu ---------------------------
        //---------------------------------------------
        if (MyData[SSAVER] != 0) {
            if (SW_timers[T_SSAVER] > MyData[SSAVER]*1000*60 && !screen_saver_active) {
                SW_timers[T_SSAVER] = 0;
                ssd1306_Fill(Black);
                screen_saver_active = true;
            }
        } else if (MyData[SSAVER] == 0) {
            screen_saver_active = 0;
        }

        if (screen_saver_active) {
            //screen saver
            /// .. . . . . ..
            ssd1306_DrawPixel((rand() & 127) , (rand () & 63), (cnt++ % 500 == 0) ? 1 : 0);
        } else {

            //Main screen
            ssd1306_Fill(Black);
            ssd1306_SetCursor(0, 0);

            if (menu_active) {
                //Settings menu
                sprintf(buffer, "%s %d", FullMenu[0].text, MyData[MENU]); //
                ssd1306_WriteString(buffer, Font_11x18, White); //11px font

                //draw menu items
                if (MyData[MENU] > 3) { //4 - X vertical menu scroll
                    int offset = MyData[MENU] - 3;
                    for (int i = 0; i < 4; ++i) {
                        if (FullMenu[i + 2 + offset].editable == 0) {//
                            sprintf(buffer, "%s", FullMenu[i + 2 + offset].text); //
                        } else {
                            sprintf(buffer, "%s=%d", FullMenu[i + 2 + offset].text, MyData[i + 1 + offset]); //
                        }
                        ssd1306_SetCursor(7, 18 + i * 10);
                        ssd1306_WriteString(buffer, Font_7x10, White);
                    }
                    //set cursor for draw fixed selection 3 ">"
                    ssd1306_SetCursor(0, 18 + 30);
                } else { //0 - 3 items
                    for (int i = 0; i < 4; ++i) {
                        if (FullMenu[i+2].editable == 0) {//
                            sprintf(buffer, "%s", FullMenu[i + 2].text); //
                        } else {
                            sprintf(buffer, "%s=%d", FullMenu[i + 2].text, MyData[i + 1]); //
                        }
                        ssd1306_SetCursor(7, 18 + i * 10);
                        ssd1306_WriteString(buffer, Font_7x10, White);
                    }
                    //set cursor for draw selection 0-3 ">"
                    ssd1306_SetCursor(0, 18 + MyData[MENU] * 10);
                }

                if (SW_timers[T_EDIT_BLINK] > 200) {//200ms edit blink
                    my_blink = !my_blink;
                    SW_timers[T_EDIT_BLINK] = 0;
                }
                if (edit_active) {
                    ssd1306_WriteString(my_blink ? ">" : "", Font_7x10, White); //with blink

                    //auto turn off edit mode after 2 seconds (spare one click!)
                    if (SW_timers[T_EDIT_OFF] > 3000 && my_blink) {
                        edit_active = 0;
                        SW_timers[T_EDIT_OFF] = 0;
                    }
                } else {
                    ssd1306_WriteString(">", Font_7x10, White);
                }

            } else {
                //Main screen with Volume
                if ((MyData[THEME] & theme_enable_title ) != 0) ssd1306_WriteString(FullMenu[1].text, Font_7x10, White);

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

                //debug ----------------
                if (MyData[DEBUG] == 1) {
                    //FPS
                    //sprintf(buffer, "VOL=%01B" PRINTF_BINARY_PATTERN_INT8,                            PRINTF_BYTE_TO_BINARY_INT8(MyData[VOLUME])); //
                    sprintf(buffer, "FPS=%d", fps_last); //
                    ssd1306_SetCursor(7, 10);
                    ssd1306_WriteString(buffer, Font_7x10, White); //7px font

                    sprintf(buffer, "R17[%d %d %d %d %d %d %d]",
                            (_Bool) (GPIOA->IDR & GPIO_IDR_IDR3),
                            (_Bool) (GPIOA->IDR & GPIO_IDR_IDR4),
                            (_Bool) (GPIOA->IDR & GPIO_IDR_IDR5),
                            (_Bool) (GPIOA->IDR & GPIO_IDR_IDR6),
                            (_Bool) (GPIOA->IDR & GPIO_IDR_IDR7),
                            (_Bool) (GPIOB->IDR & GPIO_IDR_IDR0),
                            (_Bool) (GPIOB->IDR & GPIO_IDR_IDR1)
                    ); //
                    ssd1306_SetCursor(0, 44);
                    ssd1306_WriteString(buffer, Font_7x10, White); //7px font
                }

                //Level scale at bottom
                if ((MyData[THEME] & theme_enable_vu ) != 0) Draw_VU(0, 58, 64 - (mute_active ? tmp_volume : MyData[VOLUME]));

            }
        }


        //---------------------------------------------
        // Button -------------------------------------
        //---------------------------------------------
        //Button kratak klick 10-500ms
        if (SW_timers[T_BUTTON] > 50 && SW_timers[T_BUTTON] < 500 && SW_timers_enable[0] == 0) {
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
                    case 8:
                        edit_active = !edit_active; //edit mode
                        break;

                    case 9: //reset settings
                        save_change_flag = 1;
                        Reset_Settings();
                        break;

                    case 12: //save settings
                        menu_active = 0; //izlazimo iz menu
                        MyData[MENU] = 0;
                        if (save_change_flag) Save_Settings();
                        break;

                }
            } else {
                //kratak klik na glavnom ekranu = mute/unmute
                mute_active = !mute_active;
                if (mute_active) {
                    tmp_volume = MyData[VOLUME];
                    MyData[VOLUME] = 64;
                } else {
                    MyData[VOLUME] = tmp_volume;
                }
            }
            SW_timers[T_BUTTON] = 0;
        }

        //Button dugacak klick > 1000ms aktivira menu | na glavnom ekranu
        if (SW_timers[T_BUTTON] >= 1000 && !menu_active) {
            menu_active = 1;
            SW_timers[T_BUTTON] = 0;
            SW_timers_enable[0] = 0;
        }
        //Button dugacak klick > 1000ms aktivira edit | u edit ekatu
        if (SW_timers[T_BUTTON] >= 1000 && menu_active) {
            edit_active = 1;
            SW_timers[T_BUTTON] = 0;
            SW_timers_enable[0] = 0;
        }

        //pred kraj osvezi ceo ekran
        ssd1306_UpdateScreen();

    } // end while (1)
} //end main

#pragma clang diagnostic pop
