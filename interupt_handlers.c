/**************************************************************************
 * Author  Daniel S.
 * Date    10/10/2025
 * File    interupt_handlers.c
 * Version V1.0
 * Description 
 * Where all Interupt Routines are placed.
 *
 ************************************************************************/
 
/*------------------------------------------------------------------------
INCUDES
------------------------------------------------------------------------*/
 
 #include "stm32l476xx.h"
 #include "gpio_lib.h"
 
 // Make sure to clear NVIC_CearPendingIRQ(IRQn);
 
/*------------------------------------------------------------------------
GPIO ISRs
------------------------------------------------------------------------*/
 
// Make sure to also do: EXTI->PR1 |= EXTI_PR1_PIF#;
 
//Pin3
void EXTI3_IRQHandler(void){
 // Code Here
 NVIC_ClearPendingIRQ(EXTI3_IRQn);
 EXTI->PR1 |= EXTI_PR1_PIF3;
}
 
//Pin4
extern int genBotMessage;
void EXTI4_IRQHandler(void){
	genBotMessage =1;
	NVIC_ClearPendingIRQ(EXTI4_IRQn);
	EXTI->PR1 |= EXTI_PR1_PIF4;
 }
 
/*------------------------------------------------------------------------
Peripheral ISRs
------------------------------------------------------------------------*/

//Tim2
 
extern int nsCount;
void TIM2_IRQHandler(void){
	nsCount += 1000;
	NVIC_ClearPendingIRQ(TIM2_IRQn);
	TIM2->SR &= ~TIM_SR_UIF;  // clear update interrupt flag
}


