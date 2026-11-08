/**************************************************************************
 * Author  Daniel S.
 * Date    11/13/2025
 * File    main.c
 * Version V1.6
 * Completes Lab 9
 * V1.6 (10/27/2026): EDF loop replaced by the O(1) RTOS from senior design (RTOS.c/.h)
 *                    Old EDF loop kept behind USE_OLD_EDF_SCHEDULER (globals.h) for Timekeeper comparisons
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
#include "pwm_lib.h"
#include "pid.h"
#include "RTOS.h"
#include "Timekeeper.h"

//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

void createTargetString(unsigned char msg[GenevaLCDColSize], void *value, int isString, char *units, char *descriptor)
{
	if (isString)
	{
		snprintf((char *)msg, GenevaLCDColSize, "%s%s%s", descriptor, (char *)value, units); // Insert the target String into the %s spot
	}
	else
	{
		snprintf((char *)msg, GenevaLCDColSize, "%s%6.2f%s", descriptor, *(float *)(value), units); // Insert the target String into the %s spot
	}
}

void createFreqString(unsigned char msg[GenevaLCDColSize], double freq)
{
	snprintf((char *)msg, GenevaLCDColSize, "ACTUAL:%6.2fRPM", freq); // 2 decimal places
}

void createVoltString(unsigned char msg[GenevaLCDColSize], double volt)
{
	snprintf((char *)msg, GenevaLCDColSize, "VOLTAGE: %5.2fV", volt); // 2 decimal places
}

// Tasks

static inline void readVoltage()
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
	voltageAccum = 0;
	voltageMeasurements = 0;
	calcVoltFlag = 1;
	updateMainMenuFlag = 1; // Set flag to update main menu
}

void calcFrequency()
{
	frequency = (freqCounts / timeElapsed) / (20*7); // 140 is from gear ratio and pulses/rotation
	newRPMFlag = 1;
	freqCounts = 0;
	timeElapsed = 0.0;
	calcFreqFlag = 0;
	updateMainMenuFlag = 1; // Set flag to update main menu
}

void handleSW1Press()
{
	if (targetSetFlag)
	{
		tor_rpm_toggle = 0; // Reset to RPM display if a new target RPM was set
	}
	else
	{
		tor_rpm_toggle ^= 1; // Toggle the torque/RPM display flag
	}
	SW1LED->setState(SW1LED, 0); // Dissable LED1
	sw1PressedFlag = 0;
	updateMainMenuFlag = 1; // Set flag to update main menu
}

void updateMainMenu()
{
	float value = targetRPM; // Keep This - Green
	char unitSTR[4] = "RPM";
	char descriptorSTR[8] = "TARGET:";
	if (tor_rpm_toggle)
	{

		torque = 17.29 * (voltage / (22.0 / 3.0));
		snprintf(unitSTR, sizeof(unitSTR), "mNm");
		snprintf(descriptorSTR, sizeof(descriptorSTR), "TORQUE:");
		// also V/7.3 is apparently current
		value = torque;
	}

	createTargetString(&(Display->wholeMSG[0][0]), &value, FALSE, unitSTR, descriptorSTR); // Top Row
	createFreqString(&(Display->wholeMSG[1][0]), (frequency * (60.0)));			   // Bot Row
	updateMainMenuFlag = 0;
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

void readPad()
{
	readFinishedFlag = NumberPad->stateMachineReadPad(NumberPad);
}

void handlePadPress()
{
	static unsigned char targetString[7] = {'_', '_', '_', '.', '_', '_', 0x00};
	static unsigned int cursorAt = 0;
	float tempTargetRPM = 0.0;

	if (sw2PressedFlag && !gettingUserInputFlag)
	{
		snprintf(&(((char *)targetString)[0]), 7, "___.__");
		return;
	}

	// Detect button release
	switch (NumberPad->recentPress)
	{
	// Button Press was a backspace (*)
	case 10:
		if (targetString[cursorAt] == '_')
		{
			cursorAt = (cursorAt == 0) ? 0 : cursorAt - 1;
			if (cursorAt == 3)
			{
				cursorAt = 2;
			}
			targetString[cursorAt] = '_';
		}
		else
		{
			targetString[cursorAt] = '_';
		}
		break;

	// Button Press was enter (#)
	case 11:
		if (targetString[0] != '_')
		{
			for (cursorAt = 0; targetString[cursorAt] != 0x00; cursorAt++)
			{
				if (cursorAt == 3)
				{
					cursorAt++;
				} // Skip the decimal
				if (targetString[cursorAt] == '_')
				{
					continue;
				} // Skip location if it is an _
				float tempValue = targetString[cursorAt] - '0';
				float numbersPlace;
				if (cursorAt > 3)
				{ // add the decimal value to target RPM
					numbersPlace = 10;
					for (uint32_t i = 0; i < (cursorAt - 4); i++)
					{
						numbersPlace *= 10;
					}										   // Find the correct decimal place
					tempTargetRPM += tempValue / numbersPlace; // Add the decimal place value to targetRPM
				}
				else
				{
					numbersPlace = 1;
					for (uint32_t i = 0; i < (2 - cursorAt); i++)
					{
						numbersPlace *= 10;
					} // Find the correct numbers place
					tempTargetRPM += tempValue * numbersPlace;
				}
			}
			targetRPM = (tempTargetRPM < RPM_LOWER) ? RPM_LOWER : tempTargetRPM; // Lower Limit
			targetRPM = (targetRPM > RPM_UPPER) ? RPM_UPPER : targetRPM;		 // Upper Limit
		}
		cursorAt = 0;
		targetSetFlag = 1;
		sw1PressedFlag = 1; // Set SW1 to reset display
		gettingUserInputFlag = 0;
		snprintf(&(((char *)targetString)[0]), 7, "___.__");
		break;

	default:
		targetString[cursorAt] = NumberPad->recentPress + '0';
		cursorAt++;
		if (cursorAt == 3)
		{
			cursorAt = 4;
		}
		else if ((cursorAt >= 6))
		{
			cursorAt = 5;
		}

		break;
	}
	createTargetString(&(Display->wholeMSG[1][0]), &(targetString[0]), 1, "RPM", "TARGET:"); // Update Bot Row's Message
	readFinishedFlag = 0;
}

void handleSW2Press()
{
	if (gettingUserInputFlag)
	{
		snprintf((char *)(&(Display->wholeMSG[0][0])), GenevaLCDColSize, "ENTER TARGET RPM"); // Update Top Row's Message
		snprintf((char *)(&(Display->wholeMSG[1][0])), GenevaLCDColSize, "TARGET:___.__RPM"); // Update Bot Row's Message
		SW2LED->setState(SW2LED, 0);														  // Dissable LED2
		sw2PressedFlag = 0;
	}
	else
	{
		handlePadPress();			 // Handle the Pad Press to reset targetString
		SW2LED->setState(SW2LED, 0); // Dissable LED2
		sw1PressedFlag = 1;			 // Set SW1 to reset display
		tor_rpm_toggle = 0;			 // Reset to RPM display if a new target RPM was set
		sw2PressedFlag = 0;			 // Reset SW2 Pressed Flag
	}
	updateMainMenuFlag = 0; // Reset Main Menu Update Flag
}

void handleNewRPM()
{
	if(targetSetFlag){
		MotorPID->SetPointChanged(MotorPID, targetRPM, 1/frequency);
		MotorPWM->updateDutyCycle(MotorPWM, MotorPID->pidOut / 100.0); // Convert from % to decimal
		targetSetFlag = 0;
	}
	else{
		// Update PWM based on new RPM
		MotorPID->CurrentPosChanged(MotorPID, frequency * 60.0, 1/frequency); // Convert to RPM
		MotorPWM->updateDutyCycle(MotorPWM, MotorPID->pidOut / 100.0); // Convert from % to decimal
		newRPMFlag = 0;
	}

}


// Ready Fns

int voltCalcReady() { return (tor_rpm_toggle) ? 1 : 0; }																			   // Onlt calc voltage when torque is being displayed
int freqCalcReady() { return calcFreqFlag; }																						   // Only calc frequency when flag is set
int handleSW1PressReady() { return sw1PressedFlag; }																				   // Only handle SW1 press when SW1 is pressed
int updateMainMenuReady() { return (updateMainMenuFlag && !gettingUserInputFlag); }													   // Only update main menu when flag is set and not getting user input
int dispUpdaReady() { return 1; }																									   // Always ready to update display
int readPadReady() { return gettingUserInputFlag; }																					   // Only read pad when getting user input
int handlePadPressReady() { return (readFinishedFlag && ((!(NumberPad->state)) && NumberPad->prevState)) ? gettingUserInputFlag : 0; } // Only handle pad press when read is finished and getting user input
int handleSW2PressReady() { return sw2PressedFlag; }																				   // Only handle SW2 press when SW2 is pressed
int handleNewRPMReady() { return newRPMFlag; }																	   								// Only handle new RPM when newRPMFlag is set and not targetSetFlag

// Cooldown Fns

int voltCooldown() { return VOLTAGE_DEADLINE; } // Voltage deadline
int freqCooldown() { return FREQ_DEADLINE; }	// Frequency deadline
int handleSW1PressCooldown() { return 0; }		// Run as fast as possible after SW1 is pressed
int updateMainMenuCooldown() { return 0; }		// Run as fast as possible after main menu update
int dispCooldown() { return 0; }				// Run as fast as possible after display update
int readPadCooldown() { return 5; }				// 5ms cooldown for reading pad for debouncing and capacitance
int handlePadPressCooldown() { return 0; }		// Run as fast as possible after read is finished
int handleSW2PressCooldown() { return 0; }		// Run as fast as possible after SW2 is pressed
int handleNewRPMCooldown() { return (int)((30/(targetRPM * 7)) * 1000); }		// Run as fast as possible after new RPM is measured
// Colleen was here :)

//------------------------------------------------------------------------------
// Scheduler Glue
//------------------------------------------------------------------------------

/* USE_OLD_EDF_SCHEDULER (globals.h) picks which scheduler runs the tasks:
 *   0 = new O(1) RTOS (RTOS.c/.h)
 *   1 = old EDF loop (V1.5), kept here so the two can be compared
 * Either way, Timekeeper times each task's actual run (the task fn itself, not the ready/cooldown checks),
 * and a task lands in the same timekeeper_tasks[] slot under both schedulers (slot = RTOS priority).
 * So: run one, note timekeeper_tasks[] in the debugger, flip the switch, run the other, compare.
 *   max_runtime_us					- longest single run of the task
 *   min_period_us / max_period_us	- shortest / longest time between the starts of two runs
 *   jitter_range_us				- max_period_us - min_period_us
 */

