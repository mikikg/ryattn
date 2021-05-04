//

void TIM4_Setup_TBASE (void);

//Za razvoj da bi radio program i bez optimizacije (zbog debugera) 
//tj sa opciom -O0 postaviti 2 jer je preveliko opterecenje ISR, mora 50kHz
//Sa ukljucenom kompajler optimizacijom -O1, -O2 ili -O3 
//za 100kHz postaviti 1
#define MY_TIM4_TIME_SCALE 1 //1=1khz 2=0.5khz ... 

