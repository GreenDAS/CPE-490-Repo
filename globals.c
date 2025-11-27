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
IODevice VoltReader;
IODevice FreqReader;
GeneralPurposeTimer Timer2;
GeneralPurposeTimer Timer3;
GenevaLCDDevice *Display;
int voltageMeasurements = 0;
float voltageAccum = 0;
float frequency = 0;
int freqCounts = 0;
extern double timeElapsed;

EDFToDo schedulerTasks;
dispState displayState = SUCCESS;

// FLAGS
int calcVoltFlag = 1; // Set to always be 1 to calculate voltage so long as the deadline is met
int calcFreqFlag = 0;

// this was created by caleb
int tor_rpm_toggle = 0; // 0 for RPMs(target and actual), 1 for (Target and torque) only
int togglestate = 0;    // 0 for first string 1 for second // this is for what value to put in

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------