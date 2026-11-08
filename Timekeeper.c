/*
 * Timekeeper.c
 *
 *	Measures and logs the runtime of each scheduler task, and counts an
 *	overrun whenever a task runs past its configured time budget. Tracking
 *	only - it never interrupts or cuts off a task.
 *
 *  Created on: Oct 12, 2025
 *      Author: Dan
 */

#include "Timekeeper.h"

Timekeeper_Task_t timekeeper_tasks[TIMEKEEPER_NUM_TASKS] = {0};

// Returns the elapsed DWT cycles across at most one 32-bit counter rollover.
static uint32_t Timekeeper_Elapsed_Cycles(uint32_t start_cycles, uint32_t end_cycles) {
	if (end_cycles >= start_cycles) {
		return end_cycles - start_cycles;
	}
	return (UINT32_MAX - start_cycles) + end_cycles + 1U;
}

/**
 * @brief Starts the DWT cycle counter and sets up each task's period tracking. Call once during startup;
 * 		  RTOS_Init() calls this.
 * @param None
 * @retval None
 */
void Timekeeper_Init(void) {
	// --- DWT cycle counter, used for max-runtime measurement ---
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	// --- Period tracking ---
	// CPE 490's tasks are released every tick and then gated by their ready/cooldown functions,
	// so there's no fixed configured period to store: period_us stays 0 ("not periodic").
	// min/max period still record the time seen between real runs (Timekeeper_Reset_All() below clears them).
	for (uint8_t i = 0; i < TIMEKEEPER_NUM_TASKS; i++) {
		timekeeper_tasks[i].period_us = 0;
	}

	Timekeeper_Reset_All();
}

/**
 * @brief Marks the start of a task's run. Call right before the task's code runs
 * @param task_id which task is starting
 * @retval None
 */
void Timekeeper_Start(Timekeeper_Task_ID task_id) {
	if (task_id >= TIMEKEEPER_NUM_TASKS) return;
	uint32_t startOfNewCycleTime = DWT->CYCCNT;	// grab this immediately so the rest of this function doesn't get counted
	Timekeeper_Task_t *task = &timekeeper_tasks[task_id];

	// update min_period_us, max_period_us, and jitter_range_us from the time since this task's last start.
	// The first run after Init/Reset has no previous start to measure from, so it only records its start time.
	if (task->has_run) {
		uint32_t elapsed_period_us = Timekeeper_Elapsed_Cycles(task->start_cycles, startOfNewCycleTime) / TIMEKEEPER_CYCLES_PER_US;
		// first period seen sets both min and max, after that keep the smallest / largest
		if (task->period_samples == 0 || elapsed_period_us < task->min_period_us) {
			task->min_period_us = elapsed_period_us;
		}
		if (task->period_samples == 0 || elapsed_period_us > task->max_period_us) {
			task->max_period_us = elapsed_period_us;
		}
		task->period_samples++;
		// spread between the longest and shortest period seen so far. 0 until at least two periods have been seen
		task->jitter_range_us = task->max_period_us - task->min_period_us;
	}
	task->has_run = 1;

	// record the start time for this run, and mark the task as running
	task->start_cycles = startOfNewCycleTime;

	// mark the task as running so Timekeeper_Stop() knows to record its runtime
	task->running = 1;
}

/**
 * @brief Marks the end of a task's run, records its runtime, updates that task's max runtime if this run was
 * 		  the longest yet, and counts an overrun if it ran past limit_us
 * @param task_id which task just finished
 * @retval None
 */
void Timekeeper_Stop(Timekeeper_Task_ID task_id) {
	uint32_t stop_cycles = DWT->CYCCNT;	// grab this immediately so the rest of this function doesn't get counted

	if (task_id >= TIMEKEEPER_NUM_TASKS) return;

	Timekeeper_Task_t *task = &timekeeper_tasks[task_id];

	if (!task->running) return;	// Stop() called without a matching Start() - ignore

	// wraparound-safe even once DWT->CYCCNT rolls over, as long as the task itself didn't take
	// longer than ~53 seconds (2^32 cycles at 80 MHz) to run
	uint32_t elapsed_cycles = Timekeeper_Elapsed_Cycles(task->start_cycles, stop_cycles);
	uint32_t elapsed_us = elapsed_cycles / TIMEKEEPER_CYCLES_PER_US;

	task->last_runtime_us = elapsed_us;
	if (elapsed_us > task->max_runtime_us) {
		task->max_runtime_us = elapsed_us;
	}

	if (task->limit_us != 0 && elapsed_us > task->limit_us) {
		task->overrun_count++;
	}

	task->running = 0;
}

