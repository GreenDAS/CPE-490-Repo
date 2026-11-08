/*
 * RTOS.c
 *
 *	Priority-based task scheduler with O(1) next-task selection through a
 *	perfect hash table (De Bruijn multiply-and-shift). Can time every task with
 *	Timekeeper when Timekeeper is part of the build. See RTOS.h for the full
 *	explanation of how it works, with worked examples and reference links.
 *
 *	Quick reference for the hash table:
 *	  https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
 *	  https://bits.stephan-brumme.com/lowestBitSet.html
 *
 *  Created on: Oct 25, 2026
 *      Author: Dan
 */

#include "RTOS.h"
#include <stddef.h>

#if RTOS_USE_TIMEKEEPER
#include "Timekeeper.h"
#endif

// ---------------------------------------------------------------------------
// Private state
// ---------------------------------------------------------------------------

// Task control blocks. Each task's TCB lives at the index matching its priority.
// This array is only the storage; the scheduler never indexes it directly; it
// always finds a TCB through rtos_hash_table below.
static RTOS_Task_t rtos_tasks[RTOS_MAX_TASKS];

// The hash table: maps a one-hot ready bit (1 << priority) straight to that task's TCB.
// Lookups only ever go through RTOS_Hash(), never through a priority index, so the
// scheduler never searches for anything.
//
// Where each priority lands (slot = RTOS_Hash(1 << priority)):
//	priority:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
//	slot:      0  1  3  7 14 29 27 23 15 31 30 28 25 18  5 11
//	priority: 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
//	slot:     22 13 26 21 10 20  9 19  6 12 24 17  2  4  8 16
// Every slot 0-31 appears exactly once, so there are no collisions. In the
// debugger, the drone's six tasks (priority 0-5) show up in slots 0, 1, 3, 7,
// 14 and 29 of this array; every other slot stays NULL.
static RTOS_Task_t *rtos_hash_table[RTOS_MAX_TASKS];

// The three masks below each use one bit per task: bit n is the task at priority n.
static volatile uint32_t rtos_ready_mask = 0;		// bit n set = the task at priority n has been released and is waiting to run
static volatile uint32_t rtos_registered_mask = 0;	// bit n set = a task is registered at priority n
static volatile uint32_t rtos_enabled_mask = 0;		// bit n set = registered AND enabled, so RTOS_Tick() should count it down
static volatile uint32_t rtos_tick_count = 0;		// number of RTOS_Tick() calls since start/reset
static volatile uint8_t rtos_running = 0;			// 0 until RTOS_Start(); RTOS_Tick() only counts ticks until then

#if RTOS_USE_TIMEKEEPER
static volatile uint8_t rtos_timekeeping = 1;	// on by default when compiled in
#endif

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

/**
 * @brief Perfect hash for a one-hot 32-bit key. Multiplying 2^n by the De Bruijn
 * 		  constant 0x077CB531 is the same as shifting it left by n, and the top 5 bits
 * 		  of the result are different for each n = 0..31, so all 32 keys land in
 * 		  32 different slots with no collisions.
 *
 * 		  Worked example, key = 1 << 3 = 0x00000008 (the task at priority 3):
 * 		    0x077CB531 * 8 = 0x3BE5A988   (same as << 3; anything past bit 31 is dropped)
 * 		                   = 00111 011111001011010100110001000
 * 		    >> 27 keeps the top 5 bits:   00111 = 7  -> slot 7
 *
 * 		  The multiply has to stay 32-bit (the uint32_t cast) so the bits that shift
 * 		  past bit 31 fall off; that's what makes it read a 5-bit "window" of the
 * 		  constant. See section 3 of RTOS.h, and:
 * 		  https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
 * @param one_hot a value with exactly one bit set
 * @retval hash table slot, 0-31
 */
static inline uint32_t RTOS_Hash(uint32_t one_hot) {
	return (uint32_t)(one_hot * RTOS_DEBRUIJN_32) >> 27;
}

/**
 * @brief Isolates the lowest set bit, i.e. the highest-priority task in a mask.
 * 		  In two's complement, -mask = ~mask + 1. Flipping the bits turns the low 0s
 * 		  into 1s and the lowest 1 into a 0; adding 1 then carries through those 1s and
 * 		  stops exactly at that bit. So mask and -mask have only that one bit in common.
 * 		    mask        = 0010 1000   (tasks 3 and 5 ready)
 * 		    ~mask + 1   = 1101 1000
 * 		    AND         = 0000 1000   (task 3)
 * 		  https://www.chessprogramming.org/BitScan ("With isolated LS1B")
 * @param mask any mask (non-zero)
 * @retval mask with only its lowest set bit left
 */
