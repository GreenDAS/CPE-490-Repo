/**************************************************************************
 * Author  Daniel S.
 * Date    10/13/2025
 * File    interupt_lib.h
 * Version V1.0
 * INTERUPT
 * Description 
 * Make interupt_lib.c's Classes and Functions visable to others
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


//------------------------------------------------------------------------------
// # Classes
//------------------------------------------------------------------------------


/*---Peripheral Interupt Handling---*/
#ifndef PeripheralInteruptHandlingClass
#define PeripheralInteruptHandlingClass
typedef struct PeripheralInteruptHandling{
	//*-Properties-*//
	IRQn_Type IRQN; // The Interupt number for the peripheral
	
	//*-Function Pointers-*//
	void (*setIXER)(struct PeripheralInteruptHandling* self, char setOrDisable); // Enables or Disables the Interupt
	void (*setPriorityBit)(struct PeripheralInteruptHandling* self, uint32_t priority); // Sets the Priority of the Interupt
	void (*initCCInterupt)(TIM_TypeDef *Timer); // Sets up the Capture & Compare of a peripheral (Only TIM2 For now)
	
}PeripheralInteruptHandling;

// Class Constructor

PeripheralInteruptHandling* PeripheralInteruptHandling_Create(IRQn_Type IRQn);
#endif

#ifndef GPIOInteruptHandlingClass
#define GPIOInteruptHandlingClass
void _init_GPIOInterupt(int pin, char GPIOChar, IRQn_Type IRQn, int ccInterupt, int priority); // Does not return the structure (Purely to enable GPIO Interupts)
#endif