/**
 * @brief Sets the time budget for a task. Timekeeper_Stop() counts an overrun whenever a run takes longer than this
 * @param task_id which task to configure
 * @param limit_us time budget in microseconds. 0 disables overrun tracking for this task
 * @retval None
 */
void Timekeeper_Set_Limit_us(Timekeeper_Task_ID task_id, uint32_t limit_us) {
	if (task_id >= TIMEKEEPER_NUM_TASKS) return;
	timekeeper_tasks[task_id].limit_us = limit_us;
}

/**
 * @brief Gets the longest runtime recorded for a task so far
 * @param task_id which task to read
 * @retval max runtime in microseconds, or 0 if task_id is out of range
 */
uint32_t Timekeeper_Get_Max_Runtime_us(Timekeeper_Task_ID task_id) {
	if (task_id >= TIMEKEEPER_NUM_TASKS) return 0;
	return timekeeper_tasks[task_id].max_runtime_us;
}

/**
 * @brief Gets the runtime of the most recently completed run of a task
 * @param task_id which task to read
 * @retval last runtime in microseconds, or 0 if task_id is out of range
 */
uint32_t Timekeeper_Get_Last_Runtime_us(Timekeeper_Task_ID task_id) {
	if (task_id >= TIMEKEEPER_NUM_TASKS) return 0;
	return timekeeper_tasks[task_id].last_runtime_us;
}

/**
 * @brief Gets how many times a task has run longer than its configured limit
 * @param task_id which task to read
 * @retval overrun count, or 0 if task_id is out of range
 */
uint32_t Timekeeper_Get_Overrun_Count(Timekeeper_Task_ID task_id) {
	if (task_id >= TIMEKEEPER_NUM_TASKS) return 0;
	return timekeeper_tasks[task_id].overrun_count;
}

/**
 * @brief Gets the period of a task
 * @param task_id which task to read
 * @retval period in microseconds, or 0 if task_id is out of range or if the task is not periodic
 */
uint32_t Timekeeper_Get_Period_us(Timekeeper_Task_ID task_id) {
	if (task_id >= TIMEKEEPER_NUM_TASKS) return 0;
	return timekeeper_tasks[task_id].period_us;
}

/**
 * @brief Gets the shortest period seen for a task so far
 * @param task_id which task to read
 * @retval minimum period in microseconds, or 0 if task_id is out of range
 */
uint32_t Timekeeper_Get_Min_Period_us(Timekeeper_Task_ID task_id) {
	if (task_id >= TIMEKEEPER_NUM_TASKS) return 0;
	return timekeeper_tasks[task_id].min_period_us;
}

/**
 * @brief Gets the longest period seen for a task so far
 * @param task_id which task to read
 * @retval maximum period in microseconds, or 0 if task_id is out of range
 */
uint32_t Timekeeper_Get_Max_Period_us(Timekeeper_Task_ID task_id) {
	if (task_id >= TIMEKEEPER_NUM_TASKS) return 0;
	return timekeeper_tasks[task_id].max_period_us;
}

/**
 * @brief Clears the recorded stats (max/last runtime, overrun count, min/max period, jitter) for one task. Leaves its limit alone
 * @param task_id which task to reset
 * @retval None
 */
void Timekeeper_Reset_Task(Timekeeper_Task_ID task_id) {
	if (task_id >= TIMEKEEPER_NUM_TASKS) return;
	timekeeper_tasks[task_id].max_runtime_us = 0;
	timekeeper_tasks[task_id].last_runtime_us = 0;
	timekeeper_tasks[task_id].overrun_count = 0;
	timekeeper_tasks[task_id].running = 0;
	timekeeper_tasks[task_id].min_period_us = 0;
	timekeeper_tasks[task_id].max_period_us = 0;
	timekeeper_tasks[task_id].jitter_range_us = 0;
	timekeeper_tasks[task_id].period_samples = 0;
	timekeeper_tasks[task_id].has_run = 0;	// next Start() only records a start time, so the period stats don't include the time since the reset
}

/**
 * @brief Clears the recorded stats for every task
 * @param None
 * @retval None
 */
void Timekeeper_Reset_All(void) {
	for (uint8_t i = 0; i < TIMEKEEPER_NUM_TASKS; i++) {
		Timekeeper_Reset_Task((Timekeeper_Task_ID)i);
	}
}
