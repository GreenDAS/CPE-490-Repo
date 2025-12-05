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
#include "globals.h"
#include "globals.h"

#define DEBOUNCE_TIME_MS 500  // 500 ms debounce time - Also done to prevent multiple presses when getting user input
		                      // Not a fan of this method, but it works for now

// Make sure to clear NVIC_CearPendingIRQ(IRQn);

/*------------------------------------------------------------------------
GPIO ISRs
------------------------------------------------------------------------*/

// Make sure to also do: EXTI->PR1 |= EXTI_PR1_PIF#;

// Pin3
void EXTI3_IRQHandler(void)
{
	// Code Here
	NVIC_ClearPendingIRQ(EXTI3_IRQn);
	EXTI->PR1 |= EXTI_PR1_PIF3;
}

// Pin4
void EXTI4_IRQHandler(void)
{

	if(Timer4.getBits(Timer4.TIMX->CR1, TIM_CR1_CEN_Pos, 0x1)){ // If Timer4 is running, ignore the button press - For Debounce
		NVIC_ClearPendingIRQ(EXTI4_IRQn);
		EXTI->PR1 |= EXTI_PR1_PIF4;
		return;
	}

	if(!gettingUserInputFlag){
		SW1LED->setState(SW1LED,1); // Toggle LED1
		sw1PressedFlag = 1;
	}

	// For Debounce
	Timer4.TIMX->ARR = DEBOUNCE_TIME_MS - 1; // Reset ARR
	Timer4.setBits(&Timer4.TIMX->CR1, TIM_CR1_CEN_Pos, 1); // Turn on Timer4

	NVIC_ClearPendingIRQ(EXTI4_IRQn);
	EXTI->PR1 |= EXTI_PR1_PIF4;
}

// Pin5
void EXTI9_5_IRQHandler(void)
{

	if(Timer4.getBits(Timer4.TIMX->CR1, TIM_CR1_CEN_Pos, 0x1)){ // If Timer4 is running, ignore the button press - For Debounce
		NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
		EXTI->PR1 |= EXTI_PR1_PIF5;
		return;
	}

	if(!gettingUserInputFlag){

		sw2PressedFlag = 1;
		gettingUserInputFlag = 1;
		SW2LED->setState(SW2LED,1); // Turn on LED2 to show we are getting user input

	}
	else{
		sw2PressedFlag = 1;
		gettingUserInputFlag = 0;
		SW2LED->setState(SW2LED,1); // Turn on LED2 to show we are getting user input
	}

	// For Debounce
	Timer4.TIMX->ARR = DEBOUNCE_TIME_MS - 1; // Reset ARR to 5 milli second
	Timer4.setBits(&Timer4.TIMX->CR1, TIM_CR1_CEN_Pos, 1); // Turn on Timer4

	NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
	EXTI->PR1 |= EXTI_PR1_PIF5;
}

/*------------------------------------------------------------------------
SysTick ISR
------------------------------------------------------------------------*/



// Currently a Cooperative RTOS & EDF
void SysTick_Handler(void)
{
	systickFlag = 1;
}

/*------------------------------------------------------------------------
Peripheral ISRs
------------------------------------------------------------------------*/

// Tim2

// Externs

// Local Vars
uint32_t timeI;
uint32_t timeF;


void TIM2_IRQHandler(void)
{

	if (TIM2->SR & TIM_SR_UIF)
	{ // UIF Interrupt

		TIM2->SR &= ~TIM_SR_UIF; // Clear interrupt flag
    } 

	else if (TIM2->SR & TIM_SR_CC1IF) { // Channel #1
		if(calcFreqFlag){
			timeI = TIM2->CCR1;
		}
		else
		{
			timeF = TIM2->CCR1;

			int deltaTime = timeI - timeF;
			double clkSpeed = (double)clockSpeedHz / (double)(Timer2.PSC + 1);

			if (deltaTime < 0)
				deltaTime += TIM2->ARR + 1; // Wrap-around
			timeElapsed += (double)deltaTime / clkSpeed;
			freqCounts++;
			timeI = timeF;

			if (timeElapsed >= (30/(targetRPM * 7)))
			{ // Every 0.5 seconds
				calcFreqFlag = 1;
			}
		}
		Timer5.TIMX->ARR = ((1/frequency) * (clockSpeedHz / Timer5.TIMX->PSC + 1));		   // Set ARR of Timer 5
		Timer5.TIMX->CNT = ((1/frequency) * (clockSpeedHz / Timer5.TIMX->PSC + 1));
		Timer5.setBits(&Timer5.TIMX->CR1, TIM_CR1_CEN_Pos, 1); // Turn on Timer5
		TIM2->SR &= ~TIM_SR_CC1IF; // Clear interrupt flag
	}
	else if (TIM2->SR & TIM_SR_CC2IF)
	{							   // Channel #2
		TIM2->SR &= ~TIM_SR_CC2IF; // Clear interrupt flag
	}

	NVIC_ClearPendingIRQ(TIM2_IRQn);
}

void TIM4_IRQHandler(void)
{

	TIM3->SR &= ~TIM_SR_UIF; // Clear interrupt flag
	NVIC_ClearPendingIRQ(TIM4_IRQn);
}