// Runs one task with Timekeeper wrapped around it
static void TimedRun(void (*task)(void), Timekeeper_Task_ID id)
{
	Timekeeper_Start(id);
	task();
	Timekeeper_Stop(id);
}

#if !USE_OLD_EDF_SCHEDULER

/* The RTOS releases tasks on a fixed period. These tasks are event driven
 * instead (a ready fn says when they can run, a cooldown fn says how long to
 * wait after a run), so each one is registered with a 1 tick (1 ms) period and
 * run through RunGatedTask(), which checks those the same way the old EDF loop did.
 * The old EDF deadlines never changed at runtime, so EDF picked tasks in a fixed
 * order: shortest deadline first. That order is kept within each group in main(), but the
 * control tasks (measurements + PID) all sit above the Human Machine Interface tasks (0 = highest).
 */
typedef struct {
	void (*task)(void);			// The task itself
	int  (*taskCond)(void);		// Ready fn - task only runs when this returns non-zero
	int  (*coolDownFn)(void);	// Cooldown fn - ms to wait after a run before the task can run again
	uint32_t readyAt;			// RTOS tick (ms) the cooldown ends on - the task can't run before this tick
	Timekeeper_Task_ID tkID;	// Timekeeper slot (= RTOS priority)
} GatedTask;

