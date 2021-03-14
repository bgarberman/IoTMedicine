/**
 * \file
 *
 * \brief SD/MMC card example with FatFs
 *
 * Copyright (c) 2014-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

/**
 * \mainpage SD/MMC Card with FatFs Example
 *
 * \section Purpose
 *
 * This example shows how to implement the SD/MMC stack with the FatFS.
 * It will mount the file system and write a file in the card.
 *
 * The example outputs the information through the standard output (stdio).
 * To know the output used on the board, look in the conf_example.h file
 * and connect a terminal to the correct port.
 *
 * While using Xplained Pro evaluation kits, please attach I/O1 Xplained Pro
 * extension board to EXT1.
 *
 * \section Usage
 *
 * -# Build the program and download it into the board.
 * -# On the computer, open and configure a terminal application.
 * Refert to conf_example.h file.
 * -# Start the application.
 * -# In the terminal window, the following text should appear:
 *    \code
 *     -- SD/MMC Card Example on FatFs --
 *     -- Compiled: xxx xx xxxx xx:xx:xx --
 *     Please plug an SD, MMC card in slot.
 *    \endcode
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include <asf.h>
#include "conf_example.h"
#include <string.h>
#include "SerialConsole/SerialConsole.h"

//! Structure for UART module connected to EDBG (used for unit test output)
struct usart_module cdc_uart_module;

void configure_port_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction  = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(PIN_PB23, &config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_PA23, &config_port_pin);
}


/**
 * \brief Application entry point.
 *
 * \return Unused (ANSI-C compatibility).
 */
int main(void)
{

	//INITIALIZE VARIABLES
	char test_file_name[] = "0:sd_mmc_test.txt";
	Ctrl_status status;
	FRESULT res;
	FATFS fs;
	FIL file_object;

	//INITIALIZE SYSTEM PERIPHERALS
	system_init();
	delay_init();
	InitializeSerialConsole();
	system_interrupt_enable_global();
	/* Initialize SD MMC stack */
	sd_mmc_init();
	configure_port_pins();
	irq_initialize_vectors();
	cpu_irq_enable();

	SerialConsoleWriteString("ESE516 - MAIN APPLICATION running!\r\n\r\n");	//Order to add string to TX Buffer
	
	while(1){
		port_pin_set_output_level(PIN_PA23, 1);
		delay_ms(50);
		port_pin_set_output_level(PIN_PA23, 0);
		delay_ms(50);
	}

	
}




