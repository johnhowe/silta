#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
#include "console.h"
#include "config.h"
#include "fifo.h"
#include "i2c.h"
#include "gpio.h"

typedef struct {
	char *commandStr;
	void (*fn)(uint8_t argc, char *argv[]);
	char *helpStr;
} command_t;

fifo_t usbRxFifo;

static char cmdBuff[64];
static uint8_t argc;
static char* argv[8];

static void helpFn(uint8_t argc, char *argv[]);
static void i2cCmd(uint8_t argc, char *argv[]);
static void gpioCmd(uint8_t argc, char *argv[]);
static void gpioCfgCmd(uint8_t argc, char *argv[]);

static command_t commands[] = {
	{"i2c", i2cCmd, "i2c <addr> <rdlen> [wrbytes (04 D1 ..)]"},
	{"config", cfgCmd, "<key> [value]"},
	{"gpio", gpioCmd, "<port[A-E]> <pin0-15> [value]"},
	{"gpiocfg", gpioCfgCmd, "<port[A-E]> <pin0-15> <in|outpp|outod> [pullup|pulldown|nopull]"},
	// Add new commands here!
	{"help", helpFn, "Print this!"},
	{NULL, NULL, NULL}
};

//
// Print the help menu
//
static void helpFn(uint8_t argc, char *argv[]) {
	command_t *command = commands;

	if(argc < 2) {
		while(command->commandStr != NULL) {
			printf("%s - %s\n", command->commandStr, command->helpStr);
			command++;
		}
	} else {
		while(command->commandStr != NULL) {
			if(strcmp(command->commandStr, argv[1]) == 0) {
				printf("%s - %s\n", command->commandStr, command->helpStr);
				break;
			}
			command++;
		}
	}
}

#define I2C_ADDR_OFFSET		(1)
#define I2C_RLEN_OFFSET		(2)
#define I2C_WBUFF_OFFSET	(3)
static void i2cCmd(uint8_t argc, char *argv[]) {
	uint8_t wBuff[128];
	uint8_t rBuff[128];
	int32_t rval;

	do {
		if(argc < 3) {
			printf("ERR: I2C Not enough arguments\n");
			break;
		}

		uint8_t addr = strtoul(argv[I2C_ADDR_OFFSET], NULL, 16);
		uint8_t rLen = strtoul(argv[I2C_RLEN_OFFSET], NULL, 10);
		uint8_t wLen = argc - I2C_WBUFF_OFFSET;
		
		if(wLen > sizeof(wBuff)) {
			printf("ERR: I2C Not enough space in wBuff\n");
			break;
		}

		if(rLen > sizeof(rBuff)) {
			printf("ERR: I2C Not enough space in rBuff\n");
			break;
		}

		if(wLen > 0) {
			for(uint32_t byte = 0; byte < wLen; byte++) {
				wBuff[byte] = strtoul(argv[I2C_WBUFF_OFFSET + byte], NULL, 16);
			}
		}

		rval = i2c(I2C1, addr, wLen, wBuff, rLen, rBuff);

		if(rval) {
			printf("ERR %d\n", rval);
		} else {
			printf("OK ");
			for(uint32_t byte = 0; byte < rLen; byte++) {
				printf("%02X ", rBuff[byte]);
			}
			printf("\n");
		}

	} while (0);
}

// 
// Set/get GPIO pins
// 
static void gpioCmd(uint8_t argc, char *argv[]) {
	if(argc > 2) {
		do {
			char port = toupper((uint32_t)argv[1][0]);
			uint8_t pin = strtoul(argv[2], NULL, 10);
			GPIO_TypeDef *GPIOx = NULL;

			if ((port < 'A') || (port > 'E')) {
				printf("ERR Invalid port");
				break;
			}

			if (pin > 15) {
				printf("ERR Invalid pin");
				break;
			}

			GPIOx = (GPIO_TypeDef *)(GPIOA_BASE + (uint32_t)(port - 'A') * (GPIOB_BASE - GPIOA_BASE));

			if(argc == 3) {
				int32_t value = gpioGet(GPIOx, pin);
				
				if (value < 0) {
					printf("ERR\n");
				} else {
					printf("OK %d\n", value);
				}
			} else {
				int32_t rval = gpioSet(GPIOx, pin, (argv[3][0] != '0'));
				
				if (rval < 0) {
					printf("ERR\n");
				} else {
					printf("OK\n");
				}
			}
		} while (0);
	} else {
		printf("ERR Invalid args");
	}
}

