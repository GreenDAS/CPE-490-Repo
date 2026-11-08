/*
 * RTOS.h
 *
 *  Created on: Oct 25, 2026
 *      Author: Dan
 */

#ifndef INC_RTOS_H_
#define INC_RTOS_H_

#include <stdint.h>
#include "stm32l476xx.h"	// CMSIS core (PRIMASK, DWT) - CPE 490 is register-level, no HAL

/* ============================================================================
 * RTOS
 *
 * Priority-based, co-operative task scheduler for the drone. Replaces the old
 * priority-flag blocks that lived directly inside main()'s while(1) loop and
 * the flag math in HAL_TIM_PeriodElapsedCallback().
 *
 * CPE 490 port (Oct 27, 2026): same scheduler code as the drone, but here it
 * replaces the EDF loop that used to sit in main(). The system tick is
 * SysTick (1 ms, set up by InitSysTick() in _init_.c) instead of TIM16
 * (50 us), so wherever the notes below say "TIM16", read "SysTick", and one
 * tick = 1 ms. See main.c for how the old ready/cooldown functions are run
 * on top of it.
 *
 * ----------------------------------------------------------------------------
 * 1. The big picture
 * ----------------------------------------------------------------------------
 * There are only two places the RTOS code ever runs from:
 *
 *	  TIM16 interrupt (every 50 us)          main() -> RTOS_Start() -> for(;;)
 *	  ------------------------------         ------------------------------------
 *	  RTOS_Tick()                            RTOS_Run_Next()
 *	   - counts every task down by one        - picks the highest-priority task
 *	   - when a count hits 0, sets that         whose bit is set in the ready mask
 *	     task's bit in the ready mask         - clears that bit
 *	     ("releases" the task)                - calls the task's function
 *	                                          - repeats forever
 *
 *	The interrupt side only *marks* tasks as ready; it never runs task code.
 *	The main loop side only *runs* tasks; it never decides when they're due.
 *	The ready mask (one 32-bit variable) is the only thing they share.
 *
 *	Co-operative means a task is never interrupted to let another task run.
 *	Once RTOS_Run_Next() calls a task's function, that function runs until it
 *	returns. Interrupts (TIM16, I2C, DMA, ...) still happen in the middle of a
 *	task, but they only do their small job and return to the same task. So a
 *	task that takes too long delays every other task, which is why overruns
 *	(see Timekeeper below) are worth tracking.
 *	  More on co-operative vs. preemptive scheduling:
 *	  https://en.wikipedia.org/wiki/Cooperative_multitasking
 *
 * ----------------------------------------------------------------------------
 * 2. Tasks, priorities and the ready mask
 * ----------------------------------------------------------------------------
 *	- Each task is registered with a priority (0 = highest, up to 31), a
 *	  period and a phase offset, both in system ticks (one tick = one TIM16
 *	  interrupt = 50 us right now).
 *	- Only one task can live at each priority, so the priority doubles as the
 *	  task's ID. Each task "owns" one bit of a 32-bit number: priority n owns
 *	  bit n, i.e. the value (1 << n).
 *	- The ready mask is a 32-bit number with bit n set when the task at
 *	  priority n has been released and is waiting to run. Example: tasks 0, 3
 *	  and 5 are waiting:
 *
 *		bit:          ... 7 6 5 4 3 2 1 0
 *		ready mask  = ... 0 0 1 0 1 0 0 1   = 0x29
 *
 *	  Because priority 0 is bit 0, "the highest-priority ready task" is always
 *	  the lowest set bit of the mask.
 *
 * ----------------------------------------------------------------------------
 * 3. Picking the next task in O(1): the hash table
 * ----------------------------------------------------------------------------
 *	A simple scheduler would loop over every task asking "are you ready?",
 *	which takes longer the more tasks there are (O(n)). This one instead does
 *	the same fixed handful of instructions no matter how many tasks exist
 *	(O(1)), in three steps:
 *
 *	Step 1 - isolate the lowest set bit:  key = ready & -ready
 *		In two's complement, -x is (~x + 1). Flipping every bit and adding 1
 *		carries up through all the low 0s and stops at the lowest 1, so x and
 *		-x share only that one bit. ANDing them leaves just that bit:
 *
 *			ready          = 0010 1001   (0x29: tasks 0, 3, 5 ready)
 *			-ready         = 1101 0111
 *			ready & -ready = 0000 0001   -> task 0 is the highest priority
 *
 *		The result is "one-hot": exactly one bit set. It's a power of two,
 *		1 << n, but we don't know n yet.
 *
 *	Step 2 - hash the one-hot key into a table slot:
 *			slot = (key * 0x077CB531) >> 27
 *		We need to turn 1 << n into a table index. A plain array indexed by
 *		the key itself would need 2^32 entries, so instead the key goes
 *		through a hash function that squeezes it into 0-31.
 *		  Multiplying by 1 << n is the same as shifting left by n, so
 *		key * 0x077CB531 is just the constant shifted left n places. The
 *		constant is a "De Bruijn sequence": a 32-bit pattern in which every
 *		possible 5-bit pattern appears exactly once if you read 5 bits at a
 *		time, starting at each bit position. Shifting it left by n and keeping
 *		the top 5 bits (>> 27) reads the 5-bit window that starts n places in,
 *		and since every window is different, every n gives a different slot:
 *
 *			0x077CB531      = 00000 111011111001011010100110001
 *			key = 1 << 0  -> top 5 bits 00000 -> slot  0
 *			key = 1 << 1  -> top 5 bits 00001 -> slot  1
 *			key = 1 << 2  -> top 5 bits 00011 -> slot  3
 *			key = 1 << 3  -> top 5 bits 00111 -> slot  7
 *			key = 1 << 4  -> top 5 bits 01110 -> slot 14
 *			key = 1 << 5  -> top 5 bits 11101 -> slot 29
 *			... and so on, all 32 slots used exactly once.
 *
 *		A hash function where no two keys ever land in the same slot is
 *		called a "perfect hash", so the table never needs collision handling
 *		(no chaining, no probing). The slots look scrambled, but that doesn't
 *		matter: RTOS_Add_Task() stores each task with the same hash that the
 *		scheduler later looks it up with.
 *
 *	Step 3 - load the pointer:  task = rtos_hash_table[slot]
 *		Each slot holds a pointer straight to that task's control block
 *		(RTOS_Task_t), so the scheduler can call task->function right away.
 *
 *	Total cost of a pick: one AND, one negate, one multiply, one shift and one
 *	memory load. The Cortex-M4 does a 32-bit multiply in a single cycle, so
 *	this is a few cycles whether there are 2 tasks or 32.
 *	(Side note: the Cortex-M4 also has RBIT and CLZ instructions, so
 *	__CLZ(__RBIT(ready)) would find n directly. FreeRTOS uses CLZ like that
 *	for its "optimised task selection". The De Bruijn hash does the same job
 *	in portable C that works on any CPU with a fast multiply.)
 *
 *	Further reading on the hash table / De Bruijn trick:
 *	  - Bit Twiddling Hacks (Stanford), "Count the consecutive zero bits
 *	    (trailing) on the right with multiply and lookup" - the exact
 *	    0x077CB531 code this is based on:
 *	    https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
 *	  - Step-by-step walk-through of the same code, with explanation:
 *	    https://bits.stephan-brumme.com/lowestBitSet.html
 *	  - Chess Programming Wiki, BitScan (see "De Bruijn Multiplication" and
 *	    "With isolated LS1B" for the x & -x part), with diagrams:
 *	    https://www.chessprogramming.org/BitScan
 *	  - De Bruijn sequences in general, including this use:
 *	    https://en.wikipedia.org/wiki/De_Bruijn_sequence
 *	  - Perfect hash functions in general:
 *	    https://en.wikipedia.org/wiki/Perfect_hash_function
 *	  - Finding the lowest set bit ("find first set"), and the hardware
 *	    instructions (CLZ etc.) that do it:
 *	    https://en.wikipedia.org/wiki/Find_first_set
 *	  - Original paper: Leiserson, Prokop & Randall, "Using de Bruijn
 *	    Sequences to Index a 1 in a Computer Word" (MIT, 1998):
 *	    https://supertech.mit.edu/?p=187
 *
 * ----------------------------------------------------------------------------
 * 4. Releasing tasks: periods, offsets and countdowns
 * ----------------------------------------------------------------------------
 *	- A task with period P and offset O is released on every tick t where
 *	  (t % P) == O. The offset spreads tasks with the same period apart so
 *	  they don't all become ready on the same tick.
 *	- Doing a % on every task every tick would be slow inside an interrupt,
 *	  so each task keeps a countdown instead. RTOS_First_Countdown() works out
 *	  the first one with %, once. After that RTOS_Tick() only subtracts 1, and
 *	  when the countdown hits 0 it reloads it with P and sets the task's ready
 *	  bit. This releases tasks on exactly the same ticks as the old
 *	  "!((g_timer_ticks - offset) % period)" math in main.c.
 *	- If a task's bit is still set when it's released again, it never got to
 *	  run for its last release. That's counted in missed_count.
 *
 * ----------------------------------------------------------------------------
 * 5. Critical sections
 * ----------------------------------------------------------------------------
 *	The ready mask is changed both by RTOS_Tick() (in the TIM16 interrupt) and
 *	by RTOS_Run_Next() (in main). Clearing a bit is a read-modify-write: load,
 *	AND, store. If the interrupt fired between the load and the store, the
 *	main loop would write back its old copy and wipe out the bit the
 *	interrupt just set, and that task would silently never run. So every
 *	change to the shared masks from main is wrapped in a short critical
 *	section that turns interrupts off (PRIMASK) for a few instructions. Task
 *	code itself always runs with interrupts on.
 *
 * ----------------------------------------------------------------------------
 * 6. Timekeeper (optional)
 * ----------------------------------------------------------------------------
 *	If Timekeeper.h is in the project, the RTOS times every task with it
 *	automatically and counts an overrun whenever a task runs past its limit.
 *	This is tracking only: the RTOS is co-operative, so a task always runs to
 *	completion and is never interrupted or cut off. If Timekeeper.h/.c are
 *	removed, or RTOS_USE_TIMEKEEPER is defined as 0, the RTOS still builds and
 *	runs the same way, just without timing. It can also be switched on and
 *	off at runtime with RTOS_Set_Timekeeping().
 * ============================================================================
 */

