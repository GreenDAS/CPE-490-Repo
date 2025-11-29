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

void createTargetString(unsigned char msg[GenevaLCDColSize], void *value, int isString, char* units){
	if (isString){
		snprintf((char*)msg, GenevaLCDColSize, "Target:%s%s", (char*)value, units); // Insert the target String into the %s spot
	}
	else{
		snprintf((char*)msg, GenevaLCDColSize, "Target:%6.2f%s", *(float*)(value), units); // Insert the target String into the %s spot
	}
}

void createFreqString(unsigned char msg[GenevaLCDColSize], double freq)
{
	snprintf((char *)msg, GenevaLCDColSize, "FREQ: %8.2fHz", freq); // 2 decimal places
}

void createVoltString(unsigned char msg[GenevaLCDColSize], double volt)
{
	snprintf((char *)msg, GenevaLCDColSize, "VOLTAGE: %5.2fV", volt); // 2 decimal places
}

// this was created by caleb
void createTorRPMString(unsigned char msg[GenevaLCDColSize], double tor_rpm)
{
	switch (tor_rpm_toggle)
	{
	case 0:
		switch (togglestate)
		{
		case 0:
			snprintf((char *)msg, GenevaLCDColSize, "TARGET: %4.2f", tor_rpm); // 2 decimal places
			togglestate = 1;
			break;
		case 1:
			snprintf((char *)msg, GenevaLCDColSize, "ACTUAL: %4.2f", tor_rpm); // 2 decimal places
			togglestate = 0;
			break;
		}
		break;

	case 1:
		switch (togglestate)
		{
		case 0:
			snprintf((char *)msg, GenevaLCDColSize, "TARGET: %4.2f", tor_rpm); // 2 decimal places
			togglestate = 1;
			break;
		case 1:
			snprintf((char *)msg, GenevaLCDColSize, "TORQUE: %4.2f", tor_rpm); // 2 decimal places
			togglestate = 0;
			break;
		}
		break;
	}
}

// Tasks

void readVoltage()
{

	// Read Voltage
	voltageAccum += ((ADC1->DR) * (10.0 / 3.0)) / 255.0;
	voltageMeasurements += 1;

	// Start New Conversion
	ADC1->ISR |= ADC_ISR_EOC;	// Clear End of Conversion Flag
	ADC1->CR |= ADC_CR_ADSTART; // Start ADC Conversion
}

void calcVoltage()
{
	createVoltString(&(Display->wholeMSG[0][0]), voltageAccum / (voltageMeasurements)); // Update Voltage String
	voltageAccum = 0;
	voltageMeasurements = 0;
	calcVoltFlag = 1;
}

void calcFrequency()
{
	createFreqString(&(Display->wholeMSG[1][0]), freqCounts / timeElapsed); // Update Frequency String
	freqCounts = 0;
	timeElapsed = 0.0;
	calcFreqFlag = 0;
}

// made by caleb
void togMsg()
{
	double torque;
	double target_rpm;
	double value;
	double actual_rpm;
	char unitSTR[4] = "RPM";
	//^these will prolly go away later but i just need to see what im doing
	switch (tor_rpm_toggle)
	{
	case 0:
		switch (togglestate)
		{
		case 0:
			value = target_rpm;
			break;

		case 1:
			value = actual_rpm;
			break;
		}
		break;
	case 1:
		switch (togglestate)
		{
		case 0:
			value = target_rpm;
			break;

		case 1:
			torque = 17.29 * (volts / (22/3)); // prolly needs fixed/ somehow get volts
			// also V/7.3 is apparently current
			value = torque;
			break;
		}
		break;

		createTargetString(&(Display->wholeMSG[0][0]), value, FALSE, "Nm" );
		calcTorFlag = 0;									// doesnt exist yet
	}
}

