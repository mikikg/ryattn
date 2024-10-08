/*
 Relay-attenuator V1.4
 Created by miki on 23/04/2021.
 Updated on 07/09/2024.

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

 Button Input (EXTI2):
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

 POWER OFF/ON OUT:
    PC14

 Input CH Switch OUT:
    PC15

 IR Input (EXTI3)
    PB3
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "ssd1306.h"
#include "TIM2_enc.h"
#include "TIM1_tbase.h"
#include "flash.h"
#include "USART3.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

#define theme_enable_title 1
#define theme_enable_vu 2
#define max_menu 23

//some vars
volatile bool update_seq_up_down = 1;
bool my_blink;
bool power_off_on = false;
volatile int current_seq_position = 1;
volatile int seq_position_max = 6;
int16_t tmp_volume, cnt;
int fps, fps_last;
char buffer[32];

volatile uint32_t SW_timers[8];
volatile uint32_t SW_timers_enable[8];
volatile bool screen_saver_active;
volatile bool screen_off_active;

extern volatile int16_t MyData[max_menu];
extern volatile bool menu_active;
extern volatile bool edit_active;
extern volatile bool mute_active;
extern volatile bool save_change_flag;
extern volatile bool vol_change_flag;
extern volatile int ENC_IMPS_PER_STEP;
extern volatile int ENC_IMPS_PER_STEP_HALF;

extern volatile bool is_screen_sleeping;  // Praćenje da li je ekran u sleep modu


char* chNames[] = {
        "CD   ", //0
        "DAC  ", //1
        "PHONO", //2
        "TAPE ", //3
        "TUNER", //4
        "PC   ", //5
        "LINE ", //6
        "RPi  ", //7
        "TV   ", //8
        "AUX  ", //9
};

struct NEC {
    uint8_t addr;
    uint8_t addr_inv;
    uint8_t cmd;
    uint8_t cmd_inv;
    uint8_t i;
    uint8_t init_seq;
    uint8_t gpio;
    uint16_t count;
    uint8_t repeat;
    uint8_t complet;
};

struct NEC NEC1;
u_int8_t last_IR_cmd;
bool flag_edge_mute;
bool flag_edge_chsw;
bool flag_edge_power;

//---------------------------------------------------------------------------

//handler koji resetuje IR strukturu kada istekne count (~650ms)
void TIM1_UP_IRQHandler(void) {
    if (TIM1->SR & TIM_SR_UIF) {
        TIM1->SR &=~ TIM_SR_UIF;//clear flag
        if (MyData[ENABLEIR] != 0){
            memset(&NEC1, 0, sizeof(NEC1));  // Resetuje celu strukturu na 0
            flag_edge_mute = 0;
            flag_edge_chsw = 0;
            flag_edge_power = 0;
        }
    }
}

//EXTI IRQ handler za IR_PIN na obe ivice
void EXTI3_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR3) { // Jel pending od kanala 3?
        EXTI->PR |= EXTI_PR_PR3; // Flag se čisti tako što se upiše 1

        if (MyData[ENABLEIR] != 0) {
            NEC1.count = TIM1->CNT;
            TIM1->CNT = 0;
            NEC1.gpio = IR_PIN ? 1 : 0;

            // Optimizacija pomoću switch-case umesto više if uslova
            switch (NEC1.gpio) {
                case 1: // Rising edge (GPIO high)
                    switch (NEC1.count) {
                        case 850 ... 999: // Interval 850-999 (Initial sequence)
                            NEC1.init_seq = 1;
                            NEC1.complet = 0;
                            break;
                        case 40 ... 69: // Interval 40-69 (Bit shift increment)
                            NEC1.i++;
                            break;
                        default:
                            // No relevant actions for other count values
                            break;
                    }
                    break;

                case 0: // Falling edge (GPIO low)
                    switch (NEC1.count) {
                        case 420 ... 479: // Interval 420-479 (Repeat sequence reset)
                            if (NEC1.init_seq) {
                                NEC1.i = -1;
                                NEC1.repeat = 0;
                                NEC1.init_seq = 0;
                            }
                            break;
                        case 200 ... 259: // Interval 200-259 (Repeat command)
                            if (NEC1.init_seq) {
                                NEC1.i = -1;
                                NEC1.init_seq = 0;
                                NEC1.repeat++;
                                NEC1.complet = 1;
                            }
                            break;
                        case 40 ... 179: // Interval 40-179 (Bit manipulation)
                            switch (NEC1.i / 8) {
                                case 0: // Address
                                    NEC1.addr = (NEC1.count > 100) ? (NEC1.addr | (1 << (NEC1.i % 8))) : (NEC1.addr & ~(1 << (NEC1.i % 8)));
                                    break;
                                case 1: // Inverted address
                                    NEC1.addr_inv = (NEC1.count > 100) ? (NEC1.addr_inv | (1 << (NEC1.i % 8))) : (NEC1.addr_inv & ~(1 << (NEC1.i % 8)));
                                    break;
                                case 2: // Command
                                    NEC1.cmd = (NEC1.count > 100) ? (NEC1.cmd | (1 << (NEC1.i % 8))) : (NEC1.cmd & ~(1 << (NEC1.i % 8)));
                                    break;
                                case 3: // Inverted command
                                    NEC1.cmd_inv = (NEC1.count > 100) ? (NEC1.cmd_inv | (1 << (NEC1.i % 8))) : (NEC1.cmd_inv & ~(1 << (NEC1.i % 8)));
                                    break;
                                default:
                                    break;
                            }
                            break;
                        default:
                            // No relevant actions for other count values
                            break;
                    }
                    break;
            }

            if (NEC1.i == 32) {
                NEC1.complet = 1;
            }
        }
    }
}

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
    MyData[ENABLEIR] = 1;
    MyData[THEME] = 3;
    MyData[SSAVER] = 0;
    MyData[IRVUP] = 10;
    MyData[IRVDOWN] = 12;
    MyData[IRMUTE] = 92;
    MyData[IRCHSW] = 3;
    MyData[IRPOWER] = 95;
    MyData[INPUTCH] = 1;
    MyData[POWSAVE] = 1;
    MyData[CH1NAME] = 0; //0=cd 1=dac 2=phono 3=tape 4=tuner
    MyData[CH2NAME] = 1;
    MyData[DOFFMIN] = 0;
    Update_Encoder_Settings();
}

const struct {
    char *text;
    bool editable;
} FullMenu[] = {
        {"Settings",            0},//0
        {"  BP Pre-amp ",       0},//1
        {"*Exit",               0},//2
        {"OPmode",              1},//3
        {"Delay",               1},//4
        {"Debug",               1},//5
        {"QEImode",             1},//6
        {"QEIimps/s",           1},//7
        {"EnableIR",            1},//8
        {"Theme",               1},//9
        {"SSaverMin",           1},//10
        {"IR-VolUP",            1},//11
        {"IR-VolDown",          1},//12
        {"IR-Mute",             1},//13
        {"IR-CHSW",             1},//14
        {"IR-Power",            1},//15
        {"Input-CH",            1},//16
        {"STB-PowerSave",       1},//17
        {"CH1-Name",            1},//18
        {"CH2-Name",            1},//19
        {"DispOfMin",            1},//19
        {"*ResetSettings!",     0},//20
        {"*forum.yu3ma.net",    0},//21
        {"*FW v1.4 09-2024",    0},//22
        {"*Power OFF",          0},//23
        {"*Save & Exit",        0},//24
};

//--------------------------------------------------------
//------------------------ RELAYS ------------------------
//--------------------------------------------------------
void Handle_Relays_mode1() {
    //Mode1 (Goran) prvo palimo releje koji trebaju pa posle one koji se gase
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

void Handle_Relays_mode2() {
    //Mode2 (Braca), sve prvo OFF pa oni koji trebaju ON
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

void Handle_Relays_mode4() {
    //Mode4 (Goran) prvo gasimo releje koji trebaju pa posle one koji se pale
    if (SW_timers[T_RY] == 1) { //prvi prolaz gasimo sve koje trebaju
        if (bitmask1 != 0) RY1_off; //PA3 -1dB
        if (bitmask2 != 0) RY2_off; //PA4 -2dB
        if (bitmask3 != 0) RY3_off; //PA5 -4dB
        if (bitmask4 != 0) RY4_off; //PA6 -8dB
        if (bitmask5 != 0) RY5_off; //PA7 -16dB
        if (bitmask6 != 0) RY6_off; //PB0 -32dB
        if (MyData[DELAY]==0) goto odradi_ostalo4; //nema delay odradi i ostalo
    } else if (SW_timers[T_RY] >= (MyData[DELAY]+1)) {//kada istekne palimo koje trebaju
        odradi_ostalo4:
        SW_timers[T_RY] = 0;
        vol_change_flag = 0;
        if (bitmask1 == 0) RY1_on; //PA3 -1dB
        if (bitmask2 == 0) RY2_on; //PA4 -2dB
        if (bitmask3 == 0) RY3_on; //PA5 -4dB
        if (bitmask4 == 0) RY4_on; //PA6 -8dB
        if (bitmask5 == 0) RY5_on; //PA7 -16dB
        if (bitmask6 == 0) RY6_on; //PB0 -32dB
    }
}

// Konfiguracija (72000) za SysTick da radi kao 1kHz (1ms) timer
// Iskoriscen SysTick HW tajmer za jos X SW tajmera + Handle_relays()
void SysTick_Handler (void) {

    //PC13_on; //for timing test

    if (SW_timers_enable[T_BUTTON] == 1) SW_timers[T_BUTTON] ++;
    SW_timers[T_EDIT_BLINK] ++;
    SW_timers[T_EDIT_OFF] ++;
    if (SW_timers_enable[T_WAKEUP] == 1) SW_timers[T_WAKEUP] ++;
    SW_timers[T_DISPOFF] ++;
    SW_timers[T_SSAVER] ++;
    SW_timers[T_FPS] ++;
    SW_timers[T_RY] ++;

    if (power_off_on == 0 && MyData[POWSAVE] == 1) {
        //power save mode, off all relays
        RY1_off;
        RY2_off;
        RY3_off;
        RY4_off;
        RY5_off;
        RY6_off;
        RY7_off;

        RY9_off;
    } else {
        //switch operation mode, function call every 1ms
        switch (MyData[OPMODE]) {
            case 1:
                Handle_Relays_mode1();
                break;
            case 2:
                Handle_Relays_mode2();
                break;
            case 3:
                Handle_Relays_mode3();
                break;
            case 4:
                Handle_Relays_mode4();
                break;
        }

        //mute OUT
        if (mute_active || MyData[VOLUME] == 64) RY7_off; else RY7_on;

        //input ch switch OUT
        if (MyData[INPUTCH] == 1) RY9_off; else RY9_on;
    }

    //power off/on OUT
    if (power_off_on == 0) RY8_off; else RY8_on;

    //PC13_off;
}

//----------------------------------------
// Main
//----------------------------------------
int main(void) {

    MYSYS_init();
    GPIO_init();
    USART3_init();
    TIM2_Setup_ENC();
    TIM1_Setup_TBASE();

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

    //sve je spremno, startuj glavni tajmer
    SysTick_Config(72000); //1ms

    //---------------------
    //endless loop!
    while (1) {
        last_IR_cmd = NEC1.cmd;

        fps++;
        if (SW_timers[T_FPS] >= 1000) {
            SW_timers[T_FPS] = 0;
            fps_last = fps;
            fps = 0;
        }

        // stb led pc13
        if (power_off_on == 0) {
            GPIOC->BSRR = GPIO_BSRR_BR13;
            TIM2->CR1 &= ~TIM_CR1_CEN; //DISABLE tim periferial
        } else {
            GPIOC->BSRR = GPIO_BSRR_BS13;
            TIM2->CR1 |= TIM_CR1_CEN; //ENABLE tim periferial
        }

        //---------------------------------------------
        //IR kada je primio komandu -------------------
        //---------------------------------------------
        if (MyData[ENABLEIR] != 0 && NEC1.complet && !menu_active) {

            NEC1.complet = 0;

            //probudi ekran
            screen_saver_active = false;
            screen_off_active = false;
            SW_timers[T_SSAVER] = 0;
            SW_timers[T_DISPOFF] = 0;
            SW_timers[T_RY] = 0; //take care!

            //decode IR cmd
            if (NEC1.repeat && !mute_active && power_off_on == 1) {

                if (NEC1.cmd == MyData[IRVUP]) {
                    if (MyData[VOLUME] > 0) MyData[VOLUME]--;
                }

                if (NEC1.cmd == MyData[IRVDOWN]) {
                    if (MyData[VOLUME] < 64) MyData[VOLUME]++;
                }

            } else {
                //one-time cmd

                //Mute
                if (NEC1.cmd == MyData[IRMUTE]) {
                    if (flag_edge_mute == 0 && power_off_on == 1) {
                        mute_active = !mute_active;
                        if (mute_active) {
                            tmp_volume = MyData[VOLUME];
                            MyData[VOLUME] = 64;
                        } else {
                            MyData[VOLUME] = tmp_volume;
                        }
                        flag_edge_mute = 1;
                    }
                }

                //Switch CH1/CH2
                if (NEC1.cmd == MyData[IRCHSW]) {
                    if (flag_edge_chsw == 0 && power_off_on == 1) {
                        if (MyData[INPUTCH]==1) MyData[INPUTCH] = 2; else MyData[INPUTCH] = 1;
                        flag_edge_chsw = 1;
                    }
                }

                //Switch Power
                if (NEC1.cmd == MyData[IRPOWER]) {
                    if (flag_edge_power == 0) {
                        power_off_on = !power_off_on;
                        flag_edge_power = 1;
                    }
                }
            }
        }

        //---------------------------------------------
        //Crtanje po ekranu ---------------------------
        //---------------------------------------------
        if (MyData[SSAVER] != 0) {
            if (SW_timers[T_SSAVER] > MyData[SSAVER] * 1000 * 60 && !screen_saver_active) {
                SW_timers[T_SSAVER] = 0;
                ssd1306_Fill(Black);
                screen_saver_active = true;
            }
        } else if (MyData[SSAVER] == 0) {
            screen_saver_active = 0;
        }

        if (MyData[DOFFMIN] != 0) { //Display off
            if (SW_timers[T_DISPOFF] > MyData[DOFFMIN]*  1000 * 60 && !screen_off_active) {
                //Display OFF activated
                SW_timers[T_DISPOFF] = 0;
                if (is_screen_sleeping == false) {
                    ssd1306_Fill(Black);
                    ssd1306_UpdateScreen(); //need to get last crc32
                    screen_off_active = true;
                    is_screen_sleeping = true;  // Obeleži da je ekran sada u sleep režimu
                    ssd1306_WriteCommand(0xAE);  // Display OFF (sleep mode)
                }
            }
        } else if (MyData[DOFFMIN] == 0){
            screen_off_active = 0;
            is_screen_sleeping = false;
        }

        if (screen_saver_active && power_off_on == 1 && screen_off_active == 0) {
            //screen saver .. . . . . ..
            ssd1306_DrawPixel((rand() & 127), (rand() & 63), (cnt++ % 500 == 0) ? 1 : 0);
        } else {

            //Main screen
            ssd1306_Fill(Black);
            ssd1306_SetCursor(0, 0);

            if (power_off_on == 0) {
                goto skeep_update_src;
            }

            if (screen_off_active == 1) {
                goto no_draw;
            }

            if (menu_active) {
                // Settings menu
                sprintf(buffer, "%s %d", FullMenu[0].text, MyData[MENU]); // Naziv menija i trenutna pozicija
                ssd1306_WriteString(buffer, Font_11x18, White); // 11px font

                // Draw menu items
                if (MyData[MENU] > 3) { // Kada se dešava vertikalni skrol (MENU > 3)
                    int offset = MyData[MENU] - 3;
                    for (int i = 0; i < 4; ++i) { // Prikazuje samo 4 stavke
                        int menu_index = i + 2 + offset; // Prilagođavanje indeksa menija
                        if (!FullMenu[menu_index].editable) {
                            strncpy(buffer, FullMenu[menu_index].text, sizeof(buffer) - 1); // Kopiraj tekst stavke
                        } else {
                            if (i + offset == 16 || i + offset == 17) { // Za specifična imena kanala
                                sprintf(buffer, "%s=%s", FullMenu[menu_index].text, chNames[MyData[i + offset + 1]]);
                            } else {
                                sprintf(buffer, "%s=%d", FullMenu[menu_index].text, MyData[i + offset + 1]);
                            }
                        }
                        ssd1306_SetCursor(7, 18 + i * 10); // Pomeraj kursor za svaku sledeću stavku
                        ssd1306_WriteString(buffer, Font_7x10, White);
                    }
                    // Postavi kursor na fiksnu poziciju za trenutnu stavku u okviru skrol menija
                    ssd1306_SetCursor(0, 18 + 30); // Kursor je uvek na poslednjoj stavci pri skrolovanju
                } else { // Kada nema skrolovanja (MENU <= 3)
                    for (int i = 0; i < 4; ++i) {
                        int menu_index = i + 2;
                        if (!FullMenu[menu_index].editable) {
                            strncpy(buffer, FullMenu[menu_index].text, sizeof(buffer) - 1); // Kopiraj tekst stavke
                        } else {
                            sprintf(buffer, "%s=%d", FullMenu[menu_index].text, MyData[i + 1]);
                        }
                        ssd1306_SetCursor(7, 18 + i * 10); // Pomeraj kursor za svaku sledeću stavku
                        ssd1306_WriteString(buffer, Font_7x10, White);
                    }
                    // Postavi kursor za trenutnu stavku (0-3) u okviru menija
                    ssd1306_SetCursor(0, 18 + MyData[MENU] * 10);
                }

                // Blink logika za edit mod
                if (SW_timers[T_EDIT_BLINK] > 200) {
                    my_blink = !my_blink;
                    SW_timers[T_EDIT_BLINK] = 0;
                }

                if (edit_active) {
                    ssd1306_WriteString(my_blink ? ">" : "", Font_7x10, White); // Blink sa trepćućim ">"
                } else {
                    ssd1306_WriteString(">", Font_7x10, White); // Staticki ">"
                }
            }

            else {
                //Main screen with Volume
                if ((MyData[THEME] & theme_enable_title) != 0) {
                    ssd1306_WriteString(FullMenu[1].text, Font_7x10, White);
                    ssd1306_WriteString(MyData[INPUTCH] == 2 ? chNames[MyData[CH2NAME]] : chNames[MyData[CH1NAME]], Font_7x10, White);
                }

                if (MyData[VOLUME] == 64) {
                    sprintf(buffer, "MUTE"); //
                    ssd1306_SetCursor(35, 20);
                } else {
                    sprintf(buffer, " %s%ddB", MyData[VOLUME] == 0 ? "" : "-", MyData[VOLUME]); //
                    ssd1306_SetCursor(MyData[VOLUME] == 0 ? 21 : 5, 20);
                }

                ssd1306_WriteString(buffer, Font_16x26, White); //16px font

                if (last_IR_cmd != 0 && MyData[DEBUG] == 1) {
                    //print received IR cmd
                    sprintf(buffer, "IR=%d", last_IR_cmd); //
                    ssd1306_SetCursor(7, 10);
                    ssd1306_WriteString(buffer, Font_7x10, White); //7px font
                } else if (last_IR_cmd != 0 && MyData[DEBUG] == 0) {
                    sprintf(buffer, "."); //
                    ssd1306_SetCursor(7, 10);
                    ssd1306_WriteString(buffer, Font_7x10, White); //7px font
                }

                //debug ----------------
                if (MyData[DEBUG] == 1) {
                    //FPS
                    sprintf(buffer, "VOL=%01B" PRINTF_BINARY_PATTERN_INT8,
                            PRINTF_BYTE_TO_BINARY_INT8(MyData[VOLUME])); //
                    sprintf(buffer, "FPS=%d", fps_last); //
                    ssd1306_SetCursor(70, 10);
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
                if ((MyData[THEME] & theme_enable_vu) != 0)
                    Draw_VU(0, 58, 64 - (mute_active ? tmp_volume : MyData[VOLUME]));

            }
        }

        //label, jumped if display OFF activated
        no_draw:

        //---------------------------------------------
        // Button -------------------------------------
        //---------------------------------------------
        //Button kratak klick 10-500ms
        if (SW_timers[T_BUTTON] > 15 && SW_timers[T_BUTTON] < 500 && SW_timers_enable[0] == 0 && power_off_on == 1) {
            if (menu_active) {
                switch (MyData[MENU]) {
                    case 0:
                        menu_active = 0; //izlazimo iz menu
                        break;

                    case 1 ... 18:
                        edit_active = !edit_active; //edit mode
                        break;

                    case 19: //reset settings
                        save_change_flag = 1;
                        Reset_Settings();
                        break;

                    case 22: //power off
                        power_off_on = 0;
                        menu_active = 0;
                        MyData[MENU] = 0;
                        break;

                    case 23: //save settings
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

        skeep_update_src:

        if (power_off_on == 1) {
            //Button dugacak klick > 1000ms aktivira menu | na glavnom ekranu
            if (SW_timers[T_BUTTON] >= 1000) {
                if (!menu_active) menu_active = 1;
                SW_timers[T_BUTTON] = 0;
                SW_timers_enable[T_BUTTON] = 0;
            }
        } else {
            SW_timers_enable[T_BUTTON] = 0;
            if (Button) { //handle wakeup
                SW_timers_enable [T_WAKEUP] = 1;
            } else {
                SW_timers_enable [T_WAKEUP] = 0;
                SW_timers [T_WAKEUP] = 0;
            }
        }

        //wakeup after 1 sec button on
        if (SW_timers [T_WAKEUP] > 1000) {
            power_off_on = 1;
            SW_timers [T_WAKEUP]=0;
            SW_timers_enable [T_WAKEUP] = 0;
        }

        //pred kraj osvezi ceo ekran
        ssd1306_UpdateScreen();

    } // end while (1)
} //end main

#pragma clang diagnostic pop
