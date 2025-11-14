/**************************************************************************
 * Author  Daniel S.
 * Date    11/13/2025
 * File    main.c
 * Version V1.5
 * Completes Lab 9
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

#define FSIZE 12
#define VSIZE 4

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

void createFreqString(int msg[GenevaLCDColSize], double freq){
	char buffer[16];
	snprintf(buffer, 16, "FREQ: %4.2fHz", freq);  // 2 decimal places
	buffer[15] = 172; // Move to Line 2
	for (int i =0; i<16; i++){
		msg[i] = buffer[i];
	}
}

void createVoltString(int msg[GenevaLCDColSize], double volt){
	char buffer[16];
	snprintf(buffer, 16, "VOLTAGE: %2.2f V", volt);  // 2 decimal places
	buffer[15] = 128; // Move to Line 1
	for(int i =0; i<16; i++){
		msg[i] = buffer[i];
	}
}

void calcVoltage(GenevaLCDDevice* Disp,float voltageMeasurements[VSIZE], float* voltage){
	static int voltIndex = 0;
	ADC1->ISR |= ADC_ISR_EOC; // Clear End of Conversion Flag
	ADC1->CR |= ADC_CR_ADSTART; // Start ADC Conversion
	while((ADC1->ISR & ADC_ISR_EOC) == 0){} // Wait for Conversion to finish
	voltageMeasurements[voltIndex] = ((ADC1->DR) * (10/3))/ 255.0;
		voltIndex = (voltIndex<VSIZE) ? voltIndex + 1: 0;
		*voltage = 0;
		for(int i =0; i<VSIZE; i++){
			*voltage += voltageMeasurements[i];
		}
		*voltage /= VSIZE;
	createVoltString(&(Disp->wholeMSG[1][0][0]), *voltage); // Update Voltage String
}

void calcFrequency(GenevaLCDDevice* Disp, int freqCounts, double timeElapsed, float* frequecy){
	*frequecy = freqCounts / timeElapsed;
	createFreqString(&(Disp->wholeMSG[1][1][0]), *frequecy); // Update Frequency String
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

// Global Vars
IODevice VoltReader;
IODevice FreqReader;
GeneralPurposeTimer Timer2;
GenevaLCDDevice *Display;
float voltageMeasurements[VSIZE];
float voltage = 0;
float frequency = 0;
int freqCounts = 0;


// FLAGS
int calcVoltFlag = 0;
int calcFreqFlag = 0;


int main(void){
	_init_();	// Sets up classes and other variables
	while(True){
		// Main Loop does nothing, everything is handled in interupts
	}
}
