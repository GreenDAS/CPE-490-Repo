/**************************************************************************
 * Author  Ian Fillinager
 * Date    August 18, 2025
 * File    lcd_routines.c
 * Version V1.0
 * Description
 * Function for the LCD Display on the Geneva Hat
 *
 **************************************************************************
*/
//------------------------------------------------------------------------------
// Files to Include 
//------------------------------------------------------------------------------
#include "stm32l476xx.h"
#include "lcd_routines.h"
//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Timer 6 delay - input a long preload to achieve a delay of up to 262ms. No interrupts are active. It will hold the processor until the delay is complete.

void tim6_Del (int preload) {
	
	TIM6->CNT = (TIM6->CNT & TIME_MASK) | preload;									//Load the preload into the counter.
	TIM6->CR1 |= TIM67_CR1_START;																		//Start the delay.

	while ( (TIM6->CR1 & TIME_STOP_CHK) == TIME_STOP_CHK) {}				//Wait until the CR1 register indicates that the delay is over. NOTE: TIME_STOP_CHK acts as both a mask and a bit check.	
	
}



//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Timer 6 setup rouitine - call in the LCD Init routine to set-up Timer 6 for delays required for LCD function (see datasheet).

void tim6_Init (void) {
	
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM6EN;									//Enable the clock to see Timer 6.
	TIM6->CR1 = TIM6_CR1_SETUP;														//Disable TIM6 when an overflow occurs. This will clear the TIM6 CEN bit.
	TIM6->PSC = TIM6_PSC_SETUP;														//Apply a prescaler to Timer 6.
	TIM6->CNT = (TIM6->CNT & TIME_MASK) | 0x0000FFFE;			//Run the timer once to setup the counter for the first use of the routine.
	TIM6->CR1 |= TIM67_CR1_START;													//Enable the CEN bit to start the timer. This bit will be cleared when the timer overflows.
	
}


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//I2C1 Initialization routine - called by the lcd_Init routine, but provided here for separate use. 
//This routine will initialize the I2C1 peripheral for operation as a sole master (follow flowchart on pg. 1280 of the RMO351 Reference Manual)

void i2c1_Init (void) {
	
	RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;		//Allow the clock to see I2C Channel 1 (hereby I2C1)
	I2C1->CR1 &= I2C_PER_DISABLE;						//Make sure the peripheral is disabled for setup
	I2C1->CR1 = I2C1_CR1_SETUP;							//Setup I2C1 channel
	I2C1->TIMINGR = I2C1_TIM_SETUP;					//Configure I2C1 timing characteristics
	I2C1->CR1 |= I2C1_CR1_STRETCH;					//Configure I2C1 clock stretch parameters
	I2C1->CR1 |= I2C_PER_ENABLE;						//After configuration, re-enable the peripheral

}	


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//I2C1 Pin Initialization routine - called by the lcd_Init routine, but provided here for separate use.
//This routine will setup GPIOB 6 and 7 to use peripheral I2C1. PB7 will be I2C1 SDA and PB6 will be I2C1 SCL.

void i2c1_Pinmode_PB67 (void) {
	
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;		//Enable the clock to see GPIOB for I2C1 use.
	
//IO PORTB Initial setup for I2C1 Operation
//PB6: Mode -> Alternate Function, Output Type -> Open-Drain;;, Output Speed -> Medium;;, Internal PU/PD Resisters -> None (default), Function -> I2C1_SCL (AF4)
//PB7: Mode -> Alternate Function, Output Type -> Open-Drain;;, Output Speed -> Medium;;, Internal PU/PD Resisters -> None (default), Function -> I2C1_SDA (AF4)

	GPIOB->AFR[0] |= GPIOB_76_ALT_FUNCT;		//Select I2C1 as the alternate function for PB6 and PB7
	GPIOB->MODER &= GPIOB_76_MODER;					//Select the alternate function GPIO configuration option for PB6 and PB7
	GPIOB->OTYPER |=GPIOB_76_OTYPER;				//Setup PB6 and PB7 as open drain I/O. NOTE: Not sure this in entirely necessary; I2C needs open drain pins, and may do it for you.
	GPIOB->OSPEEDR |= GPIOB_76_OSPEEDR;			//Set PB6 and PB7 output speed to "medium". NOTE: Likely uneccesary, but additional experimentation is still required to confirm.
	
}


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// LCD Command routine - execute a command on the LCD with (5) or less retries. See datasheet.
// This function will return 0x00 if the transmission is successful.
// This function will retry up to 5 times if a message is NACKed. If the transmission is not successful, it will end the transmission and return 0x01.
// NOTE: When a NACK is received by the Master-transmitter, a STOP is immediately generated by hardware afterwards. (pg. 1296)

