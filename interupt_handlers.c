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
 
#define VSIZE 6
#define vDeadline 100
#define fDeadline 500
#define dDeadline 600
#define lineSize 15 // 16 Max however index 15 is the control character

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
extern int calcVoltFlag;
extern int calcFreqFlag;
extern void calcVoltage(GenevaLCDDevice* Disp,float voltageMeasurements[VSIZE], float* voltage);
extern void calcFrequency(GenevaLCDDevice* Disp, int freqCounts, double timeElapsed, float* frequency);

extern GenevaLCDDevice* Display;
extern float voltageMeasurements[VSIZE];
extern float voltage;

extern float frequency;

void SysTick_Handler(void){
	static uint32_t voltDeadline = 100;
	static uint32_t freqDeadline = 500;
	static uint32_t displayDeadline = 600;
	static enum {START, UPPER, LOWER, SUCCESS} dispState = 0;
	systick_counter = systick_counter > 600 ? 0 : systick_counter++;

	if(calcVoltFlag && (((voltDeadline - systick_counter) <= (freqDeadline - systick_counter)) || ((voltDeadline - systick_counter) <= (displayDeadline - systick_counter)))){
		calcVoltage(Display,voltageMeasurements, &voltage);
		voltDeadline = voltDeadline > 600 ? vDeadline : voltDeadline + vDeadline;
		calcVoltFlag = 0;
	}
	else if(calcFreqFlag && ((freqDeadline - systick_counter) <= (displayDeadline - systick_counter))){
		calcFrequency(Display, freqCounts, timeElapsed, &frequency);
		freqDeadline = freqDeadline > 600 ? fDeadline : freqDeadline + fDeadline;
		calcFreqFlag = 0;
	}
	else // display
	{
		if(Display->lcd_Nack()){dispState = START;}else{dispState++;}
		switch (dispState)
		{
		case 0:
			Display->startTalking();
			break;
		case 1:
			Display->sendMSGBits(Display, 0); // First Portion of Message
			break;
		case 2:
			Display->sendMSGBits(Display, 1); // Second Portion of Message})
			break;
		case 3:
			switch (Display->cursorPos[0])
			{
			case 0:
				Display->cursorPos[0] = Display->cursorPos[1] > lineSize ? 1 : 0;
				break;
			case 1:
				Display->cursorPos[0] = Display->cursorPos[1] > lineSize ? 0 : 1;
			}
			Display->cursorPos[1] = Display->cursorPos[1] > lineSize ? 0 : Display->cursorPos[1]++;
			dispState = START;
			break;
		default:
			while(1); // Error Catching
		}
		
	}
	
	
	
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


