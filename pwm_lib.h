/**************************************************************************
 * Author  Daniel S.
 * Date    11/29/2025
 * File    pwm_lib.c
 * Description 
 * PWM Class
 *
 *************************************************************************/
 
//------------------------------------------------------------------------------
// Defines 
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Temp Defs 
//------------------------------------------------------------------------------

struct IODevice;
struct GeneralPurposeTimer;
	
//------------------------------------------------------------------------------
// PWM Class
//------------------------------------------------------------------------------

typedef struct PWMDevice{
	// --- Parents --- //
	struct IODevice* pinOut;
	struct GeneralPurposeTimer* pwmClock;
	// --- Properties --- //
	float dutyCycle;
	unsigned int CCR;
	
	
	// --- Fn Pointers --- //
	void (*updateDutyCycle)(struct PWMDevice* self, float dutyCycle);
	void (*setCCR)(struct PWMDevice* self, unsigned int CCR);
	
}PWMDevice;

// Assumes pinOut and pwmClock are setup properly
PWMDevice* createPWMDevice(struct IODevice* pin_out, struct GeneralPurposeTimer* pwm_clock, float dutyCycle, unsigned int CCR);