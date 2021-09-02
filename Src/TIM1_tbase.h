//
// Created by miki on 11/4/19.
//

#ifndef ARCUT_TIM1_TBASE_H
#define ARCUT_TIM1_TBASE_H

#define EnableTim()               (TIM1->CR1 |= TIM_CR1_CEN)
#define DisableTim()              (TIM1->CR1 &= ~TIM_CR1_CEN)
#define ClearTimCount()           (TIM1->CNT = 0)

void TIM1_Setup_TBASE (void);

#endif //ARCUT_TIM1_TBASE_H
