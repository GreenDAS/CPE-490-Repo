/**************************************************************************
 * Author  Daniel S.
 * Date    11/13/2025
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
 #include "timer_lib.h"
 
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
SysTick ISR
------------------------------------------------------------------------*/

// Add your SysTick counter or flags here if needed
volatile uint32_t systick_counter = 0;

void SysTick_Handler(void){
	// This handler is called at every SysTick interrupt
	systick_counter++;
	// Add your code here - no need to clear flags manually for SysTick
	// The hardware automatically clears the interrupt when you read from ICSR
}

/*------------------------------------------------------------------------
Peripheral ISRs
------------------------------------------------------------------------*/

//Tim2
// Externs
extern int calcFreqFlag;
extern int freqCounts;
extern GeneralPurposeTimer *Timer2;
// Local Vars
uint32_t timeI;
uint32_t timeF;
double timeElapsed;
void TIM2_IRQHandler(void){
	
	if(TIM2->SR & TIM_SR_UIF) { // UIF Interrupt

		TIM2->SR &= ~TIM_SR_UIF; // Clear interrupt flag
  } 
	else if (TIM2->SR & TIM_SR_CC1IF) { // Channel #1
		if(calcFreqFlag){
			timeI = TIM2->CCR1;
		}
		else{
			freqCounts++;
			timeF = TIM2->CCR1;
			if(timeF >= timeI){
				timeElapsed += ((double)(timeF - timeI) + Timer2->TIMX->ARR)*((Timer2->PSC+1)/clockSpeedHz); // in seconds
			}
			else{
				timeElapsed += ((double)(timeI - timeF))*((Timer2->PSC+1)/clockSpeedHz); // in seconds
			}
			timeI = timeF;
			if(timeElapsed >= 0.5){ // Every 0.5 seconds
				calcFreqFlag = 1;
			}
		}
		TIM2->SR &= ~ TIM_SR_CC1IF; // Clear interrupt flag
  } 
	else if (TIM2->SR & TIM_SR_CC2IF) { // Channel #2
		TIM2->SR &= ~ TIM_SR_CC2IF; // Clear interrupt flag
  }
	
	NVIC_ClearPendingIRQ(TIM2_IRQn);
}


