/**************************************************************************
 * Author  Daniel S.
 * Date    11/19/2025
 * File    pid.h
 * Version V1.0
 * PID Controler's header for Semester Project
 *
 **************************************************************************

//------------------------------------------------------------------------------
// Files to Include
//------------------------------------------------------------------------------

#include "stm32l476xx.h"

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
	double error; // target-actual (100/7)
	int delta;	  // (currentPos - previousPos)

	int pGain;	// 1
	int pError; // targetPos - currentPos
	int pTerm;	// error * pGain

	int iGain;	  // 0.73
	int integral; // pTerm * delta + iTerm : Clamp this value to prevent windup
	int iTerm;	  // integral * iGain

	// PID Out = pTerm  + iTerm : Clamp this value to 100> PID Out > 0

	//*-Function Pointers-*//

} PIDController;

/* Class Constructor
 Arg1 = The timer the object uses
 Arg2 = How many times it should retry commands
 Arg3 = what the onOffRatio should be
*/


double UpdatePI(PIDController*this , double targetPos, double currentPos, int delta){




}




