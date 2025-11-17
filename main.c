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
#include "globals.h"


//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

void createFreqString(unsigned char msg[GenevaLCDColSize], double freq){
	snprintf((char*)msg, 40, "FREQ: %8.2fHz", freq);  // 2 decimal places
	msg[39] = 0x00; // Null Terminator
}

void createVoltString(unsigned char msg[GenevaLCDColSize], double volt){
	snprintf((char*)msg, 40, "VOLTAGE: %5.2fV", volt);  // 2 decimal places
	msg[39] = 0x00; // Null Terminator
}

void readVoltage(){

	// Read Voltage
	voltageAccum += ((ADC1->DR) * (10.0/3.0))/ 255.0;
	voltageMeasurements += 1;

	// Start New Conversion
	ADC1->ISR |= ADC_ISR_EOC; // Clear End of Conversion Flag
	ADC1->CR |= ADC_CR_ADSTART; // Start ADC Conversion

}

void calcVoltage(){
	createVoltString(&(Display->wholeMSG[0][0]), voltageAccum/(voltageMeasurements)); // Update Voltage String
	voltageAccum = 0;
	voltageMeasurements = 0;
	calcVoltFlag = 1;
}

void calcFrequency(){
	createFreqString(&(Display->wholeMSG[1][0]), freqCounts / timeElapsed); // Update Frequency String
	freqCounts = 0;
	timeElapsed = 0.0;
	calcFreqFlag = 0;
}

void displayUpdate(){

	if(Display->lcd_Nack() || displayState == SUCCESS){
		displayState = START;
	}
	else{(displayState)++;}
	switch (displayState)
	{
	case 0:
		Display->startTalking();
		break;
	case 1:
		Display->sendMSGBits(Display, (Display->wholeMSG[Display->cursorPos[0]][Display->cursorPos[1]] == 0x00) ? 0: 1); // First Portion of Message
		break;
	case 2:
		if (Display->wholeMSG[Display->cursorPos[0]][Display->cursorPos[1]] == 0x00){
			switch (Display->cursorPos[0])
			{
			case 0:
				Display->sendBits(192); // Go to 2nd Line
				break;
			case 1:
				Display->sendBits(128); // Go to 1st Line
				break;
			}
		}
		else{
			Display->sendMSGBits(Display, 2); // Data Portion of Message
		}
		break;
	case 3:
		if (Display->wholeMSG[Display->cursorPos[0]][Display->cursorPos[1]] == 0x00) {
				Display->cursorPos[1] = 0;
				Display->cursorPos[0] = (Display->cursorPos[0] + 1) % GenevaLCDRowSize;
		}
		else{
			Display->cursorPos[1]++;
		}
		
		break;
	default:
		while(1); // Error Catching
	}
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------


extern int systickFlag;

int main(void){
	_init_();	// Sets up classes and other variables

	// Set up Scheduler Tasks
	schedulerTasks = (EDFToDo){
		.tasks = { calcVoltage, calcFrequency, displayUpdate },
		.deadlines  = { VOLTAGE_DEADLINE, FREQ_DEADLINE, DISPLAY_DEADLINE },
		.cooldowns  = { 0, 0, 0 },
		.clksWaited = { 0, 0, 0 },
		.taskFlag = {&calcVoltFlag, &calcFreqFlag, NULL}
	};
	// End Set up Scheduler Tasks

	while(TRUE){ 
		while(!systickFlag){} // Wait for SysTick

		readVoltage(); // always read voltage every systick (should a few us)

		uint32_t taskToRun = 0;
		for (uint32_t task = 1; task < MAX_TASKS; task++){
			// Checks to see if the task to run has a cooldown
			if(schedulerTasks.cooldowns[taskToRun] != 0) {
				schedulerTasks.cooldowns[taskToRun]--;
				taskToRun = task;
				continue;
			}
			// Checks to see if task has a cooldown
			else if (schedulerTasks.cooldowns[task] != 0){
				schedulerTasks.cooldowns[task]--;
				continue;
			}
			// Checks to see if the task to run's Flag is not set
			else if ((*schedulerTasks.taskFlag[taskToRun] != TRUE) && (schedulerTasks.taskFlag[taskToRun] != NULL)){
				schedulerTasks.clksWaited[taskToRun]++;
				taskToRun = task;
				continue;
			}
			// Checks to see if the task's Flag is not set
			else if ((*schedulerTasks.taskFlag[task] != TRUE) && (schedulerTasks.taskFlag[taskToRun] != NULL)){
				schedulerTasks.clksWaited[task]++;
				continue;
			}
			// Will check to see if the task to run has a deadline further ahead than the current task
			else if(schedulerTasks.deadlines[taskToRun] > schedulerTasks.deadlines[task]){
				schedulerTasks.clksWaited[taskToRun]++;
				taskToRun = task;
				continue;
			}
			// Task to run has a sorter deadline than task
			else{
				schedulerTasks.clksWaited[task]++;
				continue;
			}

		}
		
		schedulerTasks.tasks[taskToRun](); // Run the selected Task
		schedulerTasks.cooldowns[taskToRun] = schedulerTasks.deadlines[taskToRun]; // Set the cooldown
		schedulerTasks.clksWaited[taskToRun] = 0; // Reset clks waited (Could be used for priority in the EDF if need be)
		systickFlag = 0; // Clear the systick Flag
	}
}
