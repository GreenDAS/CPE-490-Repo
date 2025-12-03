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
	double delta; 		// (currentPos - previousPos)

	float pGain;   		// 1
	double pError; 		// targetPos - currentPos
	double pTerm;  		// error * pGain

	float iGain;		// 0.73
	double integral; 	// pTerm * delta + iTerm : Clamp this value to prevent windup
	double iTerm;	 	// integral * iGain

	double integratorMax; // Max value for integrator windup
	double integratorMin; // Min value for integrator windup

	double previousPos; // Previous Position
	double setPoint;    // Target Position
	double currentPos;  // Current Position

	double pidOut;		// Output of the PID
	double pidMax;		// Max PID Out
	double pidMin;		// Min PID Out


	// PID Out = pTerm  + iTerm : Clamp this value to 100> PID Out > 0

	//*-Function Pointers-*//
	void (*UpdatePI)(struct PIDController* self, double targetPos, double currentPos);
	void (*ResetIntegrator)(struct PIDController* self);
	double (*PIOutToDutyCycle)(struct PIDController* self);
	void (*CurrentPosChnaged)(struct PIDController* self, double newPos);
	void (*SetPointChanged)(struct PIDController* self, double newPos);

} PIDController;

/* Class Constructor*/

PIDController *PIDController_Create(float pGain, float iGain)
{
	PIDController *self = malloc(sizeof(PIDController));

	self->pGain = pGain;
	self->iGain = iGain;

	self->error = 0;
	self->delta = 0;
	self->pError = 0;
	self->pTerm = 0;
	self->integral = 0;
	self->iTerm = 0;

	return self;
}