static GatedTask freqCalcTask		= {calcFrequency,	freqCalcReady,			freqCooldown,			0,	TASK_PRIORITY_1};
static GatedTask newRPMTask			= {handleNewRPM,	handleNewRPMReady,		handleNewRPMCooldown,	0,	TASK_PRIORITY_2};
static GatedTask voltCalcTask		= {calcVoltage,		voltCalcReady,			voltCooldown,			0,	TASK_PRIORITY_3};
static GatedTask sw1PressTask		= {handleSW1Press,	handleSW1PressReady,	handleSW1PressCooldown,	0,	TASK_PRIORITY_4};
static GatedTask sw2PressTask		= {handleSW2Press,	handleSW2PressReady,	handleSW2PressCooldown,	0,	TASK_PRIORITY_5};
static GatedTask readPadTask		= {readPad,			readPadReady,			readPadCooldown,		0,	TASK_PRIORITY_6};
static GatedTask mainMenuTask		= {updateMainMenu,	updateMainMenuReady,	updateMainMenuCooldown,	0,	TASK_PRIORITY_7};
static GatedTask padPressTask		= {handlePadPress,	handlePadPressReady,	handlePadPressCooldown,	0,	TASK_PRIORITY_8};
static GatedTask dispUpdaTask		= {displayUpdate,	dispUpdaReady,			dispCooldown,			0,	TASK_PRIORITY_9};

