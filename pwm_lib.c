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
    self->ccrValue = (unsigned int)(dutyCycle * (self->period - 1));
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

/*
 * @author  Daniel S.
 * @date    11/29/2025
 * @fn PWMDevice_finalizePinOutSetup
 * @brief PWMDevice_finalizePinOutSetup
 * Finalizes the Pin Output Setup for PWM functionality
 *
 * Parameters:
 * @param self Pointer to the PWMDevice instance
 * Returns:
 * @return void
 * 
 */

void PWMDevice_finalizePinOutSetup(struct PWMDevice* self){
    // Ensure the pin is set to Alternate Function mode for PWM output
    self->pinOut->MODERState = 'F';
    self->pinOut->setMODER(self->pinOut);

    self->pinOut->GPIOX->AFR[self->pinOut->pin / 8] &= ~(0xF << ((self->pinOut->pin % 8) * 4)); // Clear AFR bits
    self->pinOut->GPIOX->AFR[self->pinOut->pin / 8] |= (2 << ((self->pinOut->pin % 8) * 4)); // Set AFR to AF2 for TIMx_CHy
    self->pinOut->GPIOX->PUPDR &= ~(3UL << (self->pinOut->pin * 2)); // No pull-up/pull-down

}

/*
 * @author  Daniel S.
 * @date    11/29/2025
 * @fn PWMDevice_finalizePWMClockSetup
 * @brief PWMDevice_finalizePWMClockSetup
 * Finalizes the PWM Clock Setup for PWM functionality
 *
 * Parameters:
 * @param self Pointer to the PWMDevice instance
 * Returns:
 * @return void
 * 
 */

void PWMDevice_finalizePWMClockSetup(struct PWMDevice* self){
    // Enable PWM mode on the timer channel
    switch(self->channel){
        case 1:
            self->pwmClock->TIMX->CCMR1 |= (6 << 4);                // PWM mode 1 on CCR1
            self->pwmClock->TIMX->CCER |= TIM_CCER_CC1E;            // Enable output on channel 1
            self->pwmClock->TIMX->CCER &= ~TIM_CCER_CC1P;           // Set output polarity to active high
            self->pwmClock->TIMX->CCMR1 |= TIM_CCMR1_OC1PE;         // Enable preload for CCR1
            break;
        case 2:
            self->pwmClock->TIMX->CCMR1 |= (6 << 12);               // PWM mode 1 on CCR2
            self->pwmClock->TIMX->CCER |= TIM_CCER_CC2E;            // Enable output on channel 2
            self->pwmClock->TIMX->CCER &= ~TIM_CCER_CC2P;           // Set output polarity to active high
            self->pwmClock->TIMX->CCMR1 |= TIM_CCMR1_OC2PE;         // Enable preload for CCR2
            break;
        case 3:
            self->pwmClock->TIMX->CCMR2 |= (6 << 4);                // PWM mode 1 on CCR3
            self->pwmClock->TIMX->CCER |= TIM_CCER_CC3E;            // Enable output on channel 3
            self->pwmClock->TIMX->CCER &= ~TIM_CCER_CC3P;           // Set output polarity to active high
            self->pwmClock->TIMX->CCMR2 |= TIM_CCMR2_OC3PE;         // Enable preload for CCR3
            break;
        case 4:
            self->pwmClock->TIMX->CCMR2 |= (6 << 12);               // PWM mode 1 on CCR4
            self->pwmClock->TIMX->CCER |= TIM_CCER_CC4E;            // Enable output on channel 4
            self->pwmClock->TIMX->CCER &= ~TIM_CCER_CC4P;           // Set output polarity to active high
            self->pwmClock->TIMX->CCMR2 |= TIM_CCMR2_OC4PE;         // Enable preload for CCR4
            break;
        default:
            while(1){} // Error Catch
            break;
    }
    // Start the timer
    self->pwmClock->TIMX->CR1 |= TIM_CR1_CEN; // Enable the timer
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
 * @param channel PWM Channel
 * @param clkSpeedHz Clock Speed in Hertz
 * Returns:
 * @return Pointer to the newly created PWMDevice instance
 * 
 */

PWMDevice* PWMDevice_Create(struct IODevice* pin_out, struct GeneralPurposeTimer* pwm_clock, float dutyCycle, unsigned int CCR, unsigned int channel){
    PWMDevice* self = malloc(sizeof(PWMDevice));
    
    self->pinOut = pin_out;
    self->pwmClock = pwm_clock;
    self->dutyCycle = dutyCycle;
    self->ccrValue = CCR;
    self->channel = channel;
    self->period = (self->pwmClock->TIMX->ARR + 1); // PWM Period in counts
    
    self->updateDutyCycle = &PWMDevice_updateDutyCycle;
    self->setCCRValue = &PWMDevice_setCCR;
    self->finalizePinOutSetup = &PWMDevice_finalizePinOutSetup;
    self->finalizePWMClockSetup = &PWMDevice_finalizePWMClockSetup;

    self->updateDutyCycle(self, dutyCycle);
    self->finalizePinOutSetup(self);
    self->finalizePWMClockSetup(self);
    
    return self;
}

/*
 * @}
 */