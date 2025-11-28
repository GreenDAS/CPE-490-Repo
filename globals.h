/**************************************************************************
 * Author  : Daniel S.
 * Date    : 11/17/2025
 * File    : globals.h
 * Version : V1.0
 * Description:
 *   Central header for global variables and shared types
 **************************************************************************/

#pragma once

/* -------------------------------------------------------------------------
 * Configuration & Constants
 * ------------------------------------------------------------------------- */
#define TRUE                1
#define FALSE               0

/* Pin assignments (use uppercase + _t suffix for clarity) */


// GPIOA
#define ROW1                8
#define ROW2                9
#define ROW3                10
#define ROW4                11
//

// GPIOB
#define COL1                8
#define COL2                9
#define COL3                10
//

// GPIOC
#define LED1                0
#define LED2                1
#define LED3                2
#define LED4                3
#define SW1                 4
#define SW2                 5
//

#define MAX_TASKS           3

#define SYSTICK_MAX         1200
#define LINE_SIZE           15   /* 16 chars total, index 15 = null terminator */

// RPM Bounds
#define RPM_UPPER           200
#define RPM_LOWER           25

// Deadlines
#define VOLTAGE_DEADLINE    400
#define FREQ_DEADLINE       500
#define DISPLAY_DEADLINE    600
#define READ_NUMPAD_DEADLINE 25
#define HANDLE_NUMPAD_PRESS_DEADLINE 100
/* -------------------------------------------------------------------------
 * Type Definitions
 * ------------------------------------------------------------------------- */
typedef enum {
    START,
    UPPER,
    LOWER,
    SUCCESS
} dispState;
//hampter
/* EDF scheduler task table */
typedef struct {
    void (*tasks[MAX_TASKS])(void);
    int  deadlines[MAX_TASKS];
    int  cooldowns[MAX_TASKS];
    int  clksWaited[MAX_TASKS];
	int  (*taskCond[MAX_TASKS])(void);
    int  (*coolDownFn[MAX_TASKS])(void);
} EDFToDo;

/* Forward declarations – NEVER include full structs here */
struct IODevice;
struct GeneralPurposeTimer;
struct GenevaLCDDevice;
struct Numpad;

/* -------------------------------------------------------------------------
 * Global Variables – extern declarations only
 * -------------------------------------------------------------------------*/
extern struct IODevice         VoltReader;
extern struct IODevice         FreqReader;
extern struct GeneralPurposeTimer Timer2;
extern struct GeneralPurposeTimer Timer3;
extern struct GenevaLCDDevice *Display;
extern struct Numpad           *NumberPad;

extern int              voltageMeasurements;
extern float            voltageAccum;
extern float            frequency;
extern int              freqCounts;
extern double           timeElapsed;

extern float            targetRPM;

extern EDFToDo          schedulerTasks;
extern dispState      	displayState;

/* Flags */
extern unsigned int         calcVoltFlag;   /* 1 = always calculate voltage when ready */
extern unsigned int         calcFreqFlag;   /* 0 = off by default */
extern unsigned int         gettingUserInputFlag; /* 0 = not getting user input by default*/
extern unsigned int         targetSetFlag;  /* 0 = value not just set yet*/
extern unsigned int         readFinishedFlag; /* 0 = read not finished yet*/
extern volatile unsigned int     systickFlag; /* 0 = don't run ROTS*/
extern int              tor_rpm_toggle; // 0 for RPMs(target and actual), 1 for (Target and torque) only
extern int              togglestate;    // 0 for first string 1 for second //

/* -------------------------------------------------------------------------
 * End of globals.h
 * -------------------------------------------------------------------------*/