//
// Configure GPIO pins as input/output(open drain or push-pull) 
// and set pull-up/down resistors
//
static void gpioCfgCmd(uint8_t argc, char *argv[]) {
	if(argc > 2) {
		do {
			GPIO_InitTypeDef gpioSettings;
			char port = toupper((uint32_t)argv[1][0]);
			uint8_t pin = strtoul(argv[2], NULL, 10);
			GPIO_TypeDef *GPIOx = NULL;

			if ((port < 'A') || (port > 'E')) {
				printf("ERR Invalid port");
				break;
			}

			if (pin > 15) {
				printf("ERR Invalid pin");
				break;
			}

			GPIOx = (GPIO_TypeDef *)(GPIOA_BASE + (uint32_t)(port - 'A') * (GPIOB_BASE - GPIOA_BASE));
			gpioSettings.GPIO_Pin  = (1 << pin);
			gpioSettings.GPIO_Mode = GPIO_Mode_IN;
			gpioSettings.GPIO_Speed = GPIO_Speed_50MHz;
			gpioSettings.GPIO_OType = GPIO_OType_PP;
			gpioSettings.GPIO_PuPd = GPIO_PuPd_NOPULL;

			// in|outpp|outod
			if (strcmp("outpp", argv[3]) == 0) {
				gpioSettings.GPIO_Mode = GPIO_Mode_OUT;
				gpioSettings.GPIO_OType = GPIO_OType_PP;
			} else if (strcmp("outpp", argv[3]) == 0) {
				gpioSettings.GPIO_Mode = GPIO_Mode_OUT;
				gpioSettings.GPIO_OType = GPIO_OType_OD;
			}

			if (argc > 4) {
				// nopull|pullup|pulldown
				if (strcmp("pullup", argv[4]) == 0) {
					gpioSettings.GPIO_PuPd = GPIO_PuPd_UP;
				} else if (strcmp("pulldown", argv[4]) == 0) {
					gpioSettings.GPIO_PuPd = GPIO_PuPd_DOWN;
				}
			}
			
			GPIO_Init(GPIOx, &gpioSettings);

			printf("OK\n");
		} while (0);
	} else {
		printf("ERR Invalid args");
	}
}

void consoleProcess() {
	uint32_t inBytes = fifoSize(&usbRxFifo);
	if(inBytes > 0) {
		uint32_t newLine = 0;
		for(int32_t index = 0; index < inBytes; index++){
			if((fifoPeek(&usbRxFifo, index) == '\n') || (fifoPeek(&usbRxFifo, index) == '\r')) {
				newLine = index + 1;
				break;
			}
		}

		if(newLine > sizeof(cmdBuff)) {
			newLine = sizeof(cmdBuff) - 1;
		}

		if(newLine) {
			uint8_t *pBuf = (uint8_t *)cmdBuff;
			while(newLine--){
				*pBuf++ = fifoPop(&usbRxFifo);
			}

			// If it's an \r\n combination, discard the second one
			if((fifoPeek(&usbRxFifo, 0) == '\n') || (fifoPeek(&usbRxFifo, 0) == '\r')) {
				fifoPop(&usbRxFifo);
			}

			*(pBuf - 1) = 0; // String terminator

			argc = 0;

			// Get command
			argv[argc] = strtok(cmdBuff, " ");

			// Get arguments (if any)
			while ((argv[argc] != NULL) && (argc < sizeof(argv)/sizeof(char *))){
				argc++;
				argv[argc] = strtok(NULL, " ");
			}

			if(argc > 0) {
				command_t *command = commands;
				while(command->commandStr != NULL) {
					if(strcmp(command->commandStr, argv[0]) == 0) {
						command->fn(argc, argv);
						break;
					}
					command++;
				}

				if(command->commandStr == NULL) {
					printf("Unknown command '%s'\n", argv[0]);
					helpFn(1, NULL);
				}
			}
		}
	}
}