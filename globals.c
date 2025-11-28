/**************************************************************************
 * Author  Daniel S.
 * Date    10/13/2025
 * File    _init_.h
 * Version V1.0
 * Description 
 * INIT HEADER
 *
 **************************************************************************
*/
//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "globals.h"

#include "gpio_lib.h"
#include "timer_lib.h"
#include "stdio.h"
#include "stdlib.h"
#include "lcd_lib.h"
#include "string.h"
#include "string_lib.h"

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------


// Global Vars
struct IODevice VoltReader;
struct IODevice FreqReader;
struct GeneralPurposeTimer Timer2;
struct GeneralPurposeTimer Timer3;
struct GenevaLCDDevice *Display;
struct Numpad *NumberPad;

int voltageMeasurements = 0;
float voltageAccum = 0;
float frequency = 0;
int freqCounts = 0;
extern double timeElapsed;

float targetRPM = 50;

EDFToDo schedulerTasks;
dispState displayState = SUCCESS;

// FLAGS
int calcVoltFlag = 1; // Set to always be 1 to calculate voltage so long as the deadline is met
int calcFreqFlag = 0;
int gettingUserInputFlag = 0;
int targetSetFlag = 0;
volatile int systickFlag = 0; 


//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------