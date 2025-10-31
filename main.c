/**************************************************************************
 * Author  Daniel S.
 * Date    10/17/2025
 * File    main.c
 * Version V1.5
 * Completes Lab 7
 *
 **************************************************************************
*/
//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "_init_.h"
#include "gpio_lib.h"
#include "timer_lib.h"
#include "stdio.h"
#include "stdlib.h"
#include "lcd_lib.h"
#include "string.h"
#include "string_lib.h"

//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------

#define SIZE 12

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

void createFreqString(char* str, double freq){
	snprintf(str, 8, "%.2f", freq);  // 2 decimal places
}

void createVoltString(char* str, double volt){
	snprintf(str, 15, "%.2f", volt);  // 2 decimal places
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

// Global Vars
IODevice VoltReader;
GeneralPurposeTimer Timer2;
GenevaLCDDevice Display;
char str[15] = "Voltage: 0.00 V";

// FLAGS
int calcVoltFlag = 0;

 
int main(void){
	_init_();	// Sets up classes and other variables
	float voltage = 0;
	
	while(True){
		
		if(calcVoltFlag == 1){
		ADC1->ISR |= ADC_ISR_EOC; // Clear End of Conversion Flag
		ADC1->CR |= ADC_CR_ADSTART; // Start ADC Conversion
		while((ADC1->ISR & ADC_ISR_EOC) == 0){} // Wait for Conversion to finish
		voltage = ((ADC1->DR) * 3.33 )/ 256.0; // Calculate Voltage
		createVoltString(str, voltage); // Create Voltage String
		calcVoltFlag = 0;
		Timer2.greedyWait(&Timer2, 5, MilSecondsScalar); // Wait 5ms to debounce button press
		}

		Display.moveCursor(&Display,0,0);
		Display.writeString(&Display,str);
		Display.clearDisplay(&Display);
	}
}
