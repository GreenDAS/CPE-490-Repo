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





void PID_ResetIntegrator(PIDController* self){
    self->integral = 0.0;
    self->iTerm = 0.0;
}

void PID_UpdatePIOut(PIDController* self, double targetPos, double currentPos){
    // Calculate error
    self->error = (targetPos - currentPos) * 100.0 / 7.0; // Scale error to range [0, 100]

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

double PID_PIOutToDutyCycle(PIDController* self){
    double pidOut = self->pTerm + self->iTerm;
    // Clamp the PID output to the range [0, 100]
    if (pidOut > 100.0) {
        pidOut = 100.0;
    } else if (pidOut < 0.0) {
        pidOut = 0.0;
    }
    return pidOut;
}

/* Class Constructor
 Arg1 = The timer the object uses
 Arg2 = How many times it should retry commands
 Arg3 = what the onOffRatio should be
*/






