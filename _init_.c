/**************************************************************************
 * Author  Daniel S.
 * Date    11/13/2025
 * File    _init_.c
 * Version V1.5
 * Description 
 * INIT
 *
 *************************************************************************/
//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------

#include "stm32l476xx.h"
#include "globals.h"
#include "_init_.h"
#include "gpio_lib.h"
#include "timer_lib.h"
#include "stdlib.h"
#include "lcd_lib.h"
#include "string.h"
#include "string_lib.h"
#include "lcd_routines.h"

//------------------------------------------------------------------------------
// # defines
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

// NUMPAD SETUP //
Numpad* CreateNumpad(){
	// Start Setting Up Numpad
	static int NumpadValues[4][3] = {
		{ 1, 2, 3},
		{ 4, 5, 6},
		{ 7, 8, 9},
		{10, 0,11}
	};
	
	IODevice** rowIOP = malloc(4 * sizeof(IODevice*));
	IODevice** colIOP = malloc(3 * sizeof(IODevice*));
	
	rowIOP[0] = IODevice_Create('A', ROW1, 0, 1, 'I');
	rowIOP[1] = IODevice_Create('A', ROW2, 0, 1, 'I');
	rowIOP[2] = IODevice_Create('A', ROW3, 0, 1, 'I');
	rowIOP[3] = IODevice_Create('A', ROW4, 0, 1, 'I');

	colIOP[0] = IODevice_Create('B', COL1, 0, 1, 'O');
	colIOP[1] = IODevice_Create('B', COL2, 0, 1, 'O');
	colIOP[2] = IODevice_Create('B', COL3, 0, 1, 'O');

	return Numpad_Create(&NumpadValues[0][0], rowIOP, colIOP, 4, 3);
	
}


// TIMER SETUP //
/* Sets up the Systick Timer
 Arg1 = The load value for the SysTick Timer
 Arg2 = 1 to enable interrupts, 0 to disable interrupts
*/
void InitSysTick(int load, int enableInterrupt){
	/*	4MHz Processor Clock,  */
	// SysTick Frequency = Processor Clock / (LOAD + 1)
	// For 1ms SysTick Interrupts: LOAD = (4,000,000 / 1000) - 1 = 3999
	SysTick->LOAD = load; // Tells the Systick Timer to a max of 0xFFFFFF


	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;     // Enable SysTick
	SysTick->CTRL |= enableInterrupt == 1 ? SysTick_CTRL_TICKINT_Msk : 0;    // Enable SysTick interrupt
	SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;  // Use processor clock

	// Set SysTick interrupt priority (optional, but recommended)
	NVIC_SetPriority(SysTick_IRQn, 0); // 0 = highest priority
}



//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

