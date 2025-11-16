/**************************************************************************
 * Author  Daniel S.
 * Date    10/13/2025
 * File    interupt_lib.c
 * Version V1.3
 * Description 
 * A Helpful library of Interupt Functions and other Definitions
 *
 ************************************************************************/
 
 //------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "stm32l476xx.h"
#include "stdlib.h"


//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

/* Sets or Clear the ISER or ICER bit for the respective peripheral 
 Arg1 = The Object Itself
 Arg2 = Either S, to set the ISER bit, or C, to set the ICER bit
*/
void setIRQn(IRQn_Type IRQN, char setOrDisable){
	switch(setOrDisable){
		case('S'):	// Sets the ISER Bit
			__NVIC_EnableIRQ(IRQN);
			break;
		case('C'):	// Clears the ICER Bit
			__NVIC_DisableIRQ(IRQN);
			break;
		default:
			while(1){}	// Catches Improper use of the method
	}
}

//------------------------------------------------------------------------------
// # Classes
//------------------------------------------------------------------------------


/*Peripheral Interupt Handling*/

#ifndef PeripheralInteruptHandlingClassCode
#define PeripheralInteruptHandlingClassCode

// Class
typedef struct PeripheralInteruptHandling{
	//*-Properties-*//
	IRQn_Type IRQN; // The Interupt number for the peripheral
	
	//*-Function Pointers-*//
	void (*setIXER)(struct PeripheralInteruptHandling* self, char setOrDisable); // Enables or Disables the Interupt
	void (*setPriorityBit)(struct PeripheralInteruptHandling* self, uint32_t priority); // Sets the Priority of the Interupt
	void (*initCCInterupt)(TIM_TypeDef *Timer); // Sets up the Capture & Compare of a peripheral (Only TIM2 For now)
	
}PeripheralInteruptHandling;

// Class Methods

/* Sets or Clear the ISER or ICER bit for the respective peripheral 
 Arg1 = The Object Itself
 Arg2 = Either S, to set the ISER bit, or C, to set the ICER bit
*/
void setIXER(PeripheralInteruptHandling* self, char setOrDisable){setIRQn(self->IRQN,setOrDisable);}

/* Set Priority Bit of the Interupt
Arg1 = The Object itself
Arg2 = The priority level to be set too; Higher priority value means lower priority
*/
void setPriorityBit(PeripheralInteruptHandling* self, uint32_t priority){
	NVIC_SetPriority(self->IRQN, priority);
}

/* Sets up the Capture & Compare of a peripheral (Only TIM2 For now)
 Arg1 = TIMX obj (TIM2)
*/
void initCCInterupt(TIM_TypeDef *Timer){
	Timer->CCMR1 &= ~3UL; // Clear Pin Connection
	Timer->CCMR1 |= 1UL; // Set Pin Connection
	Timer->CCMR1 &= ~0xF0; // Clears Filtering bits
	Timer->CCMR1 &= ~0b1100; // Clear Prescalar
	Timer->CCER &= ~0b1011; // Clears Enable Register
	Timer->CCER |= 1UL; // Enables Interupt
	Timer->CCER &= ~TIM_CCER_CC1P;   // Clear polarity bit → rising edge
	Timer->CCER &= ~TIM_CCER_CC1NP;  // Not both edges
	Timer->DIER &= ~0b10; // Clears Interupt Enabled for Channel 1
	Timer->DIER |= 0b10; // Sets Interupt Enabled for Channel 1
}

/* Class Constructor
 Arg1 = The Peripheral's Interupt #
*/
PeripheralInteruptHandling* PeripheralInteruptHandling_Create(IRQn_Type IRQn){
	PeripheralInteruptHandling* self = malloc(sizeof(PeripheralInteruptHandling));
	self->IRQN = IRQn;
	self->setPriorityBit = setPriorityBit;
	self->setIXER = setIXER;
	self->initCCInterupt = initCCInterupt;
	return self;
}

#endif

/*GPIO Interupt Handling*/

// Class Def

#ifndef GPIOInteruptHandlingClassCode
#define GPIOInteruptHandlingClassCode