int lcd_Command (int command_Code, int number_retries) {
	
	int message[] = {LCD_CTRL_COMMAND,command_Code};		//Command message
	int error;																					//Error flag indicating the communication should restart or terminate	
	int working;																				//working variable
	int i;																							//Iterative variable
	int retry = number_retries;													//Number of retries
	
	if (retry > 5 || retry < 1) {												//Put boundaries on the number of retries
		retry = 5;}		
		
	do {
//Send the address of the LCD. Try until a NACK is not recieved
		if(retry == 0) {																	//Decrement the retry variable each transmission time.
			break;}																					//Or, if retry is 0, return an error code.
		else {
			retry--;}
		
		error = 0;
		
		I2C1->ICR = I2C_NACKCF;														//Clear the NACK flag.
				
		working = I2C1->CR2;															//Get current value of I2C1_CR2
		working &=I2C_CR2_WRITE0;													//Clear NBYTES in CR2 to prepare a new transmission.
		working |= I2C_CR2_WRITE2;												//Prepare to write 2-bytes of data to the LCD.
		working |= (LCD_ADDRESS << 1);										//Load I2C1 with the LCD address. Shift it by one for the 7-bit addressing mode (See reference pg. 1328)
		working |= I2C_START;															//Start transmission, seeking response from LCD.
			
		I2C1->CR2 = working;
			
		tim6_Del (TIME_100us);												//Delay 100us for the LCD
								
		if ((I2C1->ISR & I2C_NACKF) == I2C_NACKF) {
			error=1;
			continue;}																			//If a nack is recieved, restart the process.
	
//Send the first message byte to the LCD, loop to send the second. Restart communication if a NACK is recieved.
	
	for(i=0; i<2; i++) {	
			error=0;
			I2C1->TXDR = message[i];												//Tell the LCD that a data value is going to be sent
			
			tim6_Del (TIME_100us);													//Delay 100us for LCD operation
			
			if ((I2C1->ISR & I2C_NACKF) == I2C_NACKF) {
				error=1;}																			//If a nack is recieved, restart the process.	
			}
	}	while (error);
	
	if (retry == 0) {																		//If retry == 0, generate an error bit.
		return 1;}
	else {
		return 0;}
}



//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// LCD Initialization routine - calls all functions necessary to initialize the LCD, I2C1, and TIM6 units.
// This function will return 0x00 if the transmission is successful.
// This function will retry up to 5 times if a message is NACKed. If the transmission is not successful, it will end the transmission and return 0x01.
// If the I2C1 channel is busy when called, transmission will not start. Instead, 0x02 will be returned.
// NOTE: When a NACK is received by the Master-transmitter, a STOP is immediately generated by hardware afterwards. (pg. 1296)

int lcd_Init (int number_retries) {
	int lcd_Commands [] = {LCD_CTRL_COMMAND,LCD_FUNCTION_SET,LCD_DISP_ON,LCD_CLEAR,LCD_ENTRY_MODE,LCD_CURSOR_SET};			//List of commands for the LCD
	int delay_Values [] = {TIME_100us,TIME_100us,TIME_100us,TIME_10ms,TIME_100us,TIME_100us};														//List of recommended delays for LCD function
	int i;																					//Iteration variable for use in for loops
	int error;																			//Error flag indicating the communication should restart or terminate	
	int message_length = 6;													//Number of bytes to be sent
	int working;																		//working variable
	int retry = number_retries;											//Tracks number of retries left to send information to the LCD
	
	if (retry > 5 || retry < 1) {										//Put boundaries on the number of retries
		retry = 5;}	
		
	tim6_Init ();																		//Initialize Timer 6
	i2c1_Pinmode_PB67 ();														//Initialize I2C1 peripheral and ports
	i2c1_Init ();
		
	tim6_Del (TIME_40ms);														//Wait 40ms after power-on for the LCD to become operable
	
	do {
//Send the address of the LCD. Try until a NACK is not recieved
		if(retry == 0) {															//Decrement the retry variable each transmission time.
			break;}																			//Or, if retry is 0, return an error code.
		else {
			retry--;}
		
		error=0;
		I2C1->ICR = I2C_NACKCF;												//Clear the NACK flag.
		
		working = I2C1->CR2;													//Get current value of I2C1_CR2
		working &= I2C_CR2_WRITE0;										//Clear NBYTES in CR2 to prepare a new transmission.
		working |= I2C_CR2_WRITE6;										//Prepare to write 2-bytes of data to the LCD.
		working |= (LCD_ADDRESS << 1);								//Load I2C1 with the LCD address. Shift it by one for the 7-bit addressing mode (See reference pg. 1328)
		working |= I2C_START;													//Start transmission, seeking response from LCD.
		
		I2C1->CR2 |= working;
			
		tim6_Del (TIME_100us);												//Delay 100us for the LCD
			
		if ((I2C1->ISR & I2C_NACKF) == I2C_NACKF) {
			error=1;
			continue;}																	//If a nack is recieved, restart the process.
		
		for (i=0; i<message_length; i++) {
			
			I2C1->TXDR = lcd_Commands[i];										//Provide the next command code to the LCD
			tim6_Del (i);																		//Delay by the LCD datasheet requirement
			
			if ((I2C1->ISR & I2C_NACKF) == I2C_NACKF) {
				error=1;																			//If a nack is recieved, restart the process.
				break;}		
		}
		
	} while (error);	
	
	if (retry == 0) {																		//If retry == 0, generate an error bit.
		return 1;}
	else {
		return 0;}
}



