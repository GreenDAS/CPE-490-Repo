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
#define vDeadline 400
#define fDeadline 500
#define dDeadline 600
#define systick_counterMax 1200
#define lineSize 15 // 16 Max however index 15 is the control character

/*
	128 + 64 = 192
*/

//------------------------------------------------------------------------------
// # Type Definitions
//------------------------------------------------------------------------------

typedef enum dispState {START, UPPER, LOWER, SUCCESS} dispState;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

void createFreqString(unsigned char msg[GenevaLCDColSize], double freq){
	snprintf((char*)msg, 40, "FREQ: %04.2fHz", freq);  // 2 decimal places
	msg[39] = 0x00; // Null Terminator
}

void createVoltString(unsigned char msg[GenevaLCDColSize], double volt){
	char str [40];
	sprintf(str, "VOLTAGE: %02.2f V", volt);
	for (int i = 0; i < strlen(str); i++){
		msg[i] = str[i];
	}
	// snprintf((char*)msg, 40, "VOLTAGE: %02.2f V", volt);  // 2 decimal places
	msg[39] = 0x00; // Null Terminator
}

void readVoltage(int* voltageMeasurements, float* voltageAccum){

	// Read Voltage
	*voltageAccum += ((ADC1->DR) * (10/3))/ 255.0;
	*voltageMeasurements += 1;

	// Start New Conversion
	ADC1->ISR |= ADC_ISR_EOC; // Clear End of Conversion Flag
	ADC1->CR |= ADC_CR_ADSTART; // Start ADC Conversion

}

void calcVoltage(GenevaLCDDevice* Disp,int* voltageMeasurements, float* voltageAccum){
	createVoltString(&(Disp->wholeMSG[0][0]), *voltageAccum/(*voltageMeasurements)); // Update Voltage String
	*voltageAccum = 0;
	*voltageMeasurements = 0;
}

void calcFrequency(GenevaLCDDevice* Disp, int* freqCounts, double* timeElapsed){
	createFreqString(&(Disp->wholeMSG[1][0]), *freqCounts / *timeElapsed); // Update Frequency String
	*freqCounts = 0;
	*timeElapsed = 0;
}

void displayUpdate(GenevaLCDDevice* Disp, dispState* state){

	if(Disp->lcd_Nack() || *state == SUCCESS){
		*state = START;
	}
	else{(*state)++;}
	switch (*state)
	{
	case 0:
		Disp->startTalking();
		break;
	case 1:
		Disp->sendMSGBits(Disp, (Disp->wholeMSG[Disp->cursorPos[0]][Disp->cursorPos[1]] == 0x00) ? 0: 1); // First Portion of Message
		break;
	case 2:
		if (Disp->wholeMSG[Disp->cursorPos[0]][Disp->cursorPos[1]] == 0x00){
			switch (Disp->cursorPos[0])
			{
			case 0:
				Disp->sendBits(192); // Go to 2nd Line
				break;
			case 1:
				Disp->sendBits(128); // Go to 1st Line
				break;
			}
		}
		else{
			Disp->sendMSGBits(Disp, 2); // Data Portion of Message
		}
		break;
	case 3:
		if (Disp->wholeMSG[Disp->cursorPos[0]][Disp->cursorPos[1]] == 0x00) {
				Disp->cursorPos[1] = 0;
				Disp->cursorPos[0] = (Disp->cursorPos[0] + 1) % GenevaLCDRowSize;
		}
		else{
			Disp->cursorPos[1]++;
		}
		
		break;
	default:
		while(1); // Error Catching
	}
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

// Global Vars
IODevice VoltReader;
IODevice FreqReader;
GeneralPurposeTimer Timer2;
GenevaLCDDevice *Display;
int voltageMeasurements = 0;
float voltageAccum = 0;
float frequency = 0;
int freqCounts = 0;
extern double timeElapsed;

// FLAGS
int calcVoltFlag = 1; // Set to always be 1 to calculate voltage so long as the deadline is met
int calcFreqFlag = 0;
extern int systickFlag;


int main(void){
	_init_();	// Sets up classes and other variables
	int voltDeadline = vDeadline;
	int freqDeadline = fDeadline;
	int displayDeadline = dDeadline;
	uint32_t systick_counter = 0;

	int diffVDead = 0;
	int diffFDead = 0;
	int diffDDead = 0;
	dispState displayState = SUCCESS;
	/*
	if clock speed is 4MHz, and an instruction takes 1 cycle,
	then 4,000,000 instructions are executed per second.
	So, on average, the overhead of the RTOS is 3*3 + 5 + 50 + 8 + 4 = 76 instructions. This is an estimated value, Not sure if it is accurate.
	Therefore, the percentage of time spent in the RTOS is (76/4,000,000)*100 = 0.000019 seconds or 19 microseconds.
	19 microseconds / 1millisecond = 1.9% of the CPU time is spent in the RTOS.
	*/
	while(True){ 
		while(!systickFlag){} // Wait for SysTick

		systick_counter = (systick_counter + 1) > systick_counterMax ? 0 : systick_counter + 1;
		diffVDead = voltDeadline - systick_counter;
		diffFDead = freqDeadline - systick_counter;
		diffDDead = displayDeadline - systick_counter;

		readVoltage(&voltageMeasurements, &voltageAccum); // always read voltage every systick (should a few us)

		if(!(ADC1->ISR & ADC_ISR_EOC) && (diffVDead == 0)){ // calculate voltage if its deadline is met and the flag is set
			calcVoltage(Display, &voltageMeasurements, &voltageAccum); // Calculate Voltage & Update Message
			voltDeadline = (voltDeadline + vDeadline) > systick_counterMax ? diffVDead + vDeadline : voltDeadline + vDeadline; // Handles Clock Overflow
			calcVoltFlag = 1;
		}
		else if(calcFreqFlag && (diffFDead <= diffDDead)){ // should only calculate frequency if its deadline is the soonest and the flag is set
			calcFrequency(Display, &freqCounts, &timeElapsed);
			freqDeadline = (freqDeadline + fDeadline) > systick_counterMax ? diffFDead + fDeadline : freqDeadline + fDeadline; // Handles Clock Overflow
			calcFreqFlag = 0;
		}
		else // displays if its deadline is the soonest
		{
			displayUpdate(Display, &displayState);
			displayDeadline = (displayDeadline + dDeadline) > systick_counterMax ? diffDDead + dDeadline : displayDeadline + dDeadline; // Handles Clock Overflow
		}
		systickFlag = 0;
	}
}
