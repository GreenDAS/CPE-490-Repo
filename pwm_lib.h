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

/*
 * @addtogroup PWMDevice_Class
 * @{
 */

/*
 * @author  Daniel S.
 * @date    11/29/2025
 * @struct PWMDevice
 * @brief PWMDevice
 * PWM Device Class Definition
 *
 * Members:
 * @var pinOut Pointer to the IODevice for PWM output
 * @var pwmClock Pointer to the GeneralPurposeTimer used for PWM timing
 * @var dutyCycle Duty cycle of the PWM signal (0.0 to 1.0)
 * @var ccrValue Capture/Compare Register Value
 * @var period PWM Period in counts
 * @var channel PWM Channel
 * 
 * Function Pointers:
 * @var updateDutyCycle Function to update the duty cycle
 * @var setCCRValue Function to set the CCR value
 */
typedef struct PWMDevice{
	// --- Parents --- //
	struct IODevice* pinOut;
	struct GeneralPurposeTimer* pwmClock;
	// --- Properties --- //
	float dutyCycle;
	unsigned int ccrValue;						// Capture/Compare Register Value
	unsigned int period;						// PWM Period in counts
	unsigned int channel; 						// PWM Channel
	
	
	// --- Fn Pointers --- //
	void (*updateDutyCycle)(struct PWMDevice* self, float dutyCycle);
	void (*setCCRValue)(struct PWMDevice* self, unsigned int ccrValue);
	void (*finalizePinOutSetup)(struct PWMDevice* self);
	void (*finalizePWMClockSetup)(struct PWMDevice* self);
	
}PWMDevice;

/*
 * @author  Daniel S.
 * @date    11/29/2025
 * @fn createPWMDevice
 * @brief createPWMDevice
 * Creates a new PWMDevice instance
 *
 * Parameters:
 * @param pin_out Pointer to the IODevice for PWM output
 * @param pwm_clock Pointer to the GeneralPurposeTimer used for PWM timing
 * @param dutyCycle Initial duty cycle value (0.0 to 1.0)
 * @param ccrValue Initial CCR value
 * @param channel PWM Channel
 * @param clkSpeedHz Clock Speed in Hertz
 * Returns:
 * @return Pointer to the created PWMDevice instance
 *
*/
PWMDevice* PWMDevice_Create(struct IODevice* pin_out, struct GeneralPurposeTimer* pwm_clock, float dutyCycle, unsigned int CCR, unsigned int channel, float clkSpeedHz);

/**
 * @}
 */