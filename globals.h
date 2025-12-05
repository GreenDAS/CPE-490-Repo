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

// Boolean Defines
    #define TRUE                1
    #define FALSE               0
//

/* Pin assignments*/
    // GPIOA
        #define VOLTAGE_IN          0   // Voltage Input Pin (ADC1_IN5)
        #define FREQ_IN             5   // Frequency Input Pin (TIM2_CH2)
        #define ROW1                8   // Numpad Row 1
        #define ROW2                9   // Numpad Row 2
        #define ROW3                10  // Numpad Row 3
        #define ROW4                11  // Numpad Row 4
    //

    // GPIOB
        #define PWM                 0   // PWM Output Pin
        #define COL1                8   // Numpad Column 1
        #define COL2                9   // Numpad Column 2
        #define COL3                10  // Numpad Column 3
    //

    // GPIOC
        #define LED1                0   // SW1 LED
        #define LED2                1   // SW2 LED
        #define LED3                2   // SW3 LED *Unused*
        #define LED4                3   // SW4 LED *Unused*
        #define SW1                 4   // Switch 1
        #define SW2                 5   // Switch 2
    //

// Scheduler Config
    #define MAX_TASKS           11                  // Maximum number of tasks for EDF Scheduler
    #define SYSTICK_MAX         1200                // SysTick max value ( Doesn't need to change unless a deadline is longer than this )
    // Deadlines
        #define VOLTAGE_DEADLINE    400             // 400ms
        #define FREQ_DEADLINE       15              // 15ms
        #define DISPLAY_DEADLINE    600             // 600ms
        #define READ_NUMPAD_DEADLINE 25             // 25ms
        #define HANDLE_NUMPAD_PRESS_DEADLINE 100    // 100ms
        #define HANDLE_SW1_PRESS_DEADLINE 10        // 10ms
        #define HANDLE_SW2_PRESS_DEADLINE 10        // 10ms
        #define MAIN_MENU_UPDATE_DEADLINE 50        // 50ms
        #define NEW_RPM_DEADLINE    25              // 25ms
        #define NEW_TARGET_DEADLINE 25              // 25ms
        #define HANDLE_NO_ROTATION_DEADLINE 25      // 25ms
    //
//

// RPM Bounds
    #define RPM_UPPER           200             // Upper RPM Limit of 200 RPM
    #define RPM_LOWER           25              // Lower RPM Limit of 25 RPM
//

// PID Config
    #define NO_ROTATION_WAIT_TIME   2450        // How long Timer 5 should wait to check if the motor is off
//

// PWM Config
    #define PERIOD              199             // How long the pwm period is (X + 1)
    #define PWM_PSC             1               // PSC + 1   
    #define DELTA_TIME          (PERIOD + 1) / (clockSpeedHz / (PWM_PSC + 1))   // How long in 
//

/* -------------------------------------------------------------------------
 * Type Definitions
 * ------------------------------------------------------------------------- */

 // Display States for handling display updates
    typedef enum {
        START,          // Start Talking to Display
        UPPER,          // Send Upper 8 Bits of Data
        LOWER,          // Send Lower 8 Bits of Data
        SUCCESS         // Successful Transmission
    } dispState;


/* EDF scheduler task table */
    typedef struct {
        void (*tasks[MAX_TASKS])(void);         // Task function pointers
        int  deadlines[MAX_TASKS];              // Deadlines for each task
        int  cooldowns[MAX_TASKS];              // Cooldown counters for each task
        int  clksWaited[MAX_TASKS];             // Clocks waited for each task
        int  (*taskCond[MAX_TASKS])(void);      // Condition functions to check if task is ready
        int  (*coolDownFn[MAX_TASKS])(void);    // Cooldown functions for each task
    } EDFToDo;

/* Forward declarations – NEVER include full structs here */
struct IODevice;
struct GeneralPurposeTimer;
struct GenevaLCDDevice;
struct Numpad;
struct PWMDevice;
struct PIDController;

/* -------------------------------------------------------------------------
 * Global Variables – extern declarations only
 * -------------------------------------------------------------------------*/
extern struct IODevice         VoltReader;      // Used to read voltage input
extern struct IODevice         FreqReader;      // Used to read frequency input
extern struct GeneralPurposeTimer Timer2;       // Timer2 for frequency measurements
extern struct GeneralPurposeTimer Timer4;       // Timer4 for general purpose use - Init in One Pulse Mode
extern struct GeneralPurposeTimer Timer5;       // Timer5 for Display use
extern struct GenevaLCDDevice  *Display;        // LCD Display
extern struct Numpad           *NumberPad;      // Numpad Device
extern struct PIDController    *MotorPID;       // PID Controller for motor control

extern struct IODevice*        Switch1;         // Sets up SW1 
extern struct IODevice*        Switch2;         // Sets up SW2 
extern struct IODevice*        SW1LED;          // Sets up LED1 
extern struct IODevice*        SW2LED;          // Sets up LED2

extern struct PWMDevice* MotorPWM;              // PWM Device for motor control

// Voltage Vars
extern int          voltageMeasurements;        // Number of voltage measurements taken
extern float        voltageAccum;               // Accumulated voltage for averaging
extern float        voltage;                    // Calculated voltage

// Torgue
extern float        torque;                     // Calculated torque

// Frequency Vars
extern double       frequency;                  // Measured frequency
extern double       prevFrequency;              // Previous frequency value
extern int          freqCounts;                 // Frequency counts
extern double       timeElapsed;                // Time elapsed for frequency measurement

extern float            targetRPM;              // Target RPM value

extern EDFToDo          schedulerTasks;         // EDF Scheduler Tasks
extern dispState      	displayState;           // Current display state

// FLAGS

    // System
        // Volatile
            extern volatile unsigned int systickFlag;       // SysTick Flag

    // Switches

        //Volatile
            extern volatile unsigned int sw1PressedFlag;    // Sets when SW1 is pressed
            extern volatile unsigned int sw2PressedFlag;    // Sets when SW2 is pressed

    // Task
    extern unsigned int calcVoltFlag;                       // Set to always be 1 to calculate voltage so long as the deadline is met
    extern unsigned int calcFreqFlag;                       // Off by default
    extern unsigned int gettingUserInputFlag;               // 1 = getting user input by default
    extern unsigned int readFinishedFlag;                   // 0 = read not finished yet
    extern unsigned int targetSetFlag;                      // 0 = no new target RPM set yet
    extern unsigned int newRPMFlag;                         // 0 = no new RPM measured yet
    extern unsigned int updateMainMenuFlag;                 // 0 = no need to update main menu
        //Volatile
            // this was created by caleb
            extern volatile unsigned int tor_rpm_toggle;    // 0 for RPMs(target and actual), 1 for (Target and torque) only
    
/* -------------------------------------------------------------------------
 * End of globals.h
 * -------------------------------------------------------------------------*/