void displayUpdate()
{

	if (Display->lcd_Nack() || displayState == SUCCESS)
	{
		displayState = START;
	}
	else
	{
		(displayState)++;
	}
	switch (displayState)
	{
	case 0:
		Display->startTalking();
		break;
	case 1:
		Display->sendMSGBits(Display, (Display->wholeMSG[Display->cursorPos[0]][Display->cursorPos[1]] == 0x00) ? 0 : 1); // First Portion of Message
		break;
	case 2:
		if (Display->wholeMSG[Display->cursorPos[0]][Display->cursorPos[1]] == 0x00)
		{
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
		else
		{
			Display->sendMSGBits(Display, 2); // Data Portion of Message
		}
		break;
	case 3:
		if (Display->wholeMSG[Display->cursorPos[0]][Display->cursorPos[1]] == 0x00)
		{
			Display->cursorPos[1] = 0;
			Display->cursorPos[0] = (Display->cursorPos[0] + 1) % GenevaLCDRowSize;
		}
		else
		{
			Display->cursorPos[1]++;
		}

		break;
	default:
		while (1)
			; // Error Catching
	}
}

void readPad(){
	readFinishedFlag = NumberPad->stateMachineReadPad(NumberPad);
}

void handlePadPress(){
	static unsigned char targetString[7] = {'_', '_', '_', '.', '_', '_', 0x00};
	static unsigned int cursorAt = 0;
	float tempTargetRPM = 0.0;
	// Detect button release
	switch (NumberPad->recentPress){
		// Button Press was a backspace (*)
		case 10:
			if(targetString[cursorAt] == '_'){
				cursorAt = (cursorAt == 0) ? 0 : cursorAt - 1;
				if(cursorAt == 3){
					cursorAt =  2;
				}
				targetString[cursorAt] = '_';
				
			}
			else{
				targetString[cursorAt] = '_';
			}
		break;

		// Button Press was enter (#)
		case 11:
			if (targetString[0] != '_'){
				for (cursorAt = 0; targetString[cursorAt] != 0x00; cursorAt++){
					if (cursorAt == 3){cursorAt++;} // Skip the decimal
					if (targetString[cursorAt] == '_'){continue;} // Skip location if it is an _
					float tempValue = targetString[cursorAt] - '0';
					float numbersPlace;
					if (cursorAt > 3){ // add the decimal value to target RPM
						numbersPlace = 10;
						for (uint32_t i = 0; i < (cursorAt - 4); i++){ numbersPlace *= 10; } // Find the correct decimal place
						tempTargetRPM += tempValue / numbersPlace; // Add the decimal place value to targetRPM
					}
					else{
						numbersPlace = 1;
						for (uint32_t i = 0; i < (2 - cursorAt) ; i++){ numbersPlace *= 10; } // Find the correct numbers place
						tempTargetRPM += tempValue * numbersPlace;
					}
				}
				targetRPM = (tempTargetRPM < RPM_LOWER) ? RPM_LOWER : tempTargetRPM; // Lower Limit
				targetRPM = (targetRPM > RPM_UPPER) ? RPM_UPPER : targetRPM; // Upper Limit
			}
			cursorAt = 0;
			targetSetFlag = 1;
			gettingUserInputFlag = 0;
			snprintf(&(((char*)targetString)[0]), 7, "___.__");
			// Set switch 1's flag to swap back to main menu ********
		break;

		default:
			targetString[cursorAt] = NumberPad->recentPress + '0';
			cursorAt++;
			if(cursorAt == 3){
				cursorAt =  4;
			}
			else if ((cursorAt >= 6))
			{
				cursorAt =  5;
			}
			
		break;

	}
	createTargetString(&(Display->wholeMSG[0][0]), &(targetString[0]), 1);
	readFinishedFlag = 0;
}

// Ready Fns

int voltCalcReady() { return calcVoltFlag; }
int freqCalcReady() { return calcFreqFlag; }
// i know i need to put a ready but dont know how
int dispUpdaReady() { return 1; }
int readPadReady(){return gettingUserInputFlag;}
int handlePadPressReady(){return ( readFinishedFlag && ((!(NumberPad->state)) && NumberPad->prevState)) ? gettingUserInputFlag : 0;}

// Cooldown Fns

int voltCoolDown() { return VOLTAGE_DEADLINE; }
int freqCoolDown() { return FREQ_DEADLINE; }
// i know i need a deadline but dont know how
int dispCoolDown() { return 0; }
int readPadCooldown(){return 5;}
int handlePadPressCooldown(){return 0;}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------


int main(void)
{
	_init_(); // Sets up classes and other variables

	// Set up Scheduler Tasks
	schedulerTasks = (EDFToDo){
		.tasks = {calcVoltage, calcFrequency, displayUpdate},
		.deadlines = {VOLTAGE_DEADLINE, FREQ_DEADLINE, DISPLAY_DEADLINE},
		.cooldowns = {0, 0, 0},
		.clksWaited = {0, 0, 0},
		.taskCond = {voltCalcReady, freqCalcReady, dispUpdaReady},
		.coolDownFn = {voltCoolDown, freqCoolDown, dispCoolDown}};
	// End Set up Scheduler Tasks
	while(TRUE){ 
		while(!systickFlag){} // Wait for SysTick

		gettingUserInputFlag = 1;

		uint32_t taskToRun = 0;
		// Picks the Best Task To Run (BTTR)
		for (uint32_t task = 1; task < MAX_TASKS; task++)
		{
			// Checks to see if the task to run has a cooldown
			if (schedulerTasks.cooldowns[taskToRun] != 0)
			{
				schedulerTasks.cooldowns[taskToRun]--;
				taskToRun = task;
				continue;
			}
			// Checks to see if task has a cooldown
			else if (schedulerTasks.cooldowns[task] != 0)
			{
				schedulerTasks.cooldowns[task]--;
				continue;
			}
			// Checks to see if the task to run's Flag is not set
			else if (!schedulerTasks.taskCond[taskToRun]())
			{
				schedulerTasks.clksWaited[taskToRun]++;
				taskToRun = task;
				continue;
			}
			// Checks to see if the task's Flag is not set
			else if (!schedulerTasks.taskCond[task]())
			{
				schedulerTasks.clksWaited[task]++;
				continue;
			}
			// Will check to see if the task to run has a deadline further ahead than the current task
			else if (schedulerTasks.deadlines[taskToRun] > schedulerTasks.deadlines[task])
			{
				schedulerTasks.clksWaited[taskToRun]++;
				taskToRun = task;
				continue;
			}
			// Task to run has a sorter deadline than task
			else
			{
				schedulerTasks.clksWaited[task]++;
				continue;
			}
		}
		// Checks the BTTR to see if it should be ran
		if (schedulerTasks.taskCond[taskToRun]() && (schedulerTasks.cooldowns[taskToRun] == 0))
		{
			schedulerTasks.tasks[taskToRun]();											  // Run the selected Task
			schedulerTasks.cooldowns[taskToRun] = schedulerTasks.coolDownFn[taskToRun](); // Set the cooldown
			schedulerTasks.clksWaited[taskToRun] = 0;									  // Reset clks waited (Could be used for priority in the EDF if need be)
		}

		systickFlag = 0; // Clear the systick Flag
	}
}
