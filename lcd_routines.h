//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// @file    lcd_routines.h
// @author  Ian Fillinger
// @date    August 18, 2025
// @Purpose	This code is for CPE 490 at Geneva College. This code will setup an I2C port
// 	It will send and recieve information to interface with an I2C LCD Display
//	It will be modular to allow students to utilize various subroutines in other exercises.
// 
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//RESOURCES USED----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//I2C1 On PB6 and PB7
	//TMR6 Basic Timer
	
//READ ME-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/*	Typical use of these subroutines is to interface to the PB6, PB7 I2C1 channel.
They can be used to write and command components including the LCD. The user must first
call the lcd_Init routine. This initializes the I2C port, the LCD, and TMR6.
	The LCD routines contained in this module require the input of a byte corresponding
to the command or character one wishes to send and the number of tries it is appropriate
for the LCD to complete when NACKs are generated or the channel is busy. The default and
and maxumum is (5) tries. If a connection is not established, or more than the alotted
number of retries occur, the routine will return (1) as an error code. Otherwise it will
return (0) to indicate normal operation.
	Once initialized, the user may write to the LCD cursor (displayed blinking) by
using the lcd_Write function, providing the ASCII character desired. The user may send
a command to the LCD by calling the lcd_Command function, providing the command byte
as an input to the function.
	Once initialized, the user may use the tmr6_Del function to start a timer of up to
524ms by providing the proper preload as an input for the function. No interrupts are
active for this peripheral. Polling will need to be conducted.
	The user may include any of these routines into their own main.c file by copying
the listed "include" directive and uncommenting it. A list of functions in order of
appearance within this file are included for ease of access.
	All page numbers can be reference to STMicroelectronics Manual RM0351 REV9 and to
the AMC1602AR-B-B6WTDW-I2C LCD datasheet REV1.*/

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
void tim6_Del (int preload);
void tim6_Init (void);
void i2c1_Init (void);
void i2c1_Pinmode_PB67 (void);
int lcd_Command (int command_Code, int number_retries);
int lcd_Init (int number_retries);
int lcd_Write (int write_Code, int number_retries);

//SUBROUTINES---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Copy and uncomment the following #include statement to use this entire file
	
	//#include "lcd_routines.h";
	
//LIST OF FUNCTIONS:

	//void tim6_Del (int);								//Setups TIM6 as a delay using the preload passed into the function.
	//void tim6_Init (void);							//Setups TIM6 as a count-up timer for times under 32.8ms. Called in lcd_Init.
	
	//void i2c1_Init (void);							//Initializes the I2C1 channel for operation. Called in lcd_Init.
	//void i2c1_Pinmode_PB67 (void);			//Initializes the GPIOB pins 6,7 for I2C1 operation. Called in lcd_Init.
	
	//int lcd_Command (int,int);					//Sends the command stored in LCD_DATA to the LCD (See LCD datasheet pg. 15)
	//int lcd_Init (int);									//Initializes the LCD, must be called before the LCD can be used
	//int lcd_Write	(int,int);						//Write the character stored in LCD_DATA into LCD memory, on-screen characters are stored in data memory from 00-0F and 40-4F (See datasheet pg. 12)
	
//CONSTANTS-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "stm32l476xx.h"
		
#define TIM6_CR1_SETUP			0x00000008		//Disables timer at overflow, clearing CEN, does not start clock
#define TIM6_PSC_SETUP			0x0000000F		//Sets TMR6 prescale to 15
#define TIM67_CR1_START			0x00000001		//Set the lowest bit in the TIM6/7 CR1 register to start timing	

#define TIME_MASK						0xFFFF0000		//Clears the current value in the TIM6 counter (should be 0 already)
#define TIME_STOP_CHK				0x00000001		//Check the CEN bit in the CR1 register to see if the timer is halted
#define TIME_100us					0x0000FFE7		//Preload into TIM6 to create a count-up timer for 100us at a clock default 4MHz, prescale 15 (See Reference pg. 1184 for details)
#define TIME_10ms						0x0000F63C		//Preload into TIM6 to create a count-up timer for 10ms at a clock default 4MHz, prescale 15	(See Reference pg. 1184 for details)
#define TIME_40ms						0x0000D8F0		//Preload into TIM6 to create a count-up timer for 40ms at a clock default 4MHz, prescale 15	(See Reference pg. 1184 for details)

