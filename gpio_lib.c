/**************************************************************************
 * Author  Daniel S.
 * Date    10/10/2025
 * File    gpio_lib.c
 * Version V1.4
 * Description 
 * A Helpful library of GPIO Functions and other Definitions
 *
 ************************************************************************/

//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "stm32l476xx.h"
#include "gpio_lib.h"
#include "stdlib.h"


//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

/*Gets the value of a GPIO input
 Arg1 = GPIO Base
 Arg2 = Pin Number
 Arg3 = What the True value is for the pin
 If the Pin's Value == DefaultState: Return 1, Else: Return 0
*/
int GetGPIO(GPIO_TypeDef* base, int pin, uint32_t trueState){
	return ((base->IDR & (1UL<<(pin)))>>(pin)) == trueState; // 
}

/*Sets the value of a GPIO output
 Arg1 = GPIO Base
 Arg2 = Pin Number
 Arg3 = Value to set Pin (1 or 0)

*/
void SetGPIO(GPIO_TypeDef* base, int pin, int value){
	base->ODR &= ~(1UL<<(pin));	// Clears the Bit
	base->ODR |= (value<<(pin)); // Sets the Bit
}




//------------------------------------------------------------------------------
// Classes
//------------------------------------------------------------------------------

/*---IODevice---*/

#ifndef IODeviceClassCode
#define IODeviceClassCode
#include "interupt_lib.h"

// Class Methods //

/* Sets the Pin's State to a Value
***For Output Only***
*/
void setState(IODevice* self,int value) {
	self->prevState = self->state;
	self->state = value;
	if(self->MODERState != 'O'){return;}
    SetGPIO(self->GPIOX, self->pin, value);
}

/* Gets the State of the Pin
***For Input Only***
*/
void getState(IODevice* self) {
	if(self->MODERState != 'I'){return;}
	self->prevState = self->state;
    self->state = GetGPIO(self->GPIOX, self->pin, self->trueState);
}

/* Sets up the MODER Pin based on what you want it set to.
 I = Input
 O = Output
 F = Alt-Function
 A = Analog
*/ 
void setMODER(IODevice* self) {
    uint32_t pinShift = self->pin * 2;  // Each pin uses 2 bits in MODER/PUPDR
    self->GPIOX->MODER &= ~(3UL << pinShift);  // Clear MODER bits
    self->GPIOX->PUPDR &= ~(3UL << pinShift);  // Clear PUPDR bits (default to no pull)

    switch (self->MODERState) {
        case 'I':  // Input
            // MODER already cleared to 00
            self->GPIOX->PUPDR |= (2UL << pinShift);  // Set PUPDR=10 (pull-down)
            break;
        case 'O':  // Output
            self->GPIOX->MODER |= (1UL << pinShift);  // MODER=01
            // PUPDR=00 (no pull, already cleared)
            break;
        case 'F':  // Alternate Function
            self->GPIOX->MODER |= (2UL << pinShift);  // MODER=10
            // PUPDR=00 (no pull)
            break;
        case 'A':  // Analog
            self->GPIOX->MODER |= (3UL << pinShift);  // MODER=11
            // PUPDR=00 (no pull)
            break;
        default:
            while (1);  // Error trap
    }
}

/* Toggles State
***For Output Only***
*/
void toggle(IODevice* self){
	self->prevState = self->state;
	self->state ^= 1;
	if(self->MODERState != 'O'){return;}
	SetGPIO(self->GPIOX, self->pin, self->state);
}

/* Class Constructor
 Arg1 = GPIO Port Char (A,B,C,D)
 Arg2 = Pin #
 Arg3 = Normally Open = 0, Normally Closed = 1
 Arg4 = State should read true when; GPIO Pin is 0 or 1
 Arg5 = MODER Type (I, Input; O, Output;  F, Alt-Function; A, Analog)
*/
IODevice* IODevice_Create(char GPIO, int Pin, int NormalState, int TrueState, char MODERType) {
	IODevice* self = malloc(sizeof(IODevice));
	while(self == NULL){} // Out of Memmory
	
	switch(GPIO){
		case('A'):
			self->GPIOX = GPIOA;
			if(((RCC->AHB2ENR & RCC_AHB2ENR_GPIOAEN)>>RCC_AHB2ENR_GPIOAEN) == 0){RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;}
			break;
		case('B'):
			self->GPIOX = GPIOB;
			if(((RCC->AHB2ENR & RCC_AHB2ENR_GPIOBEN)>>RCC_AHB2ENR_GPIOBEN) == 0){RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;}
			break;
		case('C'):
			self->GPIOX = GPIOC;
			if(((RCC->AHB2ENR & RCC_AHB2ENR_GPIOCEN)>>RCC_AHB2ENR_GPIOCEN) == 0){RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;}
			break;
		case('D'):
			self->GPIOX = GPIOD;
			if(((RCC->AHB2ENR & RCC_AHB2ENR_GPIODEN)>>RCC_AHB2ENR_GPIODEN) == 0){RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;}
			break;
			default:
				while(1){} // Catches unkown GPIO port
	}
	if(((RCC->AHB2ENR & RCC_AHB2ENR_GPIOCEN)>>RCC_AHB2ENR_GPIOCEN) == 0){RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;}
	
	self->MODERState = MODERType;
	self->pin = Pin;
	self->GPIOchar = GPIO;
	self->setState = setState;
	self->getState = getState;
	self->toggle = toggle;
	self->setMODER = setMODER;
	self->setMODER(self);
	self->state = 0;
	self->prevState = 0;
	self->normalState = NormalState;
	self->trueState = TrueState;
	self->initInterupt = _init_GPIOInterupt;
	return self;
}
#endif




