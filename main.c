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

#define FSIZE 12
#define VSIZE 4

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

void createFreqString(char* str, double freq){
	snprintf(str, 8, "FREQ: %4.2fHz", freq);  // 2 decimal places
}

void createVoltString(char* str, double volt){
	snprintf(str, 16, "VOLTAGE: %2.2f V", volt);  // 2 decimal places
}

void calcVoltage(GenevaLCDDevice* Disp,float voltageMeasurements[VSIZE], float* voltage){
	static int voltIndex = 0;
	ADC1->ISR |= ADC_ISR_EOC; // Clear End of Conversion Flag
	ADC1->CR |= ADC_CR_ADSTART; // Start ADC Conversion
	while((ADC1->ISR & ADC_ISR_EOC) == 0){} // Wait for Conversion to finish
		voltageMeasurements[voltIndex] = ((ADC1->DR) * 3.33 )/ 255.0;
			voltIndex = (voltIndex<VSIZE) ? voltIndex + 1: 0;
			*voltage = 0;
			for(int i =0; i<VSIZE; i++){
				*voltage += voltageMeasurements[i];
			}
			*voltage /= VSIZE;
	createVoltString(&(Disp->wholeMSG[1][0][0]), *voltage); // Create Voltage String
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

// Global Vars
IODevice VoltReader;
GeneralPurposeTimer Timer2;
GenevaLCDDevice *Display;
float voltageMeasurements[VSIZE];
float voltage = 0;
char str[16] = "Voltage: 0.00 V";

// FLAGS
int calcVoltFlag = 0;


int main(void){
	_init_();	// Sets up classes and other variables
	while(True){
		
		if(calcVoltFlag == 1){
		calcVoltage(Display,voltageMeasurements, &voltage);
		calcVoltFlag = 0;
		}
	}
}
