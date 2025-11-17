/**************************************************************************
 * Author  Daniel S.
 * Date    11/17/2025
 * File    globals.h
 * Version V1.0
 * Description 
 * Used to make global variables
 *
 **************************************************************************
*/
//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "stm32l476xx.h"

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------
# define True 1
# define False 0
# define LED1 0
# define LED2 1
# define LED3 2
# define LED4 3

# define COL1 3
# define COL2 2
# define COL3 1

# define ROW1 7
# define ROW2 6
# define ROW3 5
# define ROW4 4

# define SW1 4
# define SW2 5


#define MaxTasks 3
#define vDeadline 400
#define fDeadline 500
#define dDeadline 600
#define systick_counterMax 1200
#define lineSize 15 // 16 Max however index 15 is the control character

//------------------------------------------------------------------------------
// # Type Definitions
//------------------------------------------------------------------------------

typedef enum dispState {START, UPPER, LOWER, SUCCESS} dispState;

typedef struct EDFToDo{ // A Struct designed to hold tasks and information for EDF Scheduling
	void (*tasks[MaxTasks])(void);
	int deadlines[MaxTasks];
	int cooldowns[MaxTasks];
	int clksWaited[MaxTasks];
} EDFToDo;

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------