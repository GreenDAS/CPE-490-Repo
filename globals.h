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
#define LED1                0
#define LED2                1
#define LED3                2
#define LED4                3
#define COL1                3
#define COL2                2
#define COL3                1
#define ROW1                7
#define ROW2                6
#define ROW3                5
#define ROW4                4
#define SW1                 4
#define SW2                 5

#define MAX_TASKS           3
#define VOLTAGE_DEADLINE    400
#define FREQ_DEADLINE       500
#define DISPLAY_DEADLINE    600
#define SYSTICK_MAX         1200
#define LINE_SIZE           15   /* 16 chars total, index 15 = null terminator */

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
typedef struct IODevice		         	IODevice;
typedef struct GeneralPurposeTimer 		GeneralPurposeTimer;
typedef struct GenevaLCDDevice			GenevaLCDDevice;

/* -------------------------------------------------------------------------
 * Global Variables – extern declarations only
 * ------------------------------------------------------------------------- */
extern IODevice         VoltReader;
extern IODevice         FreqReader;
extern GeneralPurposeTimer Timer2;
extern GeneralPurposeTimer Timer3;
extern GenevaLCDDevice *Display;

extern int              voltageMeasurements;
extern float            voltageAccum;
extern float            frequency;
extern int              freqCounts;
extern double           timeElapsed;

extern EDFToDo          schedulerTasks;
extern dispState      	displayState;

/* Flags */
extern int              calcVoltFlag;   /* 1 = always calculate voltage when ready */
extern int              calcFreqFlag;   /* 0 = off by default */

/* -------------------------------------------------------------------------
 * End of globals.h
 * ------------------------------------------------------------------------- */