/**************************************************************************
 * Author  Daniel S.
 * Date    10/13/2025
 * File    _init_.c
 * Version V1.5
 * Description 
 * INIT
 *
 *************************************************************************/
//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "_init_.h"
#include "gpio_lib.h"
#include "timer_lib.h"
#include "stdlib.h"
#include "lcd_lib.h"
#include "string.h"
#include "string_lib.h"
#include "lcd_routines.h"

//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

// NUMPAD SETUP //
Numpad CreateNumpad(){
	// Start Setting Up Numpad
	static int NumpadValues[4][3] = {
		{ 1, 2, 3},
		{ 4, 5, 6},
		{ 7, 8, 9},
		{10, 0,11}
	};
	
	IODevice* rowIOP = malloc(4 * sizeof(IODevice));
	IODevice* colIOP = malloc(3 * sizeof(IODevice));
	
	rowIOP[0] = IODevice_Create('A', ROW1, 0, 1, 'I');
	rowIOP[1] = IODevice_Create('A', ROW2, 0, 1, 'I');
	rowIOP[2] = IODevice_Create('A', ROW3, 0, 1, 'I');
	rowIOP[3] = IODevice_Create('A', ROW4, 0, 1, 'I');

	colIOP[0] = IODevice_Create('A', COL1, 0, 1, 'I');
	colIOP[1] = IODevice_Create('A', COL2, 0, 1, 'I');
	colIOP[2] = IODevice_Create('A', COL3, 0, 1, 'I');
	
	GeneralPurposeTimer* Timer2 = malloc(sizeof(GeneralPurposeTimer));
	*Timer2 = GeneralPurposeTimer_Create(2,1,3999,10000,'D',0);

	return Numpad_Create(&NumpadValues[0][0], rowIOP, colIOP, 4, 3, 0, Timer2);
	
}


// TIMER SETUP //
/* Sets up the Systick Timer
*/
void InitSysTick(int load){
	SysTick->LOAD = load; // Tells the Systick Timer to a max of 0xFFFFFF
	SysTick->CTRL |= (5UL); // Puts 1s into Systick Enable and Clk source
}



//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

extern IODevice VoltReader;
IODevice Switch1;
extern GeneralPurposeTimer Timer2;
GeneralPurposeTimer Timer3;
extern GenevaLCDDevice *Display;
//extern StringClass* str;
extern const int size;

void _init_(){
	Switch1 = IODevice_Create('C', SW1, True, False, 'I'); // Sets up Switch1 (PC4)
	Switch1.initInterupt(Switch1.pin, Switch1.GPIOchar, EXTI4_IRQn, 0); // Sets up Switch1 Interupt (PC4)

	Timer2 = GeneralPurposeTimer_Create(2,1,CountAtMilSecondRate,TimerPeriod1SecondInMilSeconds*10,'D',0); // Sets up Timer2
	Timer3 = GeneralPurposeTimer_Create(3,1,CountAtMilSecondRate,TimerPeriod1SecondInMilSeconds*10,'D',0); // Sets up Timer3

	int msg[2][GenevaLCDRowSize][GenevaLCDColSize] = {
		{ // First Portion of Message
			//1st Row
			{LCD_CTRL_COMMAND, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA,
			 LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA,
			 LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA,
			 LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA,
			 LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA},
			//2nd Row
			{LCD_CTRL_COMMAND, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA,
			 LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA,
			 LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA,
			 LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA,
			 LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA, LCD_CTRL_DATA}
		},
		{ // Second Portion of Message
			//1st Row
			{128, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
			//2nd Row
			{172, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
		}
	};

	Display = GenevaLCDDevice_Create(&Timer2, 5, 10, msg); // Sets up LCD Display

	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN; // Enable ADC Clock
	if ((ADC1->CR & ADC_CR_DEEPPWD) == ADC_CR_DEEPPWD){ADC1->CR &= ~ADC_CR_DEEPPWD;} // Wake up ADC from Deep Power Down
	ADC1->CR |= ADC_CR_ADVREGEN; // Enable ADC Voltage Regulator
	Timer2.greedyWait(&Timer2, 1, MilSecondsScalar); // Wait for ADC Voltage Regulator to start up (min 10us)
	VoltReader = IODevice_Create('A', 0, 1, 0, 'A'); // Sets up VoltReader ADC Pin (PA0)
	VoltReader.GPIOX->ASCR |= (1UL); // Enable Analog Switch for PA0
	ADC1->SQR1 |= 0x140UL; // Set ADC to use channel 0 (PA0) as 1st conversion
	ADC123_COMMON->CCR &= ~(0xFUL); // Divide by 1 for ADC clock
	ADC123_COMMON->CCR &= ~(0x30000UL); // Setup the ADC1 timer so that it will take the HCLK w/ a divide by 1
	ADC123_COMMON->CCR |= (0x10000UL);
	ADC1->CFGR |= (0x2UL<<3); // Set ADC to 8 bit resolution
	ADC1->SMPR1 |= (1<<15); // Set ADC Channel 0 Sample time to 6.5 cycles
	ADC1->ISR |= ADC_ISR_ADRDY; // Clear ADC Ready flag
	ADC1->CR |= ADC_CR_ADEN; // Enable ADC
	while((ADC1->ISR & ADC_ISR_ADRDY) == 0){} // Wait for ADC to be ready
}