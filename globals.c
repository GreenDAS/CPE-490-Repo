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
#include "pwm_lib.h"

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------

// Global Vars
struct IODevice VoltReader;
struct IODevice FreqReader;
struct GeneralPurposeTimer Timer2;
struct GeneralPurposeTimer Timer4;
struct GeneralPurposeTimer Timer5;
struct GenevaLCDDevice *Display;
struct Numpad *NumberPad;

struct IODevice* Switch1; // Sets up SW1 
struct IODevice* Switch2; // Sets up SW2 
struct IODevice* SW1LED;  // Sets up LED1 
struct IODevice* SW2LED;  // Sets up LED2

struct PWMDevice* MotorPWM; // PWM Device

// Voltage Vars
int voltageMeasurements = 0;
float voltageAccum = 0;
float voltage = 0;

// Torgue
float torque = 0;

// Frequency Vars
float frequency = 0;
int freqCounts = 0;
double timeElapsed = 0.0;

float targetRPM = 50;

EDFToDo schedulerTasks;
dispState displayState = SUCCESS;

// FLAGS

    // System
        // Volatile
            volatile unsigned int systickFlag = 0; 

    // Switches

        //Volatile
            volatile unsigned int sw1PressedFlag = 0; // Sets when SW1 is pressed
            volatile unsigned int sw2PressedFlag = 1; // Sets when SW2 is pressed

    // Task
    unsigned int calcVoltFlag = 1;                       // Set to always be 1 to calculate voltage so long as the deadline is met
    unsigned int calcFreqFlag = 0;                       // Off by default
    unsigned int gettingUserInputFlag = 1;               // 1 = getting user input by default
    unsigned int readFinishedFlag = 0;                   // 0 = read not finished yet
    unsigned int targetSetFlag = 0;                      // 0 = no new target RPM set yet
    unsigned int newRPMFlag = 0;                         // 0 = no new RPM measured yet
    unsigned int updateMainMenuFlag = 0;                 // 0 = no need to update main menu
        //Volatile
            // this was created by caleb
            volatile unsigned int tor_rpm_toggle = 0; // 0 for RPMs(target and actual), 1 for (Target and torque) only
//
//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------