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



//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

void createFreqString(char* str, double freq){
	snprintf(str, 8, "%.2f", freq);  // 2 decimal places
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

// Global Vars
uint32_t freqCounts = 0;
uint32_t countInitial = 0;
uint32_t countFinal = 0;
IODevice FreqReader;
GeneralPurposeTimer Timer2;
GenevaLCDDevice Display;
char str[8] = "0000.00";

// FLAGS
int calcFreqFlag = 0;

 
int main(void){
	_init_();	// Sets up classes and other variables
	float freq = 0;
	const int size = 12;
	double freqMeasurements[size];
	int measurementsFilledTo = 0;
	
	while(True){
		
		freqMeasurements[measurementsFilledTo] = ((4000000.0/(Timer2.PSC+1))/freqCounts);
		measurementsFilledTo = (measurementsFilledTo<size) ? measurementsFilledTo + 1: 0;
		freq = 0;
		for(int i =0; i<size; i++){
			freq += freqMeasurements[i];
		}
		freq /= size;
		createFreqString(str, freq);
		
		Display.moveCursor(&Display,0,0);
		Display.writeString(&Display,str);
		Display.clearDisplay(&Display);
	}
}