// Called by the RTOS once per tick for each gated task
// The cooldown is timed against the RTOS tick count (1 tick = 1 ms), not counted down per call, so it stays
// in real ms even when a long task makes this wrapper miss some ticks
void RunGatedTask(void *context)
{
	GatedTask *self = (GatedTask *)context;
	if ((int32_t)(RTOS_Get_Tick_Count() - self->readyAt) < 0)
	{
		return; // Still cooling down, try again next tick (signed difference keeps this right when the tick count wraps)
	}
	if (!self->taskCond())
	{
		return; // Not ready yet
	}
	TimedRun(self->task, self->tkID);									// Run the Task
	self->readyAt = RTOS_Get_Tick_Count() + (uint32_t)self->coolDownFn();	// Set the cooldown
}

// readVoltage() used to run every tick before the EDF picked a task, so it gets the top priority
void ReadVoltageTask(void *context)
{
	(void)context;
	TimedRun(readVoltage, TASK_PRIORITY_0);
}

#else

/* Old EDF scheduler task table (from V1.5) */
#define MAX_TASKS 9 // Maximum number of tasks for EDF Scheduler
typedef struct {
	void (*tasks[MAX_TASKS])(void);         // Task function pointers
	int  deadlines[MAX_TASKS];              // Deadlines for each task
	int  cooldowns[MAX_TASKS];              // Cooldown counters for each task
	int  clksWaited[MAX_TASKS];             // Clocks waited for each task
	int  (*taskCond[MAX_TASKS])(void);      // Condition functions to check if task is ready
	int  (*coolDownFn[MAX_TASKS])(void);    // Cooldown functions for each task
	Timekeeper_Task_ID tkID[MAX_TASKS];     // Timekeeper slot for each task (same slot the task uses under the RTOS)
} EDFToDo;

EDFToDo schedulerTasks; // EDF Scheduler Tasks

