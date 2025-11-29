/**************************************************************************
 * Author  Daniel S.
 * Date    11/29/2025
 * File    pwm_lib.c
 * Description 
 * PWM Class
 *
 *************************************************************************/
 
//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "pwm_lib.h"
#include "gpio_lib.h"
#include "timer_lib.h"
#include "stm32l476xx.h"
#include "stdlib.h"

//------------------------------------------------------------------------------
// PWM Class
//------------------------------------------------------------------------------


// --- Class Methods --- //

/* @addtogroup PWMDevice_Class
 * @{
 */

/*
 * @author  Daniel S.
 * @date    11/29/2025
 * @fn PWMDevice_updateDutyCycle
 * @brief PWMDevice_updateDutyCycle
 * Updates the Duty Cycle of the PWM Device
 * Also updates the CCR value & register based on the new duty cycle
 *
 * Parameters:
 * @param self Pointer to the PWMDevice instance
 * @param dutyCycle New duty cycle value (0.0 to 1.0)
 * Returns:
 * @return void
 * 
 */

void PWMDevice_updateDutyCycle(struct PWMDevice* self, float dutyCycle){
    self->dutyCycle = dutyCycle;
    self->ccrValue = (unsigned int)(dutyCycle * self->period);
    switch(self->channel){
        case 1:
            self->pwmClock->TIMX->CCR1 = self->ccrValue;
            break;
        case 2:
            self->pwmClock->TIMX->CCR2 = self->ccrValue;
            break;
        case 3:
            self->pwmClock->TIMX->CCR3 = self->ccrValue;
            break;
        case 4:
            self->pwmClock->TIMX->CCR4 = self->ccrValue;
            break;
        default:
            // Handle invalid channel if necessary
            while(1){} // Error Catch
            break;
    }
}


/*
 * @author  Daniel S.
 * @date    11/29/2025
 * @fn PWMDevice_setCCR
 * @brief PWMDevice_setCCR
 * Sets the CCR Value & Register of the PWM Device
 * Also updates the duty cycle based on the new CCR value
 *
 * Parameters:
 * @param self Pointer to the PWMDevice instance
 * @param ccrValue New CCR value
 * Returns:
 * @return void
 * 
 */
void PWMDevice_setCCR(struct PWMDevice* self, unsigned int ccrValue){
    self->ccrValue = ccrValue;
    self->dutyCycle = (float)ccrValue / self->period;
    switch(self->channel){
        case 1:
            self->pwmClock->TIMX->CCR1 = self->ccrValue;
            break;
        case 2:
            self->pwmClock->TIMX->CCR2 = self->ccrValue;
            break;
        case 3:
            self->pwmClock->TIMX->CCR3 = self->ccrValue;
            break;
        case 4:
            self->pwmClock->TIMX->CCR4 = self->ccrValue;
            break;
        default:
            // Handle invalid channel if necessary
            while(1){} // Error Catch
            break;
    }
}

// --- Class Constructor --- //

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
 * @param CCR Initial CCR value
 * Returns:
 * @return Pointer to the newly created PWMDevice instance
 * 
 */

PWMDevice* createPWMDevice(struct IODevice* pin_out, struct GeneralPurposeTimer* pwm_clock, float dutyCycle, unsigned int CCR){
    PWMDevice* self = malloc(sizeof(PWMDevice));
    
    self->pinOut = pin_out;
    self->pwmClock = pwm_clock;
    self->dutyCycle = dutyCycle;
    self->ccrValue = CCR;
    self->period = (self->pwmClock->PSC + 1) * (self->pwmClock->TIMX->ARR + 1) / clockSpeedHz;
    
    self->updateDutyCycle = &PWMDevice_updateDutyCycle;
    self->setCCRValue = &PWMDevice_setCCR;
    
    return self;
}

/*
 * @}
 */