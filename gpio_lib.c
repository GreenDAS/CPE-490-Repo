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

#ifndef timerLib
#define timerLib
#include "timer_lib.h"
#endif


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


// Class Declaration
typedef struct IODevice{
	//*-Parents-*//
	GPIO_TypeDef* GPIOX;
	
	//*-Properties-*//
		char GPIOchar;
	char MODERState;
	int pin;
	int state;
	int prevState;
	int normalState;
	int trueState;

	//*-Function Pointers-*//
	void (*setState)(struct IODevice*, int);
	void (*getState)(struct IODevice*);
	void (*setMODER)(struct IODevice*);
	void (*toggle)(struct IODevice*);
	void (*initInterupt)(int pin, char GPIOChar, IRQn_Type IRQn);
}IODevice;

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
void MODERSet(IODevice* device){
switch(device->MODERState){
	case 'I': // Input
		{
			device->GPIOX->MODER &= ~(3UL<<(2*device->pin));
			break;
		}
	case 'O': // Output
		{
			device->GPIOX->MODER &= ~(3UL<<(2*device->pin));
			device->GPIOX->MODER |= (1UL<<(2*device->pin));
			break;
		}
	case 'F': // Alt Function
		{
			device->GPIOX->MODER &= ~(3UL<<(2*device->pin));
			device->GPIOX->MODER |= (2UL<<(2*device->pin));
			break;
		}
	case 'A': // Analog
		{
			device->GPIOX->MODER |= (3UL<<(2*device->pin));
			break;
		}
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
IODevice IODevice_Create(char GPIO, int Pin, int NormalState, int TrueState, char MODERType) {
	IODevice self;
	
	switch(GPIO){
		case('A'):
			self.GPIOX = GPIOA;
			if(((RCC->AHB2ENR & RCC_AHB2ENR_GPIOAEN)>>RCC_AHB2ENR_GPIOAEN) == 0){RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;}
			break;
		case('B'):
			self.GPIOX = GPIOB;
			if(((RCC->AHB2ENR & RCC_AHB2ENR_GPIOBEN)>>RCC_AHB2ENR_GPIOBEN) == 0){RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;}
			break;
		case('C'):
			self.GPIOX = GPIOC;
			if(((RCC->AHB2ENR & RCC_AHB2ENR_GPIOCEN)>>RCC_AHB2ENR_GPIOCEN) == 0){RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;}
			break;
		case('D'):
			self.GPIOX = GPIOD;
			if(((RCC->AHB2ENR & RCC_AHB2ENR_GPIODEN)>>RCC_AHB2ENR_GPIODEN) == 0){RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;}
			break;
			default:
				while(1){} // Catches unkown GPIO port
	}
	if(((RCC->AHB2ENR & RCC_AHB2ENR_GPIOCEN)>>RCC_AHB2ENR_GPIOCEN) == 0){RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;}
	
	self.MODERState = MODERType;
	self.pin = Pin;
	self.GPIOchar = GPIO;
	self.setState = setState;
	self.getState = getState;
	self.toggle = toggle;
	self.setMODER = MODERSet;
	self.setMODER(&self);
	self.state = 0;
	self.prevState = 0;
	self.normalState = NormalState;
	self.trueState = TrueState;
	self.initInterupt = _init_GPIOInterupt;
	return self;
}
#endif




/*---Numpads---*/

// Class Declaration
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


// Class Methods

// Changes the Row or Column IO to the proper MODERType
void changeDimMODER(Numpad* self, char Dim, char MODERType){
	switch(Dim){
		case('R'): // Set RowIO to MODERType
			for(int i = 0; i<self->rowSize; i++){
				self->rowIO[i].MODERState = MODERType; 
				self->rowIO[i].setMODER(&(self->rowIO[i]));
			}
			break;
		case('C'): // Set ColIO to MODERType
			for(int j = 0; j<self->colSize; j++){
				self->colIO[j].MODERState = MODERType; 
				self->colIO[j].setMODER(&(self->colIO[j]));
			}
			break;
	}
}

/* Read Pad and stop program to wait for X amount of time to pass 
	(hence the Greedy as it steals valuable processing time by waiting.)
--To manually index throgh a flattened 2D array, algorythoim is the following--
	row = which row you want (0 … rows-1)
	cols = the total number of columns in each row (not cols-1)
	col = which column in that row (0 … cols-1)
	Array[row*cols+col]
*/
void greedyReadPad(Numpad* self){
	// Pointing out what Value was used
	int rowVal =0; // What Row was 0
	int colVal =0; // What Col was 0
	
	// Error Correction
	int truesCountRows =0; // how many 0s from Row
	int truesCountCollumns =0; // How many 0s from Col
	
	for(int i=0; i < self->rowSize; i++){self->rowIO[i].setState(&(self->rowIO[i]),0);} // Ensures the ODR for the Row is set to 0 to prevent any wonky signals
	self->changeDimMODER(self, 'R', 'I'); // Sets the row GPIO ports to Input
	self->changeDimMODER(self, 'C', 'O'); // Sets the col GPIO ports to Output
	for(int j=0; j < self->colSize; j++){self->colIO[j].setState(&(self->colIO[j]),1);} // Sets the Col to on
	
	self->timer->greedyWait(self->timer,5,1/1000); // Wait 5ms
	
	for(int i = 0; i<self->rowSize; i++){	// Read Rows And Count 0s
		self->rowIO[i].getState(&self->rowIO[i]); // Gets the state a row
		truesCountRows += self->rowIO[i].state;	// Counts up 1 if the there was a 0
		if(self->rowIO[i].state){rowVal = i;} // Remembers where the last 1 was
	}
	
	self->changeDimMODER(self, 'R', 'O'); // Sets the row GPIO ports to Output
	for(int i=0; i < self->rowSize; i++){self->rowIO[i].setState(&(self->rowIO[i]),1);} // Sets the Row to on
	for(int j=0; j < self->colSize; j++){self->colIO[j].setState(&(self->colIO[j]),0);} // Ensures the ODR for the Col is set to 0 to prevent any wonky signals
	self->changeDimMODER(self, 'C', 'I'); // Sets the col GPIO ports to Input
	
	self->timer->greedyWait(self->timer,5,1/1000); // Wait 5ms
	
	for(int j = 0; j<self->colSize; j++){ // Read Col and Count 0s
		self->colIO[j].getState(&self->colIO[j]); // Gets the state a col
		truesCountCollumns += self->colIO[j].state; // Counts up 1 if the there was a 0
		if(self->colIO[j].state){colVal = j;} // Remembers where the last 1 was
	}
	// Check to see if more or eqial to 1 button is being pressed
	if((truesCountRows >= 1) && (truesCountCollumns >= 1)){  // If not, update recent press value and state values
		self->prevState = self->state; // Updates PrevState
		self->state = 1; // Updates State
		if ((truesCountRows = 1) && (truesCountCollumns = 1)){self->recentPress = self->numpadValues[rowVal*(self->colSize)+colVal];} // If only one 1 in row and one 1 in columns, then update the recent press value
	}
	else{	// If so, update state values
		self->prevState = self->state;  // Updates PrevState
		self->state = 0;  // Updates State
	}
}
// Constructor
Numpad Numpad_Create(int *NumpadValues,IODevice* RowIO,IODevice* ColIO, int RowSize, int ColSize, int State, GeneralPurposeTimer* Timer) {
	Numpad self;
	self.numpadValues = NumpadValues;
	self.rowIO = RowIO;
	self.colIO = ColIO;
	self.rowSize = RowSize;
	self.colSize = ColSize;
	self.state = State;
	self.prevState = State;
	self.timer = Timer;
	self.changeDimMODER = changeDimMODER;
	self.greedyReadPad = greedyReadPad;
	
	return self;
}



