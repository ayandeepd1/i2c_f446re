/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Auto-generated by STM32CubeIDE
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include <stdint.h>
#include <stdio.h>

#include "stm32f446xx.h"
//#include "MPU6050.h"

//i2c(1) on pb8>scl  pb9>sda
void i2c_config(){
	RCC->APB1ENR |=(1<<21); 	//i2c(1) clock clock enable
	RCC->AHB1ENR |= 3;  		//Enable GPIOB and GPIOA CLOCK
	GPIOA->MODER |=(1<10);		//Enable the LED at pin PA5 in output(01) mode
	GPIOB->MODER |= (2<16) | (2<<18); 	//Enable the pin 8(17,16 bits) and 9(19,18 bits) in alternate function mode(10)
	GPIOB->OTYPER |=(3<<8); 			//Enable open drain(1) in pb8 and pb9 (8, 9 bits)
	GPIOB->OSPEEDR |=(3<<16) | (3<<18); //Enable high speed(11) in pb8(16,17 bits) and pb9(18,19 bits)
	GPIOB->PUPDR |=(1<<16) | (1<<18);  	//Enable pull up(01) in pb8 and pb9
	GPIOB->AFR[1] |=4 | (4<<4);	 		//AFR[1] is AFRH registers as pin 8 and 9 fall under AFRH(0,1,2,3 and 4,5,6,7 bits)
										//	Enable Alternate function registers(0100 for AF4 which corresponds to i2c1)

	I2C1->CR1 |=(1<<15);	//I2C put into reset
	I2C1->CR1 &=~(1<<15);	//I2c cleared from reset

	I2C1->CR2 |= (45<<0); 	//peripheral clock(tpclk1) frequency set to i2c frequency
	I2C1->CCR = 225;		//Configure the clock control registers(I2C_CCR register in datasheet)
	I2C1->TRISE = 46;		//configure i2c_trise register

	I2C1->CR1 |= 1;  		//Enable I2C(setting bit 0 for pe)

}

void i2c_start(){
	I2C1->CR1 |= (1<<10);  	//Enable ACK
	I2C1->CR1 |= (1<<8);  	//Generate START

	while(!(I2C1->SR1 & 1));	//wait for i2c_sr1 register's start bit(SB) bit to be set

}

void i2c_write(uint8_t data){
	while(!(I2C1->SR1 & (1<<7))); 	//wait for TXE bit to be set(indicates data buffer is empty)

	I2C1->DR = data;				//write data to i2c_dr (data register)

	while(!(I2C1->SR1 & (1<<2)));	//wait for transmission to end(BTF bit is set)
}

void i2c_send_address(uint8_t address){
	I2C1->DR = address;						//i2c data register is written with address
	while(!(I2C1->SR1 & 1<<1)); 			//wait for ADDR bit to be set in sr1 register(1st bit)
	uint8_t tm = I2C1->SR1 | I2C1->SR2;		// i2c_sr1 is read and then i2c sr2 is read to clear ADDR bit in i2c_sr1
}

void i2c_stop(){
	I2C1->CR1 |= 1<<9;			//stop the i2c bus(stop bit set on i2c_cr1)
}

void i2c_write_bytes(uint8_t* data, uint8_t size){
	while(!(I2C1->SR1 & (1<<7)));	//wait for TXE bit to be set(indicates data buffer is empty)
	while(size){
		while(!(I2C1->SR1 & (1<<7)));	//wait for TXE bit to be set(indicates data buffer is empty)
		I2C1->DR = (volatile uint32_t)* data++;
		size--;
	}
	while(!(I2C1->SR1 & (1<<2)));	//wait for transmission to end(BTF bit is set)
}

void i2c_read (uint8_t Address, uint8_t *buffer, uint8_t size)
{
/**** STEPS FOLLOWED  ************
1. If only 1 BYTE needs to be Read
	a) Write the slave Address, and wait for the ADDR bit (bit 1 in SR1) to be set
	b) the Acknowledge disable is made during EV6 (before ADDR flag is cleared) and the STOP condition generation is made after EV6
	c) Wait for the RXNE (Receive Buffer not Empty) bit to set
	d) Read the data from the DR

2. If Multiple BYTES needs to be read
  a) Write the slave Address, and wait for the ADDR bit (bit 1 in SR1) to be set
	b) Clear the ADDR bit by reading the SR1 and SR2 Registers
	c) Wait for the RXNE (Receive buffer not empty) bit to set
	d) Read the data from the DR
	e) Generate the Acknowledgment by setting the ACK (bit 10 in SR1)
	f) To generate the non acknowledge pulse after the last received data byte, the ACK bit must be cleared just after reading the
		 second last data byte (after second last RxNE event)
	g) In order to generate the Stop/Restart condition, software must set the STOP/START bit
	   after reading the second last data byte (after the second last RxNE event)
*/

	int remaining = size;

/**** STEP 1 ****/
	if (size == 1)
	{
		I2C1->DR = Address;  			//  send the address
		while (!(I2C1->SR1 & (1<<1)));  // wait for ADDR bit to set

		/**** STEP 1-b ****/
		I2C1->CR1 &= ~(1<<10);  // clear the ACK bit
		uint8_t temp = I2C1->SR1 | I2C1->SR2;  // read SR1 and SR2 to clear the ADDR bit.... EV6 condition
		I2C1->CR1 |= (1<<9);  // Stop I2C

		/**** STEP 1-c ****/
		while (!(I2C1->SR1 & (1<<6)));  // wait for RxNE to set

		/**** STEP 1-d ****/
		buffer[size-remaining] = I2C1->DR;  // Read the data from the DATA REGISTER

	}

/**** STEP 2 ****/
	else
	{
		/**** STEP 2-a ****/
		I2C1->DR = Address;  //  send the address
		while (!(I2C1->SR1 & (1<<1)));  // wait for ADDR bit to set

		/**** STEP 2-b ****/
		uint8_t temp = I2C1->SR1 | I2C1->SR2;  // read SR1 and SR2 to clear the ADDR bit

		while (remaining>2)
		{
			/**** STEP 2-c ****/
			while (!(I2C1->SR1 & (1<<6)));  // wait for RxNE to set

			/**** STEP 2-d ****/
			buffer[size-remaining] = I2C1->DR;  // copy the data into the buffer

			/**** STEP 2-e ****/
			I2C1->CR1 |= 1<<10;  // Set the ACK bit to Acknowledge the data received

			remaining--;
		}

		// Read the SECOND LAST BYTE
		while (!(I2C1->SR1 & (1<<6)));  // wait for RxNE to set
		buffer[size-remaining] = I2C1->DR;

		/**** STEP 2-f ****/
		I2C1->CR1 &= ~(1<<10);  // clear the ACK bit

		/**** STEP 2-g ****/
		I2C1->CR1 |= (1<<9);  // Stop I2C

		remaining--;

		// Read the Last BYTE
		while (!(I2C1->SR1 & (1<<6)));  // wait for RxNE to set
		buffer[size-remaining] = I2C1->DR;  // copy the data into the buffer
	}
}


int main(void)
{
	SysClockConfig();
	TIM6Config();

	//int16_t xa,ya,za;

	i2c_config();

	i2c_start();
	i2c_send_address(8);

	char a;
	i2c_read(8, &a, 1);
	printf("received: %c\n", a);
	//i2c_stop();

	i2c_start();
	i2c_send_address(8);
	i2c_write('a');
	i2c_stop();


	//MPU6050_initialize();
	//MPU6050_getAcceleration(&xa, &ya, &za);


	for(;;);
}