// ---------------------------------------------------------------------------
// Build options
// ---------------------------------------------------------------------------

// Set to 1 to time tasks with Timekeeper, 0 to build without it. If left
// undefined, it turns itself on only when Timekeeper.h can be found. You can
// also override it with -DRTOS_USE_TIMEKEEPER=0 in the compiler settings.
// (__has_include is a GCC/Clang feature that asks "does this header exist?"
// at compile time, which is what lets Timekeeper be optional.)
#ifndef RTOS_USE_TIMEKEEPER
	#if defined(__has_include)
		#if __has_include("Timekeeper.h")
			#define RTOS_USE_TIMEKEEPER 1
		#else
			#define RTOS_USE_TIMEKEEPER 0
		#endif
	#else
		#define RTOS_USE_TIMEKEEPER 0
	#endif
#endif

// Max number of tasks: one per bit of the 32-bit ready mask, so priority 0-31
#define RTOS_MAX_TASKS 32U

// De Bruijn constant behind the perfect hash (section 3 above). In binary it
// is 00000111011111001011010100110001: reading any 5 bits in a row out of it
// (wrapping in 0s off the end) gives each of the 32 values 0-31 exactly once.
// See RTOS_Hash() in RTOS.c.
#define RTOS_DEBRUIJN_32 0x077CB531U