static inline uint32_t RTOS_Lowest_Bit(uint32_t mask) {
	return mask & (~mask + 1U);	// same as mask & -mask, without negating an unsigned value
}

// Disables interrupts and returns the previous PRIMASK, so critical sections can nest safely.
// PRIMASK is the Cortex-M register that, when set to 1, blocks every interrupt with a
// configurable priority (here: TIM16, I2C, DMA, ...). Saving and restoring it, instead of
// always turning interrupts back on, means calling this while interrupts are already off
// won't accidentally turn them on early.
static inline uint32_t RTOS_Enter_Critical(void) {
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	return primask;
}

static inline void RTOS_Exit_Critical(uint32_t primask) {
	__set_PRIMASK(primask);
}

/**
 * @brief Works out how many ticks until a task's next release, so it fires on
 * 		  the first tick t > now where (t % period) == offset. This matches the old
 * 		  "!((g_timer_ticks - offset) % period)" flag math in main.c, but costs
 * 		  only one decrement per tick from then on.
 *
 * 		  Example: period = 20, offset = 5, now = 42.
 * 		    next_phase = 43 % 20 = 3          (where tick 43 sits in the period)
 * 		    ((5 + 20 - 3) % 20) + 1 = 2 + 1 = 3
 * 		    so it releases in 3 ticks, on tick 45, and 45 % 20 == 5.
 * 		  Adding period before subtracting keeps the math from going negative
 * 		  (it's all unsigned).
 * @param task the task to line up
 * @param now the current tick count
 * @retval countdown value, 1 to period_ticks
 */
static uint32_t RTOS_First_Countdown(const RTOS_Task_t *task, uint32_t now) {
	uint32_t next_phase = (now + 1U) % task->period_ticks;
	return ((task->offset_ticks + task->period_ticks - next_phase) % task->period_ticks) + 1U;
}

/**
 * @brief Looks up the TCB for a priority through the hash table. Turns the
 * 		  priority into its one-hot key (1 << priority) and hashes it, exactly the
 * 		  way RTOS_Add_Task() stored it
 * @param priority 0-31
 * @retval the TCB, or NULL if out of range or nothing registered there
 */
static RTOS_Task_t *RTOS_Lookup(uint8_t priority) {
	if (priority >= RTOS_MAX_TASKS) return NULL;
	return rtos_hash_table[RTOS_Hash(1UL << priority)];
}

#if RTOS_USE_TIMEKEEPER
/**
 * @brief Runs a task between Timekeeper_Start() and Timekeeper_Stop() so its runtime gets recorded, and counts
 * 		  an overrun if it ran past its limit. Tracking only - the RTOS is co-operative, so a task always runs to
 * 		  completion.
 * @param task the task to run
 * @retval None
 */
static void RTOS_Dispatch_Timed(RTOS_Task_t * const task) {
	// RTOS priority n and Timekeeper task ID n are the same task
	const Timekeeper_Task_ID id = (Timekeeper_Task_ID)task->priority;

	Timekeeper_Start(id);
	task->function(task->context);
	Timekeeper_Stop(id);
	task->run_count++;

	// The overrun is only checked after the task has finished; nothing stops it mid-run
	uint32_t runtime_us = Timekeeper_Get_Last_Runtime_us(id);
	if (task->limit_us != 0 && runtime_us > task->limit_us) {
		task->overrun_count++;
	}
}
#endif

/**
 * @brief Runs one task, with or without Timekeeper depending on the build and the runtime switch
 * @param task the task to run
 * @retval None
 */
