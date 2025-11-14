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
#define vDeadline 200
#define fDeadline 500
#define dDeadline 600
#define systick_counterMax 600
#define lineSize 16 // 16 Max however index 15 is the control character

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

void createFreqString(unsigned char msg[GenevaLCDColSize], double freq){
	snprintf((char*)msg, 16, "FREQ: %04.2fHz", freq);  // 2 decimal places
	msg[15] = 172; // Move to Line 2
}

void createVoltString(unsigned char msg[GenevaLCDColSize], double volt){
	snprintf((char*)msg, 16, "VOLTAGE: %2.2f V", volt);  // 2 decimal places
	msg[15] = 128; // Move to Line 1
}

void readVoltage(int* voltageMeasurements, float* voltage){
	ADC1->ISR |= ADC_ISR_EOC; // Clear End of Conversion Flag
	ADC1->CR |= ADC_CR_ADSTART; // Start ADC Conversion
	while((ADC1->ISR & ADC_ISR_EOC) == 0){} // Wait for Conversion to finish
	*voltage += ((ADC1->DR) * (10/3))/ 255.0;
	*voltageMeasurements++;
}

void calcVoltage(GenevaLCDDevice* Disp,int* voltageMeasurements, float* voltage){
	createVoltString(&(Disp->wholeMSG[1][0][0]), *voltage/(*voltageMeasurements)); // Update Voltage String
	*voltage = 0;
	*voltageMeasurements = 0;
}

void calcFrequency(GenevaLCDDevice* Disp, int* freqCounts, double* timeElapsed){
	createFreqString(&(Disp->wholeMSG[1][1][0]), *freqCounts / *timeElapsed); // Update Frequency String
	*freqCounts = 0;
	*timeElapsed = 0;
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

// Global Vars
IODevice VoltReader;
IODevice FreqReader;
GeneralPurposeTimer Timer2;
GenevaLCDDevice *Display;
int voltageMeasurements;
float voltage = 0;
float frequency = 0;
int freqCounts = 0;
extern double timeElapsed;

// FLAGS
int calcVoltFlag = 0;
int calcFreqFlag = 0;
extern int systickFlag;


int main(void){
	_init_();	// Sets up classes and other variables
	uint32_t voltDeadline = 600;
	uint32_t freqDeadline = 500;
	uint32_t displayDeadline = 300;
	uint32_t systick_counter = 0;
	while(True){
		while(!systickFlag){} // Wait for SysTick
		static enum {START, UPPER, LOWER, SUCCESS} dispState = START;
		systick_counter = systick_counter > systick_counterMax ? 0 : systick_counter + 1;

		readVoltage(&voltageMeasurements, &voltage);

		if(calcVoltFlag && (((calcFreqFlag && ((voltDeadline - systick_counter) <= (freqDeadline - systick_counter))) || ((voltDeadline - systick_counter) <= (displayDeadline - systick_counter))))){
			calcVoltage(Display, &voltageMeasurements, &voltage); // Calculate Voltage & Update Message
			voltDeadline = (voltDeadline + vDeadline) > systick_counterMax ? (voltDeadline + vDeadline) - systick_counterMax : voltDeadline + vDeadline; // Handles Clock Overflow
			calcVoltFlag = 0;
		}
		else if(calcFreqFlag && ((freqDeadline - systick_counter) <= (displayDeadline - systick_counter))){
			calcFrequency(Display, &freqCounts, &timeElapsed);
			freqDeadline = (freqDeadline + fDeadline) > systick_counterMax ? (freqDeadline + fDeadline) - systick_counterMax : freqDeadline + fDeadline; // Handles Clock Overflow
			calcFreqFlag = 0;
		}
		else // display
		{
			if(Display->lcd_Nack() || dispState == SUCCESS){
				dispState = START;
			}
			else{dispState++;}
			switch (dispState)
			{
			case 0:
				Display->startTalking();
				break;
			case 1:
				Display->sendMSGBits(Display, 0); // First Portion of Message
				break;
			case 2:
				Display->sendMSGBits(Display, 1); // Second Portion of Message})
				break;
			case 3:
				switch (Display->cursorPos[0])
				{
				case 0:
					Display->cursorPos[0] = Display->cursorPos[1] > lineSize ? 1 : 0;
					break;
				case 1:
					Display->cursorPos[0] = Display->cursorPos[1] > lineSize ? 0 : 1;
				}
				calcVoltFlag = (Display->cursorPos[0] == 1) && (Display->cursorPos[1] == lineSize) ? 1 : calcVoltFlag;	
				Display->cursorPos[1] = Display->cursorPos[1] > lineSize ? 0 : Display->cursorPos[1] + 1;
				break;
			default:
				while(1); // Error Catching
			}
		}
		systickFlag = 0;
	}
}