// Marks "no task" in return values
#define RTOS_NO_TASK 0xFFU

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

// Every task is a plain function. The context pointer passed to RTOS_Add_Task()
// is handed back on every call, or NULL if you don't need one.
typedef void (*RTOS_Task_Function)(void *context);

typedef enum {
	RTOS_OK = 0,
	RTOS_ERR_BAD_PRIORITY,		// priority >= RTOS_MAX_TASKS
	RTOS_ERR_PRIORITY_TAKEN,	// a task is already registered at that priority
	RTOS_ERR_NULL_FUNCTION,		// no function given
	RTOS_ERR_BAD_PERIOD,		// period_ticks was 0
	RTOS_ERR_NO_TASK			// nothing registered at that priority
} RTOS_Status;

// Task control block (TCB). One per registered task. This is everything the
// RTOS knows about a task; the hash table's slots point at these.
// Fields marked volatile are changed by RTOS_Tick() inside the TIM16
// interrupt, so the compiler must re-read them from memory every time instead
// of keeping a stale copy in a register.
typedef struct {
	RTOS_Task_Function function;	// code to run
	void *context;					// handed back to function on every call
	const char *name;				// for debugging only (shows up in the debugger's watch window)
	uint32_t period_ticks;			// release every period_ticks system ticks
	uint32_t offset_ticks;			// release when (tick % period_ticks) == offset_ticks. Spreads tasks apart so they don't all land on the same tick
	volatile uint32_t countdown;	// ticks left until the next release. Counted down by RTOS_Tick()
	uint32_t limit_us;				// time budget in microseconds, 0 = none. Only checked when Timekeeper is on
	uint32_t run_count;				// how many times the task has run
	volatile uint32_t missed_count;	// times it was released again before its last release had run (a missed deadline)
	uint32_t overrun_count;			// times it ran past limit_us (Timekeeper only)
	uint8_t priority;				// 0 = highest. Also the task's bit in the ready mask and its Timekeeper task ID
	volatile uint8_t enabled;		// 0 = never released (it stays registered)
} RTOS_Task_t;

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

