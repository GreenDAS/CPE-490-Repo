/**************************************************************************
 * Author  Daniel S.
 * Date    11/19/2025
 * File    pid.h
 * Version V1.0
 * PID Controler's header for Semester Project
 *
 **************************************************************************/

//------------------------------------------------------------------------------
// Files to Include
//------------------------------------------------------------------------------

#include "stm32l476xx.h"
#include "globals.h"
#include "stdlib.h"
#include "stdio.h"

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// # Classes
//------------------------------------------------------------------------------

/*---PID Controler---*/

// Class Def
typedef struct PIDController
{
	//*-Parents-*//

	//*-Properties-*//
	double error; 		// target-actual (100/7)

	float pGain;   		// 1
	double pTerm;  		// error * pGain

	float iGain;		// 0.73
	double integral; 	// error + integral: Clamp this value to prevent windup
	double iTerm;	 	// integral * iGain

	double integratorMax; // Max value for integrator windup
	double integratorMin; // Min value for integrator windup

	double previousPos; // Previous Position
	double setPoint;    // Target Position
	double currentPos;  // Current Position

	float pidOut;		// Output of the PID
	float pidMax;		// Max PID Out
	float pidMin;		// Min PID Out


	// PID Out = pTerm  + iTerm : Clamp this value to 100> PID Out > 0

	//*-Function Pointers-*//
	void (*CurrentPosChanged)(struct PIDController* self, double newPos);					// Function Pointer for when Current Position Changes
	void (*SetPointChanged)(struct PIDController* self, double newPos);						// Function Pointer for when Set Point Changes	
	void (*ResetIntegrator)(struct PIDController* self);									// Function Pointer to Reset the Integrator						
	void (*UpdatePI)(struct PIDController* self);											// Function Pointer to Update the PI Output


} PIDController;

/* Class Constructor*/

PIDController *PIDController_Create(float piMaxMin[2], float pGain, float iValues[3]);
