/*
 * Timekeeper.h
 *
 *  Created on: Oct 12, 2025
 *      Author: Dan
 */

#ifndef INC_TIMEKEEPER_H_
#define INC_TIMEKEEPER_H_

#include <stdint.h>
#include "stm32l476xx.h"	// CMSIS core (DWT cycle counter)

// CPE 490 runs on the 4 MHz MSI clock (see clockSpeedHz in timer_lib.h), so 1 us = 4 cycles
#ifndef CLK_FREQ
#define CLK_FREQ 4000000U
#endif

/* ============================================================================
 * Timekeeper
 *
 * Measures how long each "task" (each RTOS task) takes to execute, and keeps
 * the longest (worst-case) runtime seen for each one. Timing uses the
 * Cortex-M4 DWT cycle counter, so it doesn't tie up a hardware timer.
 *
 * Each task can also have a time budget (limit_us). Timekeeper_Stop()
 * counts an overrun whenever a run takes longer than that budget. This is
 * tracking only: nothing interrupts, stops, or cuts off a task.
 * ============================================================================
 */

// cycles-per-microsecond at the system clock speed, used to convert DWT cycle counts to microseconds
#define TIMEKEEPER_CYCLES_PER_US (CLK_FREQ / 1000000U)

// max number of tasks Timekeeper can track - bump this if more tasks get added later
#define TIMEKEEPER_NUM_TASKS 10

// Identifies each task Timekeeper measures. These line up with the RTOS priorities
// registered in CPE 490's main.c (task ID = RTOS priority).
typedef enum {
	TASK_PRIORITY_0 = 0,	// read voltage (ADC sample)			(every 1 ms)
	TASK_PRIORITY_1,		// calc frequency							(deadline 15 ms)
	TASK_PRIORITY_2,		// handle new RPM (PID + PWM)				(deadline 25 ms)
	TASK_PRIORITY_3,		// calc voltage							(deadline 400 ms)
	TASK_PRIORITY_4,		// HMI: display update						(deadline 4 ms per step)
	TASK_PRIORITY_5,		// HMI: handle SW1 press					(deadline 10 ms)
	TASK_PRIORITY_6,		// HMI: handle SW2 press					(deadline 10 ms)
	TASK_PRIORITY_7,		// HMI: read numpad						(deadline 25 ms)
	TASK_PRIORITY_8,		// HMI: update main menu					(deadline 50 ms)
	TASK_PRIORITY_9,		// HMI: handle numpad press				(deadline 100 ms)
	TIMEKEEPER_NUM_DEFINED_TASKS	// keep last - number of tasks defined above, also used as a "none" sentinel
} Timekeeper_Task_ID;

typedef struct {
	uint32_t start_cycles;		// DWT->CYCCNT value latched by Timekeeper_Start()
	uint32_t last_runtime_us;	// runtime of the most recently completed run, in microseconds
	uint32_t max_runtime_us;	// longest runtime recorded so far, in microseconds
	uint32_t limit_us;			// time budget for this task, in microseconds. 0 = no limit set
	uint32_t period_us;			// how often this task is scheduled to run, in microseconds. 0 = not periodic This is set by the scheduler when the task is started, and used to track min/max period seen so far
	uint32_t min_period_us;		// shortest period seen so far, in microseconds.
	uint32_t max_period_us;		// longest period seen so far, in microseconds.
	uint32_t jitter_range_us;		// max_period_us - min_period_us, in microseconds. 0 = not enough data yet
	uint32_t period_samples;		// number of periods measured (runs after the first one) since Init/Reset
	uint32_t overrun_count;	// number of times this task has run longer than limit_us
	uint8_t running;			// 1 between a Start() and its matching Stop() - basic misuse guard
	uint8_t has_run;			// 1 once the task has started at least once since Init/Reset, so start_cycles holds a real start time
} Timekeeper_Task_t;

extern Timekeeper_Task_t timekeeper_tasks[TIMEKEEPER_NUM_TASKS];

// Call once during startup (starts the DWT cycle counter). RTOS_Init() calls this.
void Timekeeper_Init(void);

// Call right before / right after a task's code to time it
void Timekeeper_Start(Timekeeper_Task_ID task_id);
void Timekeeper_Stop(Timekeeper_Task_ID task_id);

// Configuration
void Timekeeper_Set_Limit_us(Timekeeper_Task_ID task_id, uint32_t limit_us);

// Reading back results
uint32_t Timekeeper_Get_Max_Runtime_us(Timekeeper_Task_ID task_id);
uint32_t Timekeeper_Get_Last_Runtime_us(Timekeeper_Task_ID task_id);
uint32_t Timekeeper_Get_Overrun_Count(Timekeeper_Task_ID task_id);
uint32_t Timekeeper_Get_Period_us(Timekeeper_Task_ID task_id);
uint32_t Timekeeper_Get_Min_Period_us(Timekeeper_Task_ID task_id);
uint32_t Timekeeper_Get_Max_Period_us(Timekeeper_Task_ID task_id);

// Clearing recorded stats (limit_us is left alone)
void Timekeeper_Reset_Task(Timekeeper_Task_ID task_id);
void Timekeeper_Reset_All(void);

#endif /* INC_TIMEKEEPER_H_ */