static void RTOS_Dispatch(RTOS_Task_t * const task) {
#if RTOS_USE_TIMEKEEPER
	// Timekeeper only has slots for TIMEKEEPER_NUM_TASKS tasks, so higher priorities run untimed
	if (rtos_timekeeping && task->priority < TIMEKEEPER_NUM_TASKS) {
		RTOS_Dispatch_Timed(task);
		return;
	}
#endif
	task->function(task->context);
	task->run_count++;
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

/**
 * @brief Clears every TCB and the hash table, and starts Timekeeper if it's compiled in. Call once after
 * 		  Startup() and before adding tasks
 * @param None
 * @retval None
 */
void RTOS_Init(void) {
	uint32_t primask = RTOS_Enter_Critical();	// TIM16 may already be ticking, so keep RTOS_Tick() out while clearing
	rtos_running = 0;
	rtos_ready_mask = 0;
	rtos_registered_mask = 0;
	rtos_enabled_mask = 0;
	rtos_tick_count = 0;
	for (uint32_t i = 0; i < RTOS_MAX_TASKS; i++) {
		rtos_hash_table[i] = NULL;	// NULL slot = no task hashes here
		rtos_tasks[i] = (RTOS_Task_t){0};
	}
	RTOS_Exit_Critical(primask);

#if RTOS_USE_TIMEKEEPER
	Timekeeper_Init();	// starts the DWT cycle counter Timekeeper times tasks with
	rtos_timekeeping = 1;
#endif
}

/**
 * @brief Registers a task at a priority and puts it in the hash table
 * @param priority 0 (highest) to 31. Each priority can only hold one task
 * @param function the task's code
 * @param context pointer handed back to function on every call (can be NULL)
 * @param period_ticks release the task every period_ticks system ticks (must be > 0)
 * @param offset_ticks phase within the period, so tasks with the same period can be spread apart
 * @param name label for debugging (can be NULL)
 * @retval RTOS_OK or an error code
 */
RTOS_Status RTOS_Add_Task(uint8_t priority, RTOS_Task_Function function, void *context,
		uint32_t period_ticks, uint32_t offset_ticks, const char *name) {
	if (priority >= RTOS_MAX_TASKS) return RTOS_ERR_BAD_PRIORITY;
	if (function == NULL) return RTOS_ERR_NULL_FUNCTION;
	if (period_ticks == 0) return RTOS_ERR_BAD_PERIOD;

	// The task's one-hot key: its bit in every mask, and what gets hashed to find its slot
	uint32_t key = 1UL << priority;
	if (rtos_registered_mask & key) return RTOS_ERR_PRIORITY_TAKEN;

	// Fill in the TCB first, while nothing points at it yet
	RTOS_Task_t *task = &rtos_tasks[priority];
	*task = (RTOS_Task_t){0};
	task->function = function;
	task->context = context;
	task->name = name;
	task->period_ticks = period_ticks;
	task->offset_ticks = offset_ticks % period_ticks;
	task->priority = priority;
	task->enabled = 1;

	// Then publish it: put it in the hash table and set its mask bits. Done with interrupts off so
	// RTOS_Tick() never sees the task half-added (e.g. enabled but not yet in the table)
	uint32_t primask = RTOS_Enter_Critical();
	task->countdown = RTOS_First_Countdown(task, rtos_tick_count);
	rtos_hash_table[RTOS_Hash(key)] = task;
	rtos_registered_mask |= key;
	rtos_enabled_mask |= key;
	RTOS_Exit_Critical(primask);

	return RTOS_OK;
}

/**
 * @brief Unregisters a task, drops it from the hash table, and clears any pending release for it
 * @param priority which task
 * @retval RTOS_OK or RTOS_ERR_NO_TASK
 */
RTOS_Status RTOS_Remove_Task(uint8_t priority) {
	if (RTOS_Lookup(priority) == NULL) return RTOS_ERR_NO_TASK;
	uint32_t key = 1UL << priority;

	uint32_t primask = RTOS_Enter_Critical();
	rtos_registered_mask &= ~key;
	rtos_enabled_mask &= ~key;
	rtos_ready_mask &= ~key;
	rtos_hash_table[RTOS_Hash(key)] = NULL;
	RTOS_Exit_Critical(primask);

	return RTOS_OK;
}

/**
 * @brief Lets a disabled task be released again, lined up with its normal period/offset from the current tick
 * @param priority which task
 * @retval RTOS_OK or RTOS_ERR_NO_TASK
 */
RTOS_Status RTOS_Enable_Task(uint8_t priority) {
	RTOS_Task_t *task = RTOS_Lookup(priority);
	if (task == NULL) return RTOS_ERR_NO_TASK;

	uint32_t primask = RTOS_Enter_Critical();
	task->countdown = RTOS_First_Countdown(task, rtos_tick_count);
	task->enabled = 1;
	rtos_enabled_mask |= (1UL << priority);
	RTOS_Exit_Critical(primask);

	return RTOS_OK;
}

/**
 * @brief Stops a task from being released (stays registered). Also clears a release that's already pending
 * @param priority which task
 * @retval RTOS_OK or RTOS_ERR_NO_TASK
 */
RTOS_Status RTOS_Disable_Task(uint8_t priority) {
	RTOS_Task_t *task = RTOS_Lookup(priority);
	if (task == NULL) return RTOS_ERR_NO_TASK;

	uint32_t primask = RTOS_Enter_Critical();
	task->enabled = 0;
	rtos_enabled_mask &= ~(1UL << priority);
	rtos_ready_mask &= ~(1UL << priority);
	RTOS_Exit_Critical(primask);

	return RTOS_OK;
}

/**
 * @brief Sets a task's time budget, used to count overruns (checked after the task finishes, only when
 * 		  Timekeeper is on). Also passed to Timekeeper_Set_Limit_us() so both overrun counts use the same number
 * @param priority which task
 * @param limit_us budget in microseconds, 0 = no limit
 * @retval RTOS_OK or RTOS_ERR_NO_TASK
 */
RTOS_Status RTOS_Set_Task_Limit_us(uint8_t priority, uint32_t limit_us) {
	RTOS_Task_t *task = RTOS_Lookup(priority);
	if (task == NULL) return RTOS_ERR_NO_TASK;
	task->limit_us = limit_us;
#if RTOS_USE_TIMEKEEPER
	if (priority < TIMEKEEPER_NUM_TASKS) {
		Timekeeper_Set_Limit_us((Timekeeper_Task_ID)priority, limit_us);
	}
#endif
	return RTOS_OK;
}

/**
 * @brief Runtime on/off switch for Timekeeper timing. Does nothing if Timekeeper isn't compiled in
 * @param enable 1 = time tasks, 0 = run them bare
 * @retval None
 */
void RTOS_Set_Timekeeping(uint8_t enable) {
#if RTOS_USE_TIMEKEEPER
	rtos_timekeeping = enable ? 1 : 0;
#else
	(void)enable;
#endif
}

/**
 * @brief Checks whether tasks are currently being timed
 * @param None
 * @retval 1 if Timekeeper is compiled in AND switched on, otherwise 0
 */
uint8_t RTOS_Timekeeping_Enabled(void) {
#if RTOS_USE_TIMEKEEPER
	return rtos_timekeeping;
#else
	return 0;
#endif
}

// ---------------------------------------------------------------------------
// Running
// ---------------------------------------------------------------------------

/**
 * @brief Lines up every task's countdown with the current tick, starts releasing tasks, and runs the
 * 		  scheduler loop forever
 * @param None
 * @retval None (never returns)
 */
void RTOS_Start(void) {
	uint32_t primask = RTOS_Enter_Critical();
	// Walk every registered task, one set bit at a time (see the loop note in RTOS_Tick())
	uint32_t pending = rtos_registered_mask;
	while (pending) {
		uint32_t key = RTOS_Lowest_Bit(pending);
		RTOS_Task_t *task = rtos_hash_table[RTOS_Hash(key)];
		task->countdown = RTOS_First_Countdown(task, rtos_tick_count);
		pending &= pending - 1U;	// clear that bit
	}
	rtos_ready_mask = 0;
	rtos_running = 1;	// from the next TIM16 tick on, RTOS_Tick() starts releasing tasks
	RTOS_Exit_Critical(primask);

	// The scheduler loop. When nothing is ready, RTOS_Run_Next() returns right away and this just spins
	// until the next TIM16 tick releases something.
	for (;;) {
		RTOS_Run_Next();
	}
}

/**
 * @brief Picks the highest-priority ready task in O(1) through the hash table, clears its ready bit, and runs it
 *
 * 		  Example: tasks 2 and 4 are ready, so rtos_ready_mask = 0b10100 = 0x14
 * 		    key  = RTOS_Lowest_Bit(0x14)      = 0b00100 = 0x04   (task 2 wins)
 * 		    mask = 0x14 & ~0x04               = 0x10             (task 4 still waiting)
 * 		    slot = RTOS_Hash(0x04)            = 3
 * 		    task = rtos_hash_table[3]         -> task 2's TCB, which then runs
 * 		  Next call, task 4 runs, unless task 0, 1, 2 or 3 got released again while task 2 was running,
 * 		  in which case that one goes first.
 * @param None
 * @retval priority of the task that ran, or RTOS_NO_TASK if none were ready
 */
uint8_t RTOS_Run_Next(void) {
	uint32_t ready = rtos_ready_mask;	// quick check without touching interrupts
	if (ready == 0) return RTOS_NO_TASK;

	// Step 1: take the highest-priority ready bit and clear it, with interrupts off so a TIM16 tick
	// can't set another bit between reading the mask and writing it back (see section 5 of RTOS.h)
	uint32_t primask = RTOS_Enter_Critical();
	ready = rtos_ready_mask;	// re-read inside the critical section
	uint32_t key = RTOS_Lowest_Bit(ready);
	rtos_ready_mask = ready & ~key;
	RTOS_Exit_Critical(primask);

	// Steps 2 and 3: hash the one-hot key to a slot and load the TCB pointer from it
	RTOS_Task_t * const task = rtos_hash_table[RTOS_Hash(key)];
	if (task == NULL) return RTOS_NO_TASK;	// removed between release and now

	// Run it, with interrupts back on. Co-operative: this returns only when the task does
	RTOS_Dispatch(task);
	return task->priority;
}

/**
 * @brief System tick. Call once per TIM16 interrupt. Counts down each enabled task (only walking the bits
 * 		  that are actually set) and marks a task ready when its countdown runs out
 * @param None
 * @retval None
 */
void RTOS_Tick(void) {
	rtos_tick_count++;
	if (!rtos_running) return;

	// Loop over only the enabled tasks, not all 32 slots. Each pass:
	//	- RTOS_Lowest_Bit() picks one task's bit out of pending,
	//	- the hash table turns that bit into the TCB,
	//	- pending &= pending - 1 clears that bit (subtracting 1 flips the lowest 1 and the 0s below it,
	//	  so the AND removes exactly that bit),
	// so the loop runs once per enabled task (6 times for the drone) and then stops.
	// This runs inside the TIM16 interrupt, where nothing else can change the masks, so no critical
	// section is needed here.
	uint32_t pending = rtos_enabled_mask;
	uint32_t newly_ready = 0;
	while (pending) {
		uint32_t key = RTOS_Lowest_Bit(pending);
		RTOS_Task_t *task = rtos_hash_table[RTOS_Hash(key)];
		if (--task->countdown == 0) {
			task->countdown = task->period_ticks;	// reload for the next period
			if (rtos_ready_mask & key) {
				task->missed_count++;	// still waiting from its last release - it missed that deadline
			}
			newly_ready |= key;
		}
		pending &= pending - 1U;
	}
	// Release everything that came due this tick in one write. The main loop picks them up in priority order.
	rtos_ready_mask |= newly_ready;
}

/**
 * @brief Clears all pending releases, sets the RTOS tick count back to 0, and lines every countdown up again
 * 		  from tick 0. Software_Kill() calls this alongside setting g_timer_ticks back to 0
 * @param None
 * @retval None
 */
void RTOS_Reset(void) {
	uint32_t primask = RTOS_Enter_Critical();
	rtos_tick_count = 0;
	rtos_ready_mask = 0;
	uint32_t pending = rtos_registered_mask;
	while (pending) {
		uint32_t key = RTOS_Lowest_Bit(pending);
		RTOS_Task_t *task = rtos_hash_table[RTOS_Hash(key)];
		task->countdown = RTOS_First_Countdown(task, 0);
		pending &= pending - 1U;
	}
	RTOS_Exit_Critical(primask);
}

// ---------------------------------------------------------------------------
// Reading back state
// ---------------------------------------------------------------------------

uint32_t RTOS_Get_Ready_Mask(void) {
	return rtos_ready_mask;
}

uint32_t RTOS_Get_Tick_Count(void) {
	return rtos_tick_count;
}

// Same O(1) pick as RTOS_Run_Next(), but only looks; it doesn't clear the bit or run anything
uint8_t RTOS_Peek_Next(void) {
	uint32_t ready = rtos_ready_mask;
	if (ready == 0) return RTOS_NO_TASK;
	RTOS_Task_t *task = rtos_hash_table[RTOS_Hash(RTOS_Lowest_Bit(ready))];
	return (task != NULL) ? task->priority : RTOS_NO_TASK;
}

const RTOS_Task_t *RTOS_Get_Task(uint8_t priority) {
	return RTOS_Lookup(priority);
}

uint32_t RTOS_Get_Run_Count(uint8_t priority) {
	RTOS_Task_t *task = RTOS_Lookup(priority);
	return (task != NULL) ? task->run_count : 0;
}

uint32_t RTOS_Get_Missed_Count(uint8_t priority) {
	RTOS_Task_t *task = RTOS_Lookup(priority);
	return (task != NULL) ? task->missed_count : 0;
}

uint32_t RTOS_Get_Overrun_Count(uint8_t priority) {
	RTOS_Task_t *task = RTOS_Lookup(priority);
	return (task != NULL) ? task->overrun_count : 0;
}
