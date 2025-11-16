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
 #include "lcd_lib.h"
 
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
void EXTI4_IRQHandler(void){
	// Code Here
	NVIC_ClearPendingIRQ(EXTI4_IRQn);
	EXTI->PR1 |= EXTI_PR1_PIF4;
 }
 
/*------------------------------------------------------------------------
SysTick ISR
------------------------------------------------------------------------*/

//Flags
int systickFlag = 0;

// Currently a Cooperative RTOS & EDF
void SysTick_Handler(void){ 
	systickFlag = 1;
}

/*------------------------------------------------------------------------
Peripheral ISRs
------------------------------------------------------------------------*/

//Tim2

// Externs
extern int calcFreqFlag;
extern int freqCounts;
extern GeneralPurposeTimer* Timer2;

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
			timeF = TIM2->CCR1;

			int deltaTime = timeI - timeF;
			double clkSpeed = (double)clockSpeedHz / (double)(Timer2->PSC + 1);

			if (deltaTime < 0) deltaTime += TIM2->ARR + 1;    // Wrap-around
			timeElapsed += (double)deltaTime / clkSpeed;
			freqCounts++;
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


