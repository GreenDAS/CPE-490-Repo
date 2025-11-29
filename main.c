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
	voltage = (voltageAccum / (voltageMeasurements));
	createVoltString(&(Display->wholeMSG[0][0]), voltage); // Update Voltage String
	voltageAccum = 0;
	voltageMeasurements = 0;
	calcVoltFlag = 1;
}

void calcFrequency()
{
	frequency = freqCounts / timeElapsed;
	if(gettingUserInputFlag == 0){
		createFreqString(&(Display->wholeMSG[1][0]), frequency); // Update Frequency String
	}
	newRPMFlag = 1;
	freqCounts = 0;
	timeElapsed = 0.0;
	calcFreqFlag = 0;
}

// made by caleb
void handleSW1Press()
{
	float value; // Keep This - Green
	char unitSTR[4] = "RPM"; // Keep this - Green
	//^these will prolly go away later but i just need to see what im doing
	switch (tor_rpm_toggle)
	{
	case 0:
		value = targetRPM;
		break;

	case 1:
		torque = 17.29 * (voltage / (22/3)); // prolly needs fixed/ somehow get volts
		// also V/7.3 is apparently current
		value = torque;
		break;
	}
	createTargetString(&(Display->wholeMSG[0][0]), &value, FALSE, unitSTR); // Top Row
	SW1LED->setState(SW1LED,0); // Dissable LED1
	sw1PressedFlag = 0;
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

	if (sw2PressedFlag && !gettingUserInputFlag)
	{
		snprintf(&(((char*)targetString)[0]), 7, "___.__");
		return;
	}

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
	createTargetString(&(Display->wholeMSG[1][0]), &(targetString[0]), 1, "RPM");
	readFinishedFlag = 0;
}

void handleSW2Press()
{
	if(gettingUserInputFlag){
		snprintf((char*)(&(Display->wholeMSG[0][0])), GenevaLCDColSize, "Enter Target RPM"); // Update Top Row's Message
		snprintf((char*)(&(Display->wholeMSG[1][0])), GenevaLCDColSize, "Target:___.__RPM"); // Update Bot Row's Message
		SW2LED->setState(SW2LED,0); // Dissable LED2
		sw2PressedFlag = 0;
	}
	else{
		handlePadPress(); // Handle the Pad Press to reset targetString
		SW2LED->setState(SW2LED,0); // Dissable LED2
		sw1PressedFlag = 1; // Set SW1 to reset display
		tor_rpm_toggle = 0; // Reset to RPM display if a new target RPM was set
		sw2PressedFlag = 0; // Reset SW2 Pressed Flag
	}
}

// Ready Fns

int voltCalcReady() { return (tor_rpm_toggle) ? 1 : 0; } // Onlt calc voltage when torque is being displayed
int freqCalcReady() { return calcFreqFlag; } // Only calc frequency when flag is set
int handleSW1PressReady() { return sw1PressedFlag; } // Only handle SW1 press when SW1 is pressed
int dispUpdaReady() { return 1; } // Always ready to update display
int readPadReady(){return gettingUserInputFlag;} // Only read pad when getting user input
int handlePadPressReady(){return ( readFinishedFlag && ((!(NumberPad->state)) && NumberPad->prevState)) ? gettingUserInputFlag : 0;} // Only handle pad press when read is finished and getting user input
int handleSW2PressReady() { return sw2PressedFlag; } // Only handle SW2 press when SW2 is pressed

// Cooldown Fns

int voltCooldown() { return VOLTAGE_DEADLINE; } // Voltage deadline
int freqCooldown() { return FREQ_DEADLINE; } // Frequency deadline
int handleSW1PressCooldown() { return 0; } // Run as fast as possible after SW1 is pressed
int dispCooldown() { return 0; } // Run as fast as possible after display update
int readPadCooldown(){return 5;}  // 5ms cooldown for reading pad for debouncing and capacitance
int handlePadPressCooldown(){return 0;} // Run as fast as possible after read is finished
int handleSW2PressCooldown() { return 0; } // Run as fast as possible after SW2 is pressed

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
			// Checks to see if the task to run's ready fn says it can run
			else if (!schedulerTasks.taskCond[taskToRun]())
			{
				schedulerTasks.clksWaited[taskToRun]++;
				taskToRun = task;
				continue;
			}
			// Checks to see if the task's ready fn says it can run
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