/*---Numpads---*/

// Class Methods
// Changes the Row or Column IO to the proper MODERType
void changeDimMODER(Numpad* self, char Dim, char MODERType){
	switch(Dim){
		case('R'): // Set RowIO to MODERType
			for(int i = 0; i<self->rowSize; i++){
				self->rowIO[i]->MODERState = MODERType; 
				self->rowIO[i]->setMODER(self->rowIO[i]);
			}
			break;
		case('C'): // Set ColIO to MODERType
			for(int j = 0; j<self->colSize; j++){
				self->colIO[j]->MODERState = MODERType; 
				self->colIO[j]->setMODER(self->colIO[j]);
			}
			break;
	}
}


/* Moves through steps of the Numpad Read Process at an RTOS's Discresstion.
	For my EDF RTOS, use a cooldown of 5ms & a deadline of 100ms
--To manually index throgh a flattened 2D array, algorythoim is the following--
	row = which row you want (0 - rows-1)
	cols = the total number of columns in each row (not cols-1)
	col = which column in that row (0 - cols-1)
	Array[row*cols+col]
	
*/
void stateMachineReadPad(Numpad* self){
	
	switch (self->readState){
		case readPadROW:
			self->readingRow = 0;
			self->rowsTruesCount = 0;
			

			// Read Rows
			for(int i = 0; i<self->rowSize; i++){	// Read Rows And Count 0s
				self->rowIO[i]->getState(self->rowIO[i]); // Gets the state a row
				self->rowsTruesCount += self->rowIO[i]->state;	// Counts up 1 if the there was a 0
				if(self->rowIO[i]->state){self->readingRow = i;} // Remembers where the last 1 was
			}

			// Setup Reading Cols	
			self->changeDimMODER(self, 'R', 'O'); // Sets the row GPIO ports to Output
			for(int i=0; i < self->rowSize; i++){self->rowIO[i]->setState(self->rowIO[i],1);} // Sets the Row to on
			for(int j=0; j < self->colSize; j++){self->colIO[j]->setState(self->colIO[j],0);} // Ensures the ODR for the Col is set to 0 to prevent any wonky signals
			self->changeDimMODER(self, 'C', 'I'); // Sets the col GPIO ports to Input

			break;
		case readPadCOL:
			self->colsTruesCount = 0;
			self->readingCol = 0;

			// Read Cols
			for(int j = 0; j<self->colSize; j++){ // Read Col and Count 0s
				self->colIO[j]->getState(self->colIO[j]); // Gets the state a col
				self->colsTruesCount += self->colIO[j]->state; // Counts up 1 if the there was a 0
				if(self->colIO[j]->state){self->readingCol = j;} // Remembers where the last 1 was
			}

			// Setup Reading Rows
			for(int i=0; i < self->rowSize; i++){self->rowIO[i]->setState(self->rowIO[i],0);} // Ensures the ODR for the Row is set to 0 to prevent any wonky signals
			self->changeDimMODER(self, 'R', 'I'); // Sets the row GPIO ports to Input
			self->changeDimMODER(self, 'C', 'O'); // Sets the col GPIO ports to Output
			for(int j=0; j < self->colSize; j++){self->colIO[j]->setState(self->colIO[j],1);} // Sets the Col to on

			break;
		case readPadFINISHED:
			// Check to see if more or equal to 1 button is being pressed
			if((self->rowsTruesCount >= 1) && (self->colsTruesCount >= 1)){  // If so, reflect a press
				self->prevState = self->state; // Updates PrevState
				self->state = 1; // Updates State
				if ((self->rowsTruesCount == 1) && (self->colsTruesCount == 1)){ // If only one 1 in row and one 1 in columns, then update the recent press value
					self->recentPress = self->numpadValues[self->readingRow*(self->colSize)+self->readingCol];
				}
			}

			// If not, update state values to reflect no press
			else{	
				self->prevState = self->state;  // Updates PrevState
				self->state = 0;  // Updates State
			}
			break;
	}
	// Have I finished?
	if(self->readState == readPadFINISHED){ // If So, Reset State
		self->readState = readPadROW;
	}
	// If not, increment
	else{(self->readState)++;}
}


/* Constructor
 Arg1 = Pointer to a 2D RowSize by ColSize array
 Arg2 = An 1D RowSize Array of IODevices
 Arg3 = An 1D ColSize Array of IODevices
 Arg4 = # of rows on Numpad
 Arg5 = # of cols on Numpad
 */
Numpad* Numpad_Create(int *NumpadValues, IODevice** RowIO, IODevice** ColIO, int RowSize, int ColSize) {
	Numpad* self = malloc(sizeof(Numpad));
	self->numpadValues = NumpadValues;
	self->rowIO = RowIO;
	self->colIO = ColIO;
	self->rowSize = RowSize;
	self->colSize = ColSize;

	self->prevState = 0; // T/F value if a button was previously pressed
	self->state = 0;  // T/F value if a button was pressed
	self->recentPress = 0; // Numpad Value if a button is pressed

	self->readingRow = 0; // Used to point out what row is being read from: init as 0
	self->rowsTruesCount = 0; // Used to check how many rows are reporting being pressed: init as 0
	self->readingCol = 0; // Used to point out what col is being read from: init as 0
	self->colsTruesCount = 0; // Used to check how many cols are reporting being pressed: init as 0
	self->readState = readPadROW; // The Numpad Read State

	self->changeDimMODER = changeDimMODER;
	self->stateMachineReadPad = stateMachineReadPad;
	
	return self;
}



