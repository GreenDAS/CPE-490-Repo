/**************************************************************************
 * Author  Daniel S.
 * Date    10/13/2025
 * File    timer_lib.c
 * Version V1.4
 * Description 
 * A Helpful library of Timer Functions and other Definitions
 *
 ************************************************************************/

//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "stm32l476xx.h"
#include "interupt_lib.h"

//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// Classes
//------------------------------------------------------------------------------

/*---GeneralPurposeTimer---*/

// Class Declaration
typedef struct GeneralPurposeTimer{
	//*-Parents-*//
	PeripheralInteruptHandling* InteruptHandler; // Has helpful methods that handles all interupt stuff
	TIM_TypeDef* TIMX; // The Timer that the class is based off of
	
	//*-Properties-*//
	int PSC; // The expected prescale register value

	//*-Function Pointers-*//
	int (*greedyWait)(struct GeneralPurposeTimer*, int SecondsToWait, float Scalar); // Waits for a desired amount of time. (Holds the Processor Hostage)
	int (*getBits)(int Register, int bitOffset, int bitMask); // Gets the bits in a certain register and returns them (
	int (*setBits)(int *Register, int bitOffset, int value); // Sets the bits in a certain register and returns the register (Automatically Sets the bits in the register passed)
	
}GeneralPurposeTimer;

// Class Methods


/* Hold Precessor Hostage Until Waiting is Finished
***
	Internal Clock is 40000000 counts/sec
	countSpeed = 40000000/(PSC+1) if PSC = 3999, then count speed is 1000 count /s or 1 counts/ms
***
 Arg1 = The Class Object itself
 Arg2 = # of seconds to wait
 Arg3 = Scalar allows for User to wait for Scales of seconds i.e. Gs,Ms,Ks,ms,us,ns. (Passed to the Function as a floating point value)
*/
int greedyWait(GeneralPurposeTimer* self, int SecondsToWait, float Scalar){
	int timeI = self->TIMX->CNT; // Gets the initial time
	int timeF = timeI; // Ensures timeF has a meaningful value
	int waitError = 1; // Default Error wait to True
	int countSpeed = 4000000/(self->TIMX->PSC + 1); // Counts/Second
	uint32_t countsToWait = SecondsToWait*countSpeed*Scalar; // S * C/S * Scalar = C*Scalar (Scalar allows for User to wait for Scales of seconds. i.e. Gs,Ms,Ks,ms,us,ns)
	uint32_t countsWaited = 0; // Initalizes how long the program has waited for
	while(countsWaited<countsToWait){ // Loop so long as countsWaited is less than countsToWait
		timeF = self->TIMX->CNT; // Grabs current count
		
		switch(((self->getBits(self->TIMX->CR1,4,1))>>4)){ // Checks Direction of Clock
			case(0): // Up Counter
				if(timeF < timeI){countsWaited += timeI-timeF + self->TIMX->ARR - timeI;} // Corrects for Overflow
				else{countsWaited += timeF-timeI;} // Adds counts waited
				break;
			case(1): // Down Counter
				if(timeF > timeI){countsWaited += timeI-timeF + timeI;}  // Corrects for Overflow
				else{countsWaited += timeI-timeF;} // Adds counts waited
				break;
		}
	
		timeI = timeF; // Updates Time Initial
	}
	if(countsWaited<countsToWait){waitError = 0;} // No Error Occured
	return waitError; // Returns 1 if count did not wait long enough Otehrwise 0
}



/* Get Register's Bit(s)
 Arg1 = The Register to grab bits off of
 Arg2 = How far Left is the bit from the LSB to grab
 Arg3 = What bits to grab at the offest
*/
int getBits(int Register, int bitOffset, int bitMask){
	return (Register & (bitMask<<bitOffset)); // Gets the bits desired from the register
}
 
/* Set Register's Bit(s)
 Arg1 = The Register to set bits to (Passed by Reference)
 Arg2 = How far Left is the bit from the LSB to set
 Arg3 = What bits to set at the offest
*/
int setBits(int *Register, int bitOffset, int value){
	*Register &= (value<<bitOffset); // Clears the bits to set
	*Register |= (value<<bitOffset); // Sets the bits to set
	return *Register; // Returns the register's value
}
	

/* Class Constructor
 Arg1 = The General Purpose Timer #
 Arg2 = Should the clock be enabled to start (0, Off; 1, On)
 Arg3 = Prescale Register Value (what divides 4E6 by to get the timer's clk speed)
 Arg4 = Auto Reload Register Value (What the timer reloads to)
 Arg5 = Is the Clk upcounting or down counting (U, Up; D, Down)
 Arg6 = Should One Pulse Mode be enabled on the timer (0, Off; 1, On)
*/
GeneralPurposeTimer GeneralPurposeTimer_Create(int timer, int CEN, int PSC, int ARR, char DIR, int OPM) {
	GeneralPurposeTimer self = {0};
	int CR1 = CEN; // starts to set up CR1
	// int CR2 = 0;
	switch(timer){ // Finds what timer should be initalized and initializes the clock's bus
		case 2: 
			  self.TIMX = TIM2; 
				RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
			break;
		case 3: 
			  self.TIMX = TIM3;
				RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;
			break;
		case 4: 
			  self.TIMX = TIM4;
				RCC->APB1ENR1 |= RCC_APB1ENR1_TIM4EN;
			break;
		case 5: 
			  self.TIMX = TIM5;
				RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
			break;
		default:
			while(1){}
	}
	switch(DIR){ // Sets the Direction Bit in CR1
		case 'U':
			CR1 &= ~(1UL<<4); // Clears it
			break;
		case 'D':
			CR1 |= (1UL<<4); // Sets it
			break;
	}
	CR1 &= ~(1UL<<3);// Clears the One Pulse Mode Bit in CR1
	CR1 |= (OPM<<3);// Sets the One Pulse Mode Bit in CR1 to OPM
	
	self.TIMX->PSC = PSC;
	self.PSC = PSC;
	self.TIMX->ARR = ARR;
	self.TIMX->CR1 = CR1;
	self.greedyWait = greedyWait;
	self.getBits = getBits;
	self.setBits = setBits;
	return self;
}




