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

void createTargetString(unsigned char msg[GenevaLCDColSize], void *value, int isString){
	if (isString){
		snprintf((char*)msg, 40, "Target: %s", (char*)value); // Insert the target String into the %s spot
	}
	else{
		snprintf((char*)msg, 40, "Target: %7.2f", *(float*)(value)); // Insert the target String into the %s spot
	}
	msg[39] = 0x00; // Null Terminator it just in case
}

void createFreqString(unsigned char msg[GenevaLCDColSize], double freq){
	snprintf((char*)msg, 40, "FREQ: %8.2fHz", freq);  // 2 decimal places
	msg[39] = 0x00; // Null Terminator
}

void createVoltString(unsigned char msg[GenevaLCDColSize], double volt){
	snprintf((char*)msg, 40, "VOLTAGE: %5.2fV", volt);  // 2 decimal places
	msg[39] = 0x00; // Null Terminator
}

// Tasks

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

void readPad(){
	NumberPad->stateMachineReadPad(NumberPad);
}

void handlePadPress(){
	static unsigned char targetString[7] = {'_', '_', '_', '.', '_', '_', NULL};
	static unsigned int cursorAt = 0;
	// Detect button release
	switch (NumberPad->recentPress){
		// Button Press was a backspace (*)
		case 10:
			cursorAt--;
			targetString[cursorAt] = '_';
		break;

		// Button Press was enter (#)
		case 11:
		float tempTargetRPM = 0;
			if (targetString[0] != '_'){
				for (cursorAt = 0; cursorAt < 7; cursorAt++){
					if (cursorAt == 3){cursorAt++;} // Skip the decimal
					if (targetString[cursorAt] == '_'){continue;} // Skip location if it is an _
					float tempValue = targetString[cursorAt] - '0';
					float numbersPlace = 10;
					if (cursorAt > 3){ // add the decimal value to target RPM
						for (int i = 0; i < (cursorAt - 4); i++){ numbersPlace *= 10; } // Find the correct decimal place
						tempTargetRPM += tempValue / numbersPlace; // Add the decimal place value to targetRPM
					}
					else{
						for (int i = 0; i < (2 - cursorAt) ; i++){ numbersPlace *= 10; } // Find the correct numbers place
						tempTargetRPM += tempValue * numbersPlace;
					}
				}
				targetRPM = (tempTargetRPM > RPM_UPPER) ? RPM_UPPER : tempTargetRPM; // Upper Limit
				targetRPM = (tempTargetRPM < RPM_LOWER) ? RPM_LOWER : tempTargetRPM; // Lower Limit
			}
			targetSetFlag = 1;
			gettingUserInputFlag = 0;
			// Set switch 1's flag to swap back to main menu ********
		break;

		default:
			targetString[cursorAt] = NumberPad->recentPress + '0';
			cursorAt++;
			if(cursorAt == 3){
				cursorAt =  4;
			
			}
			else if ((cursorAt >= 7))
			{
				cursorAt =  6;
			}
			
		break;

	}
	createTargetString(&(Display->wholeMSG[0][0]), &(targetString[0]), 1);
}

// Ready Fns

int voltCalcReady(){return calcVoltFlag;}
int freqCalcReady(){return calcFreqFlag;}
int dispUpdaReady(){return 1;}
int readPadReady(){return gettingUserInputFlag;}
int handlePadPressReady(){return ((!(NumberPad->state)) == NumberPad->prevState) ? gettingUserInputFlag : 0;}

// Cooldown Fns

int voltCoolDown(){return VOLTAGE_DEADLINE;}
int freqCoolDown(){return FREQ_DEADLINE;}
int dispCoolDown(){return 0;}
int readPadCooldown(){return 5;}
int handlePadPressCooldown(){return 0;}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------


int main(void){
	_init_();	// Sets up classes and other variables

	IODevice* LEDS[4] = {
		IODevice_Create('C',LED1,0,1,'O'),
		IODevice_Create('C',LED2,0,1,'O'),
		IODevice_Create('C',LED3,0,1,'O'),
		IODevice_Create('C',LED4,0,1,'O')};

	// Set up Scheduler Tasks
	schedulerTasks = (EDFToDo){
		.tasks = { readPad, handlePadPress, displayUpdate },
		.deadlines  = { READ_NUMPAD_DEADLINE, HANDLE_NUMPAD_PRESS_DEADLINE, DISPLAY_DEADLINE },
		.cooldowns  = { 0, 0, 0 },
		.clksWaited = { 0, 0, 0 },
		.taskCond = { readPadReady, handlePadPressReady, dispUpdaReady },
		.coolDownFn = { readPadCooldown, handlePadPressCooldown, dispCoolDown }
	};
	// End Set up Scheduler Tasks

	while(TRUE){ 
		while(!systickFlag){} // Wait for SysTick

		gettingUserInputFlag = 1;

		uint32_t taskToRun = 0;
		// Picks the Best Task To Run (BTTR)
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
			else if (!schedulerTasks.taskCond[taskToRun]()){
				schedulerTasks.clksWaited[taskToRun]++;
				taskToRun = task;
				continue;
			}
			// Checks to see if the task's Flag is not set
			else if (!schedulerTasks.taskCond[task]()){
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
		// Checks the BTTR to see if it should be ran
		if(schedulerTasks.taskCond[taskToRun]() && (schedulerTasks.cooldowns[taskToRun] == 0)){
			schedulerTasks.tasks[taskToRun](); // Run the selected Task
			schedulerTasks.cooldowns[taskToRun] = schedulerTasks.coolDownFn[taskToRun](); // Set the cooldown
			schedulerTasks.clksWaited[taskToRun] = 0; // Reset clks waited (Could be used for priority in the EDF if need be)
		}

		if(NumberPad->prevState && !NumberPad->state){
			for(int i = 0; i < 4; i++){
				LEDS[i]->setState(LEDS[i],(((NumberPad->recentPress)>>i)&1));
			}
		}

		systickFlag = 0; // Clear the systick Flag
	}
}
