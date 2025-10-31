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
extern int calcVoltFlag;
void EXTI4_IRQHandler(void){
	// Code Here
	calcVoltFlag = 1;
	NVIC_ClearPendingIRQ(EXTI4_IRQn);
	EXTI->PR1 |= EXTI_PR1_PIF4;
 }
 
/*------------------------------------------------------------------------
Peripheral ISRs
------------------------------------------------------------------------*/

//Tim2
void TIM2_IRQHandler(void){
	
	if(TIM2->SR & TIM_SR_UIF) { // UIF Interrupt

		TIM2->SR &= ~TIM_SR_UIF; // Clear interrupt flag
  } 
	else if (TIM2->SR & TIM_SR_CC1IF) { // Channel #1
		TIM2->SR &= ~ TIM_SR_CC1IF; // Clear interrupt flag
  } 
	else if (TIM2->SR & TIM_SR_CC2IF) { // Channel #2
		TIM2->SR &= ~ TIM_SR_CC2IF; // Clear interrupt flag
  }
	
	NVIC_ClearPendingIRQ(TIM2_IRQn);
}


