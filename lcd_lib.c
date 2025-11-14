/**************************************************************************
 * Author  Daniel S.
 * Date    10/3/2025
 * File    lcd_lib.c
 * Version V1.3
 * Description 
 * A Helpful library of LCD functiosn and classes and other Definitions
 *
 ************************************************************************/

//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "stm32l476xx.h"
#include "interupt_lib.h"
#include "timer_lib.h"
#include "lcd_routines.h"

//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------

#define GenevaLCDRowSize 2
#define GenevaLCDRowOffset 64

#define GenevaLCDColSize 39
#define GenevaLCDColOffset 0

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

// Initializes the LCD
void _initLCD_(){
	lcd_Init(5); // Initialized the LCD
	lcd_Command(0b1100,5); // Turns off the Cursor and blinking cursor from displaying (Improves Image)
}

//------------------------------------------------------------------------------
// Classes
//------------------------------------------------------------------------------

/*---IODevice---*/

#ifndef GenevaLCDDeviceClassCode
#define GenevaLCDDeviceClassCode
#include "stdlib.h"


// Class Declaration
typedef struct GenevaLCDDevice{
	//*-Parents-*//
	GeneralPurposeTimer* timer; // The Timer the object uses to wait
	
	//*-Properties-*//

	int retries; // How many times should it retry communicating
	int cursorPos[2]; // Where the cursor is
	int onOffRatio; // How long the display should wait before clearing the display relative to the time the display is off
	unsigned char wholeMSG[2][GenevaLCDRowSize][GenevaLCDColSize]; // The message to send to the LCD: First Portion or 2nd Portion of msg,the row, the col
	
	//*-Function Pointers-*//
	void (*moveCursor)(struct GenevaLCDDevice*, int row, int col); // Moves the cursor on the display
	void (*writeString)(struct GenevaLCDDevice*, char* string); // Writes to the display
	void (*clearDisplay)(struct GenevaLCDDevice*); // Clears the display but waits onOffRatio*screenOffTime amount of ms before clearing the display
	void (*runCommand)(struct GenevaLCDDevice*, int command); // Sends a command to the display
	void (*startTalking)(void); // Starts talking to the LCD over I2C
	int (*lcd_Nack)(void); // Checks for NACK from LCD
	void (*sendMSGBits)(struct GenevaLCDDevice*, int index); // Sends the bits from wholeMSG to the LCD
	void (*sendBits)(int data); // Sends bits to the LCD
	
}GenevaLCDDevice;

// Class Methods

/* Moves the cursor to the row and column 
 Arg1 = The Object Itself
 Arg2 = What row it should move to (0, Top; 1, Bottom)
 Arg3 = What col it should move to (0, Left to 38, Right)
*/
void greedyMoveCursor(GenevaLCDDevice* self,int row, int col){
	int command = 128 + (row*64) + (col*1); // Creates the command to send
	lcd_Command(command,self->retries); // Sends the command
	self->cursorPos[0] = row; // Update the Cursor's row Position
	self->cursorPos[1] = col; // Update the Cursor's Col Position
}

// 
/* Writes any string to the lcd
 Arg1 = The Object Itself
 Arg2 = The String to write
*/
void greedyWriteString(GenevaLCDDevice* self, char* string){
	char charAt = string[0]; // Prime charAt with a valid value
	int lineAt = 0; // What line the cursor is at (or row)
	for(int i = 0; charAt != 0x00; i++){ // Stop looping once the NULL terminator has been reached
		charAt = string[i]; // Select the right char to display
		if(charAt == '\n') { // Moves to next line when the next line char comes and skips the /n char
			lineAt++; self->moveCursor(self,lineAt,0); 
			charAt = string[i + 1]; 
			self->cursorPos[0]++; // Update the Cursor's row Position
			self->cursorPos[1] = 0; // Update the Cursor's Col Position
			continue;
		} 
		lcd_Write(charAt,self->retries); // Write the Char to the Display
		self->cursorPos[1]++; // Update the Cursor's Col Position
		charAt = string[i + 1]; // Makes the for loop look forward to stop the NULL value from being sent
	}
}

 
/* Clears the Display of the LCD
 Arg1 = The Object Itself
*/
void greedyClearDisplay(GenevaLCDDevice* self){
	self->timer->greedyWait(self->timer,(self->onOffRatio*2),0.001); // Keep Letters on for 20 ms (at 20ms, on to off time ration is 10:1)
	lcd_Command(1,self->retries); // Clears Disp
	self->timer->greedyWait(self->timer,2,0.001); // Wait for Clear to finish, (Min Time of 2ms)
}


/* Runs a given command
 Arg1 = The Object Itself
 Arg2 = The command to be sent
*/
void greedyRunCommand(GenevaLCDDevice* self, int command){lcd_Command(command, self->retries);}

/* Starts talking to the LCD over I2C
 Arg1 = None
*/
void startTalking (void) {
	
int working; //working variable
			
	I2C1->ICR = I2C_NACKCF;	//Clear the NACK flag.
	working = I2C1->CR2; //Get current value of I2C1_CR2
	working &=I2C_CR2_WRITE0; //Clear NBYTES in CR2 to prepare a new transmission.
	working |= I2C_CR2_WRITE2; //Prepare to write 2-bytes of data to the LCD.
	working |= (LCD_ADDRESS << 1); //Load I2C1 with the LCD address. Shift it by one for the 7-bit addressing mode (See reference pg. 1328)
	working |= I2C_START; //Start transmission, seeking response from LCD.
		
	I2C1->CR2 = working;
}


/* checks for NACK from LCD
 Arg1 = None
*/
int lcd_Nack(void){
	return (I2C1->ISR & I2C_NACKF) == I2C_NACKF;
}


/* Starts talking to the LCD over I2C
 Arg1 = index: which part of the wholeMSG to send (0: command/write, 1: data)
*/
void sendMSGBits(GenevaLCDDevice *self,int index){
	I2C1->TXDR = self->wholeMSG[index][self->cursorPos[0]][self->cursorPos[1]]; //Tell the LCD that a command or data value is going to be sent
}


/* Starts talking to the LCD over I2C
 Arg1 = data
*/
void sendBits(int data){
	I2C1->TXDR = data; //Sends bits to the LCD
}


/* Class Constructor
 Arg1 = The timer the object uses
 Arg2 = How many times it should retry commands
 Arg3 = what the onOffRatio should be for the greedyClearDisplay method
 Arg4 = The full message to be displayed on the LCD
*/
GenevaLCDDevice* GenevaLCDDevice_Create(GeneralPurposeTimer* Timer, int ConnectionRetries, int OnOffRatio, char MSG[2][GenevaLCDRowSize][GenevaLCDColSize]){
	GenevaLCDDevice *self = malloc(sizeof(GenevaLCDDevice));
	self->timer = Timer;
	self->retries = ConnectionRetries;
	self->moveCursor = greedyMoveCursor;
	self->writeString = greedyWriteString;
	self->clearDisplay = greedyClearDisplay;
	self->runCommand = greedyRunCommand;
	self->startTalking = startTalking;
	self->lcd_Nack = lcd_Nack;
	self->sendMSGBits = sendMSGBits;
	self->onOffRatio = OnOffRatio;
	for (int r = 0; r < GenevaLCDRowSize; r++){
		for (int c = 0; c < GenevaLCDColSize; c++){
			self->wholeMSG[0][r][c] = MSG[0][r][c];
			self->wholeMSG[1][r][c] = MSG[1][r][c];
		}
	}
	_initLCD_();
	self->moveCursor(self,0,0);
	return self;

	#endif
}
