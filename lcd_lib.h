/**************************************************************************
 * Author  Daniel S.
 * Date    10/3/2025
 * File    lcd_lib.h
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

void _initLCD_();

//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------

#define msScalar 0.001
#define GenevaLCDRowSize 2
#define GenevaLCDRowOffset 64

#define GenevaLCDColSize 39
#define GenevaLCDColOffset 0

//------------------------------------------------------------------------------
// # Classes
//------------------------------------------------------------------------------


/*---GeneralPurposeTimer---*/
#ifndef GenevaLCDDeviceClass
#define GenevaLCDDeviceClass

#include "timer_lib.h"

// Forward declaration 
typedef struct GenevaLCDDevice{
	//*-Parents-*//
	GeneralPurposeTimer* timer; // The Timer the object uses to wait
	
	//*-Properties-*//

	int retries; // How many times should it retry communicating
	int cursorPos[2]; // Where the cursor is
	int onOffRatio; // How long the display should wait before clearing the display relative to the time the display is off
	int wholeMSG[2][GenevaLCDRowSize][GenevaLCDColSize]; // The message to send to the LCD: First Portion or 2nd Portion of msg,the row, the col
	
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

/* Class Constructor
 Arg1 = The timer the object uses
 Arg2 = How many times it should retry commands
 Arg3 = what the onOffRatio should be
*/
GenevaLCDDevice* GenevaLCDDevice_Create(GeneralPurposeTimer* Timer, int ConnectionRetries, int OnOffRatio, int MSG[2][GenevaLCDRowSize][GenevaLCDColSize]);
#endif