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


// Forward declaration 
typedef struct PIDController{
    //*-Parents-*//

	//*-Properties-*//
	int delta; // (currentPos - previousPos)

	int pGain;
	int pError; // targetPos - currentPos
	int pTerm; // error * pGain

	int dGain;
	int dError; // (pTerm - prevPTerm)/ delta : Upon changing the the target, set the prevPTerm to pTerm
	int dTerm; // dError * dGain

	int iGain;
	int integral; // pTerm * delta + iTerm : Clamp this value if need be
	int iTerm; // integral * iGain

	// PID Out = pTerm + dTerm + iTerm
	

	//*-Function Pointers-*//

}PIDController;

/* Class Constructor
 Arg1 = The timer the object uses
 Arg2 = How many times it should retry commands
 Arg3 = what the onOffRatio should be
*/
