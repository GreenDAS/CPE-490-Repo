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

#define TimerPeriod1SecondInMilSeconds 1000 // hex is 0x3E8,  *10 is 0x2710

#define CountAtMilSecondRate 3999 // hex is 0xF9F

#define MilSecondsScalar 0.001

#define clockSpeedHz 4000000.0

#define TIM_CR1_CEN_Pos 0 // Counter Enable Position
#define TIM_CR1_CEN_Msk (0x1 << TIM_CR1_CEN_Pos) // 0x00000001

#define TIM_CR1_UDIS_Pos 1 // Update Disable Position
#define TIM_CR1_UDIS_Msk (0x1 << TIM_CR1_UDIS_Pos) // 0x00000010

#define TIM_CR1_URS_Pos 2 // Update Request Source Position
#define TIM_CR1_URS_Msk (0x1 << TIM_CR1_URS_Pos) // 0x00000100

#define TIM_CR1_OPM_Pos 3 // One Pulse Mode Position
#define TIM_CR1_OPM_Msk (0x1 << TIM_CR1_OPM_Pos) // 0x00001000

#define TIM_CR1_DIR_Pos 4 // Direction Position
#define TIM_CR1_DIR_Msk (0x1 << TIM_CR1_DIR_Pos) // 0x00010000



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
	int (*getBits)(uint32_t Register, int bitOffset, int bitMask); // Gets the bits in a certain register and returns them (
	int (*setBits)(uint32_t *Register, int bitOffset, int value); // Sets the bits in a certain register and returns the register (Automatically Sets the bits in the register passed)
	
}GeneralPurposeTimer;

// Constructor 
GeneralPurposeTimer* GeneralPurposeTimer_Create(int timer, int CEN, int PSC, int ARR, char DIR, int OPM);
#endif