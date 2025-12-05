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
#include "pid.h"

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// # Classes
//------------------------------------------------------------------------------



// --- Class Methods --- //
/* @addtogroup PIDController_Class
 * @{
 */


/*
 * @author  Daniel S.
 * @date    11/19/2025
 * @fn PID_CurrentPosChnaged
 * @brief PID_CurrentPosChnaged
 * Function to be called when Current Position Changes
 *
 * Parameters:
 * @param self Pointer to the PIDController instance
 * @param newPos New Current Position
 * Returns:
 * @return void
 * 
 */

void PID_CurrentPosChnaged(PIDController* self, double newPos){
    self->previousPos = self->currentPos;
    self->currentPos = newPos;
    self->error = (self->setPoint - self->currentPos) * 100.0 / 7.0; // Scale error to range [0, 100]
    self->PID_UpdatePIOut(self);
}


/*
 * @author  Daniel S.
 * @date    12/3/2025
 * @fn PID_SetPointChanged
 * @brief PID_SetPointChanged
 * Function to be called when Set Point Changes
 *
 * Parameters:
 * @param self Pointer to the PIDController instance
 * @param newPos New Set Point
 * Returns:
 * @return void
 * 
 */

void PID_SetPointChanged(PIDController* self, double newPos){
    self->ResetIntegrator(self);
    self->setPoint = newPos;
    self->error = (self->setPoint - self->currentPos) * 100.0 / 7.0; // Scale error to range [0, 100]
    self->PID_UpdatePIOut(self);
}


/*
 * @author  Daniel S.
 * @date    12/3/2025
 * @fn PID_ResetIntegrator
 * @brief PID_ResetIntegrator
 * Resets the Integrator term of the PID Controller
 *
 * Parameters:
 * @param self Pointer to the PIDController instance
 * Returns:
 * @return void
 * 
 */
void PID_ResetIntegrator(PIDController* self){
    self->integral = 0.0;
    self->iTerm = 0.0;
}


/*
 * @author  Daniel S.
 * @date    12/3/2025
 * @fn PID_UpdatePIOut
 * @brief PID_UpdatePIOut
 * Updates the PI Output of the PID Controller
 *
 * Parameters:
 * @param self Pointer to the PIDController instance
 * Returns:
 * @return void
 * 
 */
void PID_UpdatePIOut(PIDController* self){
    // Proportional term
    self->pTerm = self->error * self->pGain;

    // Integral term
    self->integral += self->error;
    // Clamp integral to prevent windup
    if (self->integral > self->integratorMax) {
        self->integral = self->integratorMax;
    } else if (self->integral < self->integratorMin) {
        self->integral = self->integratorMin;
    }
    self->iTerm = self->integral * self->iGain;

    // Calculate total PID output
    double pidOut = self->pTerm + self->iTerm;

    // Clamp the PID output to the range [0, 100]
    if (pidOut > self->pidMax) {
        pidOut = self->pidMax;
    } else if (pidOut < self->pidMin) {
        pidOut = self->pidMin;
    }

    self->pidOut = pidOut;
}

/*
 * @author  Daniel S.
 * @date    11/19/2025
 * @fn PIDController_Create
 * @brief PIDController_Create
 * Creates a new PIDController instance
 *
 * Parameters:
 * @param piMaxMin Array containing [pidMax, pidMin]
 * @param pGain Proportional Gain
 * @param iValues Array containing [iGain, integratorMax, integratorMin]
 * Returns:
 * @return Pointer to the newly created PIDController instance
 *
 */
PIDController *PIDController_Create(float[2] piMaxMin, float pGain, float[3] iValues)
{
	PIDController *self = malloc(sizeof(PIDController));

    self->pidMax = piMaxMin[0];
    self->pidMin = piMaxMin[1];

	self->pGain = pGain;

	self->iGain = iValues[0];
    self->integratorMax = iValues[1];
    self->integratorMin = iValues[2];

	self->error = 0;
	self->delta = 0;
	self->pError = 0;
	self->pTerm = 0;
	self->integral = 0;
	self->iTerm = 0;
    self->previousPos = 0;
    self->setPoint = 0;
    self->currentPos = 0;
    self->pidOut = 0;

    // Assign Function Pointers
    self->CurrentPosChnaged = PID_CurrentPosChnaged;
    self->SetPointChanged = PID_SetPointChanged;
    self->ResetIntegrator = PID_ResetIntegrator;
    self->UpdatePI = PID_UpdatePIOut;


	return self;
}




