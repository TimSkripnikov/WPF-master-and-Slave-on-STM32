#ifndef USER_PROGRAM_H
#define USER_PROGRAM_H

#include <stdint.h>
#include "stm32f0xx_hal.h"
#include <string.h>

#include "MyModbusRtu.h"

typedef struct 
{
  uint16_t registers[16];                       // Array of general-purpose registers used by the program
} ProgramState;

typedef struct 
{
  uint8_t button1;
  uint8_t button2;

} StateButtons;

void CheckingLEDS(void);

void InitLEDS(void);

void UpdateRegisters(void);
  
void ApplyRegisters(void);

// Processes a Modbus RTU request and fills the corresponding response
// Supports function codes 0x03 (Read Holding Registers) and 0x10 (Write Multiple Registers)
// Returns length of the response in bytes
uint8_t RequestHandle(Request *request, Response *response, uint8_t length);

// Handles Modbus request parsing, dispatching, and response sending
void UserMainLoop(void);


#endif