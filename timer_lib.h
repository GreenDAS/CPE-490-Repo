/**************************************************************************
 * Author  Daniel S.
 * Date    10/13/2025
 * File    timer_lib.h
 * Version V1.2
 * Description 
 * Make timer_lib.c's Classes and Functions visable to others
 *
 *************************************************************************/
//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "stm32l476xx.h"

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------

#define TimerPeriod1SecondInMilSeconds 1000

#define CountAtMilSecondRate 3999

#define MilSecondsScalar 0.001

//------------------------------------------------------------------------------
// # Classes
//------------------------------------------------------------------------------


/*---GeneralPurposeTimer---*/
#ifndef GeneralPurposeTimerClass
#define GeneralPurposeTimerClass

#include "interupt_lib.h"

// Forward declaration 
typedef struct GeneralPurposeTimer{
	//*-Parents-*//
	PeripheralInteruptHandling* InteruptHandler; // Has helpful methods that handles all interupt stuff
	TIM_TypeDef* TIMX; // The Timer that the class is based off of
	
	//*-Properties-*//
	int PSC; // The expected prescale register value

	//*-Function Pointers-*//
	int (*greedyWait)(struct GeneralPurposeTimer*, int SecondsToWait, float Scalar); // Waits for a desired amount of time. (Holds the Processor Hostage)
	int (*getBits)(int Register, int bitOffset, int bitMask); // Gets the bits in a certain register and returns them (
	int (*setBits)(uint32_t *Register, int bitOffset, int value); // Sets the bits in a certain register and returns the register (Automatically Sets the bits in the register passed)
	
}GeneralPurposeTimer;

// Constructor 
GeneralPurposeTimer GeneralPurposeTimer_Create(int timer, int CEN, int PSC, int ARR, char DIR, int OPM);
#endif