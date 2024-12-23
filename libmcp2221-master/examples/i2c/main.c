/*
 * Project: MCP2221 HID Library
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2015 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/mcp2221-hid-library/
 */

#ifndef _WIN32
	#define _BSD_SOURCE
	#include <unistd.h>
	#define Sleep(ms) usleep(ms * 1000)
#endif

#include <stdio.h>
#include <stdlib.h>
#include "../../libmcp2221/win/win.h"
#include "../../libmcp2221/libmcp2221.h"
#include "../../libmcp2221/hidapi.h"

int main(int argc, char **argv)
{
  unsigned char addr;

	puts("Starting!");

	argc--;
	argv++;
	addr = strtoul(*argv, NULL, 0);
	printf("trying addr %2.2x\n", addr);

	mcp2221_init();
	
	// Get list of MCP2221s
	printf("Looking for devices... ");
	int count = mcp2221_find(MCP2221_DEFAULT_VID, MCP2221_DEFAULT_PID, NULL, NULL, NULL);
	printf("found %d devices\n", count);

	// Open whatever device was found first
	printf("Opening device... ");
	mcp2221_t* myDev = mcp2221_open();

	if(!myDev)
	{
		mcp2221_exit();
		puts("No MCP2221s found");
		getchar();
		return 0;
	}
	puts("done");

	mcp2221_error res;

	// Configure GPIOs
	printf("Configuring GPIOs... ");
	mcp2221_gpioconfset_t gpioConf = mcp2221_GPIOConfInit();

	// Check GP DESIGNATION TABLE in the datasheet for what the dedicated and alternate functions are for each GPIO pin

	// Configure GPIO 0/1/2/3 as outputs, and low
	gpioConf.conf[0].gpios		= MCP2221_GPIO0 | MCP2221_GPIO1 | MCP2221_GPIO2 | MCP2221_GPIO3;
	gpioConf.conf[0].mode		= MCP2221_GPIO_MODE_GPIO;
	gpioConf.conf[0].direction	= MCP2221_GPIO_DIR_OUTPUT;
	gpioConf.conf[0].value		= MCP2221_GPIO_VALUE_LOW;

	// Apply config
	mcp2221_setGPIOConf(myDev, &gpioConf);

	// Also save config to flash
	mcp2221_saveGPIOConf(myDev, &gpioConf);

	
	// NOTE: I2C is not complete yet

	while(1)
	{
		Sleep(500);

		uint8_t buff[2];
		mcp2221_i2c_state_t state = MCP2221_I2C_IDLE;
		mcp2221_i2c_status_t status = MCP2221_I2C_OK;

		// Stop any transfers
		mcp2221_i2cState(myDev, &state);
		if(state != MCP2221_I2C_IDLE)
			mcp2221_i2cCancel(myDev);
		
		// Set divider from 12MHz
		mcp2221_i2cDivider(myDev, 260);


		// this is the 7-bit address
		// Clock chip is 0x44
		// Write 1 byte
		puts("Writing...");
		uint8_t tmp = 0;
		mcp2221_i2cWrite(myDev, addr, &tmp, 1, MCP2221_I2CRW_NORMAL);
		puts("Write complete");

		// Wait for completion
		while(1)
		{
		  if(mcp2221_i2cFullStatus(myDev, &state, &status) != MCP2221_SUCCESS)
				puts("ERROR");
		  printf("State: %hhu Status: %hhu\n", state, status);
			if(state == MCP2221_I2C_IDLE || status != MCP2221_I2C_OK)
				break;
		}
		if (status != MCP2221_I2C_OK) {
		  printf("nak\n");
		  break;
		}

		// Begin reading 2 bytes
		puts("Reading...");
		mcp2221_i2cRead(myDev, addr, 2, MCP2221_I2CRW_NORMAL);
		puts("Read complete");
		
		// Wait for completion
		while(1)
		{
			mcp2221_i2cState(myDev, &state);
			printf("State: %hhu\n", state);
			if(state == MCP2221_I2C_DATAREADY)
				break;
		}
		
		// Get the data
		puts("Getting...");
		mcp2221_i2cGet(myDev, buff, 2);
		puts("Get complete");

		// Show the temperature
		int16_t temperature = (buff[0]<<8) | buff[1];
		temperature >>= 7;
		printf("  Temp: %u\n", temperature);
	}

	switch(res)
	{
		case MCP2221_SUCCESS:
			puts("No error");
			break;
		case MCP2221_ERROR:
			puts("General error");
			break;
		case MCP2221_INVALID_ARG:
			puts("Invalid argument, probably null pointer");
			break;
		case MCP2221_ERROR_HID:
			printf("USB HID Error: %ls\n", hid_error(myDev->handle));
			break;
		default:
			printf("Unknown error %d\n", res);
			break;
	}
	
	mcp2221_exit();
	
	return 0;
}