//Class
/*NOTE, THIS CLASS ONLY SETS UP THE PIN'S INTERUPT AND THEN KILLS ITSELF*/
typedef struct GPIOInteruptHandling{
	//*-Properties-*//
	IRQn_Type IRQN; // The Interupt number for the peripheral
	uint32_t clearInteruptMask; // Mask to Clear the correct bits for SYSCFG->EXTICR[pin/4]
	uint32_t setInteruptMask; // Mask to set the correct bits for SYSCFG->EXTICR[pin/4]
	uint32_t edgeMask; // Mask to Set/Clear the bits for the EXTI->RTSR1/FTSR1
	uint32_t connectInteruptToNVICMask; // Mask that connects the external interupt to the NVIC
	
	//*-Function Pointers-*//
	void (*setPinInterupt)(struct GPIOInteruptHandling* self, int pin, int priority); // Enables the interupt
	
}GPIOInteruptHandling;

// Class Methods

/* Enable the Pin's Interupt
 Arg1 = The Object Itself
 Arg2 = The IODevice's Pin #
*/
void setPinInterupt(GPIOInteruptHandling* self, int pin,int priority){
	SYSCFG->EXTICR[pin/4] &= ~self->clearInteruptMask; // Clear's pin #'s bit
	SYSCFG->EXTICR[pin/4] |= self->setInteruptMask; // Sets pin #'s bit (GPIOC)
	EXTI->RTSR1 &= ~self->edgeMask; // Masks over rising edge Register
	EXTI->FTSR1 |= self->edgeMask; // Set Falling edge register
	EXTI->IMR1 |= self->connectInteruptToNVICMask; // "Connects" the interupt to the NVIC
	NVIC_SetPriority(self->IRQN,1); // 0 is highest priority
	NVIC_EnableIRQ(self->IRQN);

}


/* Class Constructor
 Arg1 = The IODevice's Pin #
 Arg2 = The IODevice's GPIO Base (A,B,C,D,...)
 Arg3 = The IRQn number of the Interupt
*/
void _init_GPIOInterupt(int pin, char GPIOChar, IRQn_Type IRQn, int ccInterupt, int priority){
	if(!(RCC->APB2ENR & RCC_APB2ENR_SYSCFGEN)) {RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;} // Turns on Clk to Pin Interupts if not already enabled
	GPIOInteruptHandling *self = malloc(sizeof(GPIOInteruptHandling)); // Creates spot in memory
	if(ccInterupt){
		switch(GPIOChar){ // Enables the GPIO Interupt 
		case('A'):
			if (pin <= 7) {
				GPIOA->AFR[0] &= ~(1UL << (pin * 4));     // Clear old AF
				GPIOA->AFR[0] |=  (1UL << (pin * 4));     // Set AF1
			} else {
				GPIOA->AFR[1] &= ~(1UL << ((pin - 8) * 4));
				GPIOA->AFR[1] |=  (1UL << ((pin - 8) * 4));
			}
			break;
		case('B'):
		case('C'):
		case('D'):
		case('E'):
		case('F'):
		case('G'):
		default:
			while(1); // Catches Unkown GPIO Port
		}
	}
	else{
		int shift = (((pin)%4)*4);
		self->IRQN = IRQn;
		self->clearInteruptMask = 7<<shift; // Sets bit masks to 7 at ((pin X) % 4)*4
		self->setInteruptMask = 0; // Sets bit mask to 0 to prep it
		self->edgeMask = 1 <<(pin); // Sets bit masks to 1 at pin X
		self->connectInteruptToNVICMask = 1 <<(pin);  // Sets bit masks to 1 at pin X
		switch(GPIOChar){ // Sets bit masks to # at pin X
			case('A'):
				self->setInteruptMask = (0<<shift); // Sets bit mask to 0 at (pin X) % 4
				break;
			case('B'):
				self->setInteruptMask = (1<<shift); // Sets bit mask to 1 at (pin X) % 4
				break;
			case('C'):
				self->setInteruptMask = (2<<shift); // Sets bit mask to 2 at (pin X) % 4
				break;
			case('D'):
				self->setInteruptMask = (3<<shift); // Sets bit mask to 3 at (pin X) % 4
				break;
			case('E'):
				self->setInteruptMask = (4<<shift); // Sets bit mask to 4 at (pin X) % 4
				break;
			case('F'):
				self->setInteruptMask = (5<<shift); // Sets bit mask to 5 at (pin X) % 4
				break;
			case('G'):
				self->setInteruptMask = (6<<shift); // Sets bit mask to 6 at (pin X) % 4
				break;
			default:
				while(1); // Catches Unkown GPIO Port
			}
	}
	self->setPinInterupt = setPinInterupt;
	self->setPinInterupt(self, pin, priority);
	free(self); // Kills itself (Frees spot in memory)
}
#endif