#endif

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main(void)
{
	_init_(); // Sets up classes and other variables

#if !USE_OLD_EDF_SCHEDULER

	// Set up the RTOS (also starts Timekeeper's DWT cycle counter)
	// SysTick is already ticking from _init_(), but RTOS_Tick() won't release anything until RTOS_Start()
	RTOS_Init();

	// Priority: control path first (measurements + PID), Human Machine Interface (buttons, numpad, LCD) last.
	// Within each group the order follows the old EDF deadlines (shortest first).
	//			  Prio	Function			Context			Period	Offset	Name
	// Control
	RTOS_Add_Task(0,	ReadVoltageTask,	NULL,			1,		0,		"Read Voltage");	// Every tick
	RTOS_Add_Task(1,	RunGatedTask,		&freqCalcTask,	1,		0,		"Calc Freq");		// FREQ_DEADLINE				15ms
	RTOS_Add_Task(2,	RunGatedTask,		&newRPMTask,	1,		0,		"New RPM");			// NEW_RPM_DEADLINE				25ms (PID)
	RTOS_Add_Task(3,	RunGatedTask,		&voltCalcTask,	1,		0,		"Calc Volt");		// VOLTAGE_DEADLINE				400ms
	// Human Machine Interface
	RTOS_Add_Task(4,	RunGatedTask,		&sw1PressTask,	1,		0,		"SW1 Press");		// HANDLE_SW1_PRESS_DEADLINE	10ms
	RTOS_Add_Task(5,	RunGatedTask,		&sw2PressTask,	1,		0,		"SW2 Press");		// HANDLE_SW2_PRESS_DEADLINE	10ms
	RTOS_Add_Task(6,	RunGatedTask,		&readPadTask,	1,		0,		"Read Pad");		// READ_NUMPAD_DEADLINE			25ms
	RTOS_Add_Task(7,	RunGatedTask,		&mainMenuTask,	1,		0,		"Main Menu");		// MAIN_MENU_UPDATE_DEADLINE	50ms
	RTOS_Add_Task(8,	RunGatedTask,		&padPressTask,	1,		0,		"Pad Press");		// HANDLE_NUMPAD_PRESS_DEADLINE	100ms
	RTOS_Add_Task(9,	RunGatedTask,		&dispUpdaTask,	1,		0,		"Display");			// DISPLAY_DEADLINE				600ms

	// The RTOS's own timekeeping would time RunGatedTask() every tick, including the ticks where the
	// task isn't ready and nothing runs. Turn it off so only real runs are timed (by TimedRun()),
	// the same as the EDF build. Overruns are only counted, nothing gets cut off.
	RTOS_Set_Timekeeping(FALSE);
	// Timekeeper_Set_Limit_us(TASK_PRIORITY_2, 500); // Example: count an overrun whenever New RPM takes longer than 500us

	// Hand control to the RTOS - this never returns
	RTOS_Start();

#else

	Timekeeper_Init(); // Starts the DWT cycle counter Timekeeper times tasks with

	// Set up Scheduler Tasks
	schedulerTasks = (EDFToDo){
		.tasks = {calcVoltage, calcFrequency, displayUpdate, 
						handleSW1Press, readPad, handlePadPress, 
						handleSW2Press, updateMainMenu, handleNewRPM},
		.deadlines = {VOLTAGE_DEADLINE, FREQ_DEADLINE, DISPLAY_DEADLINE, 
						HANDLE_SW1_PRESS_DEADLINE, READ_NUMPAD_DEADLINE, HANDLE_NUMPAD_PRESS_DEADLINE, 
						HANDLE_SW2_PRESS_DEADLINE, MAIN_MENU_UPDATE_DEADLINE, NEW_RPM_DEADLINE},
		.cooldowns = {0, 0, 0, 0, 0, 0, 0, 0, 0},
		.clksWaited = {0, 0, 0, 0, 0, 0, 0, 0, 0},
		.taskCond = {voltCalcReady, freqCalcReady, dispUpdaReady, 
						handleSW1PressReady, readPadReady, handlePadPressReady, 
						handleSW2PressReady, updateMainMenuReady, handleNewRPMReady},
		.coolDownFn = {voltCooldown, freqCooldown, dispCooldown, 
						handleSW1PressCooldown, readPadCooldown, handlePadPressCooldown, 
						handleSW2PressCooldown, updateMainMenuCooldown, handleNewRPMCooldown},
		.tkID = {TASK_PRIORITY_3, TASK_PRIORITY_1, TASK_PRIORITY_9, 
						TASK_PRIORITY_4, TASK_PRIORITY_6, TASK_PRIORITY_8, 
						TASK_PRIORITY_5, TASK_PRIORITY_7, TASK_PRIORITY_2},
	};
	// End Set up Scheduler Tasks
	while (TRUE)
	{
		while (!systickFlag)
		{
		} // Wait for SysTick

		TimedRun(readVoltage, TASK_PRIORITY_0);

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
			TimedRun(schedulerTasks.tasks[taskToRun], schedulerTasks.tkID[taskToRun]);	  // Run the selected Task (timed)
			schedulerTasks.cooldowns[taskToRun] = schedulerTasks.coolDownFn[taskToRun](); // Set the cooldown
			schedulerTasks.clksWaited[taskToRun] = 0;									  // Reset clks waited (Could be used for priority in the EDF if need be)
		}

		systickFlag = 0; // Clear the systick Flag
	}

#endif
}
