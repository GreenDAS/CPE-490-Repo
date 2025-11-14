/**************************************************************************
 * Author  Daniel S.
 * Date    10/13/2025
 * File    gpio_lib.h
 * Version V1.4
 * Description 
 * Make gpio_lib.c's classes and functions visable to others
 *
 *************************************************************************/
//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------


#include "stm32l476xx.h"


//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------

int GetGPIO(GPIO_TypeDef*, int, uint32_t);

void SetGPIO(GPIO_TypeDef*, int, int);

//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// # Classes
//------------------------------------------------------------------------------



/*---IODevice---*/
#ifndef IODeviceClass
#define IODeviceClass

#include "interupt_lib.h"


// Forward declaration 
typedef struct IODevice{
	//*-Parents-*//
	GPIO_TypeDef* GPIOX;
	//*-Properties-*//

	char GPIOchar; // What GPIO Bus the the IODevice uses
	char MODERState; // What MODER Type the the IODevice is
	int pin; // What Pin the the IODevice uses
	int state; // What the most recent state of the IODevice is
	int prevState; // What the most recent state of the IODevice was
	int normalState; // What the state at the IODevice's Pin is normaly (NO, 0; NC, 1)
	int trueState; // What the sate at the IODevice's Pin is when true

	//*-Function Pointers-*//
	void (*setState)(struct IODevice*, int); // Sets the State of the IODevice
	void (*getState)(struct IODevice*); // Gets the state of the IODevice
	void (*setMODER)(struct IODevice*); // Sets the MODER of the IODevice
	void (*toggle)(struct IODevice*); // Toggles the State of the IODevice
	void (*initInterupt)(int pin, char GPIOChar, IRQn_Type IRQn, int ccInterupt, int priority);	// Initializes the IODevice's Interupt
}IODevice;

/* IODevice Constructor
 Arg1 = GPIO Port Char (A,B,C,D)
 Arg2 = Pin #
 Arg3 = Normally Open = 0, Normally Closed = 1
 Arg4 = State should read true when; GPIO Pin is 0 or 1
 Arg5 = MODER Type (I, Input; O, Output;  F, Alt-Function; A, Analog)
*/
IODevice IODevice_Create(char GPIO, int Pin, int NormalState, int TrueState, char MODERType); // Creates and IO Device
#endif



/*---Numpad---*/
#ifndef NumpadClass
#define NumpadClass

	#include "timer_lib.h"

// Forward Delclaration
typedef struct Numpad{
	//*-Parents-*//
	//*-Properties-*//
	GeneralPurposeTimer* timer; // The timer used to wait
	int rowSize; // the rowSize of the numPad
	int colSize; // the colSize of the numPad
	int prevState; // T/F value if a button was previously pressed
	int state;  // T/F value if a button was pressed
	int recentPress; // Numpad Value if a button is pressed
	//*-Array Pointers-*//
	IODevice *rowIO; // Points to an array of GPIO ports for the rows of the Numpad
	IODevice *colIO; // Points to an array of GPIO ports for the cols of the Numpad
	int *numpadValues; // Points to a 2D flattened array of int values that holds the Numpad Key's Values
	//*-Function Pointers-*//
	void (*changeDimMODER)(struct Numpad*, char Dimension, char MODERType); // Changes either the row's or cols MODER
	void (*greedyReadPad)(struct Numpad*); // Hog Processor time with greedy waits to read the numpad
	
}Numpad;

// Constructor
Numpad Numpad_Create(int *NumpadValues,IODevice *RowIO,IODevice *ColIO, int RowSize, int ColSize, int State, GeneralPurposeTimer* Timer);
#endif