void _init_(){

	MotorPWM = PWMDevice_Create(
		IODevice_Create('B', PWM, 0, 1, 'F'), // PWM Output Pin
		/* Timer 3
		* Prescale = 1
		* Count Speed = 1 count every 0.5us
		* ARR = 199
		* => Timer Period = (PSC + 1) * (ARR + 1) / Clock Speed
		* => Timer Period = (1 + 1) * (199 + 1) / 4,000,000
		* => Timer Period = 0.0001s or 100us
		* Duty Cycle Calculation:
		* Duty Cycle = (CCR / (ARR + 1))
		* => CCR = (Duty Cycle * (ARR + 1))
		*/
		GeneralPurposeTimer_Create(3,0,1,199,'U',0), // Sets up Timer3 for PWM Use
		0.0,                                 // Initial Duty Cycle
		0                                    // Initial CCR Value
	); // Sets up PWM Device


	Timer4 = *GeneralPurposeTimer_Create(4,0,CountAtMilSecondRate,0,'D',1); // Sets up Timer4 for One Pulse Mode Use

	Timer5 = *GeneralPurposeTimer_Create(5,1,CountAtMilSecondRate,(TimerPeriod1SecondInMilSeconds - 1)*10,'D',0); // Sets up Timer3 for GP Timer Use & for the Display

	unsigned char msg[GenevaLCDRowSize][(GenevaLCDColSize+1)] = {// The Message to Display
			//1st Row
		{'V', 'O', 'L', 'T', 'A', 'G', 'E', ':',
			' ', '0', '0', '.', '0', '0', 'V', 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		//2nd Row
		{'F', 'R', 'E', 'Q', ':', ' ', '0', '0',
			'0', '0', '.', '0', '0', 'H', 'z', 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

	};

	Display = GenevaLCDDevice_Create(&Timer5, 5, 10, msg); // Sets up LCD Display

	NumberPad = CreateNumpad();
	NumberPad->changeDimMODER(NumberPad, 'C', 'O'); // Sets Columns to Output
	for(int j=0; j < NumberPad->colSize; j++){NumberPad->colIO[j]->setState(NumberPad->colIO[j],1);} // Sets the Col to on

	// Setting Up SW1-2 and LED1-2
	Switch1 = IODevice_Create('C',SW1,0,1,'I'); // Sets up SW1
	Switch1->initInterupt(Switch1->pin,Switch1->GPIOchar,EXTI4_IRQn,0,4); // Sets up EXTI4 for SW1
	Switch2 = IODevice_Create('C',SW2,0,1,'I'); // Sets up SW2 
	Switch2->initInterupt(Switch2->pin,Switch2->GPIOchar,EXTI9_5_IRQn,0,5); // Sets up EXTI9_5 for SW2
	SW1LED = IODevice_Create('C',LED1,0,1,'O'); // Sets up LED1 
	SW2LED = IODevice_Create('C',LED2,0,1,'O'); // Sets up LED2 


	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN; // Enable ADC Clock
	if ((ADC1->CR & ADC_CR_DEEPPWD) == ADC_CR_DEEPPWD){ADC1->CR &= ~ADC_CR_DEEPPWD;} // Wake up ADC from Deep Power Down
	ADC1->CR |= ADC_CR_ADVREGEN; // Enable ADC Voltage Regulator
	Timer5.greedyWait(&Timer5, 1, MilSecondsScalar); // Wait for ADC Voltage Regulator to start up (min 10us)

	VoltReader = *IODevice_Create('A', 0, 1, 0, 'A'); // Sets up VoltReader ADC Pin (PA0)
	VoltReader.GPIOX->ASCR |= (1UL); // Enable Analog Switch for PA0

	ADC1->SQR1 |= 0x140UL; // Set ADC to use channel 0 (PA0) as 1st conversion
	ADC123_COMMON->CCR &= ~(0xFUL); // Divide by 1 for ADC clock
	ADC123_COMMON->CCR &= ~(0x30000UL); // Setup the ADC1 timer so that it will take the HCLK w/ a divide by 1
	ADC123_COMMON->CCR |= (0x10000UL);
	ADC1->CFGR |= (0x2UL<<3); // Set ADC to 8 bit resolution
	ADC1->SMPR1 |= (1<<15); // Set ADC Channel 0 Sample time to 6.5 cycles
	ADC1->ISR |= ADC_ISR_ADRDY; // Clear ADC Ready flag
	ADC1->CR |= ADC_CR_ADEN; // Enable ADC
	while((ADC1->ISR & ADC_ISR_ADRDY) == 0){} // Wait for ADC to be ready
	// Start New Conversion ( Primes it for RTOS )
	ADC1->ISR |= ADC_ISR_EOC; // Clear End of Conversion Flag
	ADC1->CR |= ADC_CR_ADSTART; // Start ADC Conversion

	// Sets up Frequency Reader need to use a different pin than Volt Reader (PA0)
	FreqReader = *IODevice_Create('A',5,0,1,'F');
	FreqReader.initInterupt(FreqReader.pin,FreqReader.GPIOchar,EXTI9_5_IRQn,1,4);

	Timer2 = *GeneralPurposeTimer_Create(2,1,1,0xFFFFFFFF - 1,'D',0); // Sets up Timer2 to run as fast as possible for CC Interrupt
	Timer2.InteruptHandler = PeripheralInteruptHandling_Create(TIM2_IRQn);
	Timer2.InteruptHandler->setPriorityBit(Timer2.InteruptHandler,0b0100); // Sets the Priority Bit's preemption priority to 1, sub priority to 0
	Timer2.InteruptHandler->setIXER(Timer2.InteruptHandler,'S'); // Enables the interupt in the NVIC
	Timer2.InteruptHandler->initCCInterupt(Timer2.TIMX);
	Timer2.setBits(&(Timer2.TIMX->CR1),0,1);

	InitSysTick(3999, 1); // Sets up SysTick for 1ms interrupts with interrupt enabled

}