// Clears all tasks and the hash table. If Timekeeper is compiled in, this also calls Timekeeper_Init().
// Call once, after _init_() and before RTOS_Add_Task().
void RTOS_Init(void);

// Registers a task. priority = 0 (highest) to 31. period/offset are in system ticks (1 tick = 1 ms SysTick on CPE 490).
RTOS_Status RTOS_Add_Task(uint8_t priority, RTOS_Task_Function function, void *context,
		uint32_t period_ticks, uint32_t offset_ticks, const char *name);
RTOS_Status RTOS_Remove_Task(uint8_t priority);
RTOS_Status RTOS_Enable_Task(uint8_t priority);
RTOS_Status RTOS_Disable_Task(uint8_t priority);

// Sets a task's time budget, used to count overruns. Also passed along to Timekeeper_Set_Limit_us() when Timekeeper is compiled in.
RTOS_Status RTOS_Set_Task_Limit_us(uint8_t priority, uint32_t limit_us);

// Runtime on/off switch for Timekeeper timing. Does nothing when Timekeeper isn't compiled in.
void RTOS_Set_Timekeeping(uint8_t enable);
uint8_t RTOS_Timekeeping_Enabled(void);	// 1 only if compiled in AND switched on

// ---------------------------------------------------------------------------
// Running
// ---------------------------------------------------------------------------

// Lines every task's countdown up with the current tick and starts releasing tasks.
// RTOS_Start() then runs the scheduler loop forever and never returns.
void RTOS_Start(void);

// Runs the single highest-priority ready task, if there is one. Returns that task's
// priority, or RTOS_NO_TASK if nothing was ready. RTOS_Start() calls this in a loop;
// use it directly only if you want your own loop.
uint8_t RTOS_Run_Next(void);

// Call from the system tick interrupt (SysTick_Handler() on CPE 490) once per tick.
void RTOS_Tick(void);

// Clears every pending release and sets the RTOS tick count back to 0, then re-lines up every
// countdown. (On the drone, Software_Kill() uses this. CPE 490 doesn't call it yet.)
void RTOS_Reset(void);

// ---------------------------------------------------------------------------
// Reading back state
// ---------------------------------------------------------------------------
uint32_t RTOS_Get_Ready_Mask(void);
uint32_t RTOS_Get_Tick_Count(void);
uint8_t RTOS_Peek_Next(void);	// priority of the task that would run next, or RTOS_NO_TASK. Doesn't run it
const RTOS_Task_t *RTOS_Get_Task(uint8_t priority);	// NULL if nothing registered there
uint32_t RTOS_Get_Run_Count(uint8_t priority);
uint32_t RTOS_Get_Missed_Count(uint8_t priority);
uint32_t RTOS_Get_Overrun_Count(uint8_t priority);

#endif /* INC_RTOS_H_ */