#define GPIOB_76_MODER 			0xFFFFAFFF		//Setup PB7 and PB6 to be their alternate functions (I2C1 SDA and SCL respectively), B'1111 1111 1111 1111 1010 1111 1111 1111'
#define GPIOB_76_ALT_FUNCT	0x44000000		//Setup PB7 and PB6 to use alternate function 4 (pertains to I2C1)
#define GPIOB_76_OTYPER			0x000000C0		//Setup PB7 and PB6 as open-drain outputs
#define GPIOB_76_OSPEEDR		0x00005000		//Setup PB7 and PB6 as "medium" speed outputs		
		
		
#define I2C_PER_DISABLE 		0xFFFFFFFE		//Clear the peripheral enable for setup, RESET- 0xFFFFFFFF
#define I2C_PER_ENABLE			0x00000001		//Set the peripheral enable
#define I2C1_CR1_SETUP			0x00000000		//RESET- 0x00000000,  NACK calls from other devices, allow clock stretching, do not use DMA, Analog filter ON, Digital filter OFF, Interrupts disabled, Peripheral disabled
#define I2C1_CR1_STRETCH		0x00000000		//Configure clock stretching (NOTE; leaving this as zero maintains the default of allow ing the clock to be stretched
#define I2C1_TIM_SETUP			0x00420F13		//RESET- 0x00000000,  Operation at standard mode (100kHz) to stay within the operation parameters of the I2C components on the auxilary board
												//See pg. 1304 of the STM32L47XXX Reference Manual for sample times provided by STM. See pg. 1330 of the same for equations.
												//With a clock frequency of 4MHz, desiring characteristics in Table 235 at Standard (100 kHz) mode, PRESC=D'0', SDADEL=D'2', SCLDEL=D'4', SCLH=D'15', SCLL=D'19'


#define I2C_CR2_WRITE0			0xFF00FFFF		//Used to clear the NBYTES bits to prepare the CR2 register to send a different number of bytes
#define I2C_CR2_WRITE2			0x02020000		//RESET- 0x00000000, send 2-bytes and autoend (one control, one data, See LCD datasheet pg. 18), allow clock stretching, 7-bit addressing, write transfer B'0000 0010 0000 0010 0000 0000 0000 0000'
#define I2C_CR2_WRITE6			0x02060000		//Used to initialize the LCD (See LCD datasheet pg. 18-19), send 6 bytes and end, allow clock stretching, 7-bit addressing, write transfer B'0000 0010 0000 0110 0000 0000 0000 0000'
#define I2C_REFILL_MASK			0x00000002		//Mask all other bits in the I2C_ISR register except the TXIS bit
#define I2C_REFILL					0x00000000		//If bit 1 is set, then a refill needs to occur, until then, the program will poll this flag. Note; flag resets automatically
#define I2C_START						0x00002000		//Start an I2C transfer B'0000 0000 0000 0000 0010 0000 0000 0000'
#define I2C_NACKF						0x00000010		//The NACKF is set when a NACK is recieved
#define I2C_NACKCF					0x00000010		//Writing a '1' to bit 4 (NACKCF) clears the NACK flag in the ISR. This can be used to restart a transmission if it failed, and autoended.
#define I2C_DATA_MASK				0x000000FF		//Used to mask an integer data input into I2C_TXDR as only the least significant byte is allowed to be modified.	


#define LCD_ADDRESS 				0x0000003C		//LDC address to I2C_CR2
#define LCD_CTRL_COMMAND		0x00000000		//First command to write to the LCD, another command byte will follow the data byte, the data byte will be decoded and the command executed RAM B'1000 0000'
#define LCD_CTRL_DATA				0x00000040		//First command to write to the LCD, another data byte will follow the data byte, the data byte will be save in display RAM B'0100 0000'
#define LCD_FUNCTION_SET		0x00000038		//Setup for 8-bit data interface, 2-line LCD with 5x8 dots
#define LCD_DISP_ON					0x0000000F		//Setup LCD to show the displayed locations, the cursor, and to have the cursor blink
#define LCD_CLEAR						0x00000001		//Clear the LCD display
#define LCD_ENTRY_MODE			0x00000006		//Allow for the cursor to shift to the right one after a write
#define LCD_CURSOR_SET			0x00000010		//Set cursor to shift to the right 

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------





//TEST BENCH CODE*/






