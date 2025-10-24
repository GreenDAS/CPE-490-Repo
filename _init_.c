/**************************************************************************
 * Author  Daniel S.
 * Date    10/13/2025
 * File    _init_.c
 * Version V1.5
 * Description 
 * INIT
 *
 *************************************************************************/
//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "_init_.h"
#include "gpio_lib.h"
#include "timer_lib.h"
#include "stdlib.h"
#include "lcd_lib.h"
#include "string.h"
#include "string_lib.h"

//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

// NUMPAD SETUP //
Numpad CreateNumpad(){
	// Start Setting Up Numpad
	static int NumpadValues[4][3] = {
		{ 1, 2, 3},
		{ 4, 5, 6},
		{ 7, 8, 9},
		{10, 0,11}
	};
	
	IODevice* rowIOP = malloc(4 * sizeof(IODevice));
	IODevice* colIOP = malloc(3 * sizeof(IODevice));
	
	rowIOP[0] = IODevice_Create('A', ROW1, 0, 1, 'I');
	rowIOP[1] = IODevice_Create('A', ROW2, 0, 1, 'I');
	rowIOP[2] = IODevice_Create('A', ROW3, 0, 1, 'I');
	rowIOP[3] = IODevice_Create('A', ROW4, 0, 1, 'I');

	colIOP[0] = IODevice_Create('A', COL1, 0, 1, 'I');
	colIOP[1] = IODevice_Create('A', COL2, 0, 1, 'I');
	colIOP[2] = IODevice_Create('A', COL3, 0, 1, 'I');
	
	GeneralPurposeTimer* Timer2 = malloc(sizeof(GeneralPurposeTimer));
	*Timer2 = GeneralPurposeTimer_Create(2,1,3999,10000,'D',0);

	return Numpad_Create(&NumpadValues[0][0], rowIOP, colIOP, 4, 3, 0, Timer2);
	
}


// TIMER SETUP //
/* Sets up the Systick Timer
*/
void InitSysTick(int load){
	SysTick->LOAD = load; // Tells the Systick Timer to a max of 0xFFFFFF
	SysTick->CTRL |= (5UL); // Puts 1s into Systick Enable and Clk source
}

/* General Timer Init
*** Timer Time Math
	PSC = X+1
	ClkInput/PSC = Counts/second; 1/Counts/second = seconds/count
***
 Arg1 = TimerX
 Arg2 = Pre Scale Register
 Arg3 = Auto Reload Register
 Arg4 = Direction (U for Up Counter, D for Down Counter)
 Arg5 = One Pulse Mode (Dissables CEN after The clock runs up or down)
*/
void InitGeneralTimer(int timer, int PSC, int ARR, char DIR, int OPM){
	
	TIM_TypeDef* TIMx;
	int CR1 = 1;
	// int CR2 = 0;
	switch(timer){ // Finds what timer should be initalized
		case 2: 
			  TIMx = TIM2;
				RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
			break;
		case 3: 
			  TIMx = TIM3;
				RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;
			break;
		case 4: 
			  TIMx = TIM4;
				RCC->APB1ENR1 |= RCC_APB1ENR1_TIM4EN;
			break;
		case 5: 
			  TIMx = TIM5;
				RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
			break;
		default:
			return;
	}
	
	switch(DIR){ // Sets the Direction Bit in CR1
		case 'U':
			CR1 &= ~(1UL<<4);
			break;
		case 'D':
			CR1 |= (1UL<<4);
			break;
	}
	switch(OPM){ // Sets the One Pulse Mode Bit in CR1
		case 0:
			CR1 &= ~(1UL<<3);
			break;
		case 1:
			CR1 |= (1UL<<3);
			break;
	}
	
	TIMx->PSC = PSC;
	TIMx->ARR = ARR;
	TIMx->CR1 = CR1;
}


//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

extern IODevice FreqReader;
extern GeneralPurposeTimer Timer2;
GeneralPurposeTimer Timer3;
extern GenevaLCDDevice Display;
//extern StringClass* str;
extern const int size;

void _init_(){
	FreqReader = IODevice_Create('A',0,0,1,'F');
	FreqReader.initInterupt(FreqReader.pin,FreqReader.GPIOchar,EXTI0_IRQn,1);
	
	Timer2 = GeneralPurposeTimer_Create(2,0,0,(4000000000UL),'D',0); // Clock with 1/2 seconds counter
	Timer2.InteruptHandler = PeripheralInteruptHandling_Create(TIM2_IRQn);
	//Timer2.TIMX->DIER |= TIM_DIER_UIE; // Enables TIM2's interupt (TIM2's Side)
	Timer2.InteruptHandler->setPriorityBit(Timer2.InteruptHandler,1); // Sets the Priority Bit
	Timer2.InteruptHandler->setIXER(Timer2.InteruptHandler,'S'); // Enables the interupt in the NVIC
	Timer2.InteruptHandler->initCCInterupt(Timer2.TIMX);
	Timer2.setBits(&(Timer2.TIMX->CR1),0,1);
	
	Timer3 = GeneralPurposeTimer_Create(3,1,3999,10000,'D',0); // 1 count is 1ms
	Display = GenevaLCDDevice_Create(&Timer3,5,10); // Stays on for 500ms off for 2ms
	
//	str = StringClass_Create("\0",16);
}