//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// LCD Write routine - takes an input character and writes it onto the current cursor location. Post-write, the LCD shifts one right.
// This function will return 0x00 if the transmission is successful.
// This function will retry up to 5 times if a message is NACKed. If the transmission is not successful, it will end the transmission and return 0x01.
// If the I2C1 channel is busy when called, transmission will not start. Instead, 0x02 will be returned.
// NOTE: When a NACK is received by the Master-transmitter, a STOP is immediately generated by hardware afterwards. (pg. 1296)

int lcd_Write (int write_Code, int number_retries) {
	
	int message[] = {LCD_CTRL_DATA,write_Code};					//Data message
	int error;																					//Error flag indicating the communication should restart or terminate	
	int working;																				//working variable
	int i;																							//Iterative variable
	int retry = number_retries;													//Number of retries
	
	if (retry > 5 || retry < 1) {												//Put boundaries on the number of retries
		retry = 5;}
		
	do {
//Send the address of the LCD. Try until a NACK is not recieved
		if(retry == 0) {																	//Decrement the retry variable each transmission time.
			break;}																					//Or, if retry is 0, return an error code.
		else {
			retry--;}
		
		error=0;
		I2C1->ICR = I2C_NACKCF;														//Clear the NACK flag.
				
		working = I2C1->CR2;															//Get current value of I2C1_CR2
		working &=I2C_CR2_WRITE0;													//Clear NBYTES in CR2 to prepare a new transmission.
		working |= I2C_CR2_WRITE2;												//Prepare to write 2-bytes of data to the LCD.
		working |= (LCD_ADDRESS << 1);										//Load I2C1 with the LCD address. Shift it by one for the 7-bit addressing mode (See reference pg. 1328)
		working |= I2C_START;															//Start transmission, seeking response from LCD.
			
		I2C1->CR2 = working;
			
		tim6_Del (TIME_100us);												//Delay 100us for the LCD
			
		if ((I2C1->ISR & I2C_NACKF) == I2C_NACKF) {
			error=1;
			continue;}																			//If a nack is recieved, restart the process.
	
//Send the first message byte to the LCD, loop to send the second. Restart communication if a NACK is recieved.
	
	for(i=0; i<2; i++) {	
			error=0;
			I2C1->TXDR = message[i];												//Tell the LCD that a data value is going to be sent
			tim6_Del (TIME_100us);													//Delay 100us for LCD requirements
			
			if ((I2C1->ISR & I2C_NACKF) == I2C_NACKF) {
				error=1;}																				//If a nack is recieved, restart the process.	
			}
	}	while (error);
	
	if (retry == 0) {																		//If retry == 0, generate an error bit.
		return 1;}
	else {
		return 0;}
}


/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//TEST BENCH CODE

void led_gpio_config(void) {

   // Enable the peripheral clock for GPIO port A.
   RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

   // Set pin PA5 to digital output: 01.
   GPIOA->MODER &= ~GPIO_MODER_MODE5;
   GPIOA->MODER |= GPIO_MODER_MODE5_0;

   // Set output type of pin PA5 to push-pull: 0 (default).
   GPIOA->OTYPER &= ~GPIO_OTYPER_OT5;

   // Set pin PA5 to low speed output: 00.
   GPIOA->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED5;

   // Set pin PA5 to no pull-up, no pull-down: 00.
   GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD5;

}

//---------------------------------------------------------------------------

void led_toggle(void) {

            GPIOA->ODR ^= GPIO_ODR_OD5;
}

//---------------------------------------------------------------------------

void test (void) {
	
	
	led_gpio_config();																		//Setup the green LED on the Nucleo board
	tim6_Init();																					//Setup Timer 6
	
	while (1) {
		led_toggle();
		tim6_Del(TIME_100us);																				//Full delay; no preload.
	}
}
//TEST BENCH CODE*/
