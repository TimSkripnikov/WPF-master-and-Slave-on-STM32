#ifndef MY_MODBUS_RTU_H
#define MY_MODBUS_RTU_H

#include <stdint.h>
#include "stm32f0xx_hal.h"
#include <string.h>



//The role of the device
enum ModbusRole { 
  Master, 
  Slave
};

//Main state of Modbus
struct Modbus {
    enum ModbusRole role;
    
    uint8_t device_address; 

    uint8_t tx_buffer[256];
    uint8_t rx_buffer[256];

    uint8_t tx_index;
    uint8_t rx_index;
    uint8_t tx_length;
    
    uint8_t rx_done;
    uint8_t tx_busy;
    
    uint16_t registers[16];  
    
    uint32_t request_start_time_ms; 
};

//global examle of state
extern struct Modbus modbus;

//init
void InitMyModbusRtu(void);

void MyModbusRtuHandler(void);
  
uint8_t CheckMessage(void);
    
void ProcessMessage(void);

void ModbusSend(void);

uint16_t CalculateCRC(uint8_t *data, uint8_t length_without_crc);

void UpdateRegisters(void);

 void ApplyRegisters(void);

void BuildResponse_0x03(uint16_t start_address, uint16_t number_of_registers);

void BuildResponse_0x10(uint16_t start_address, uint16_t number_of_registers, uint8_t count_of_bytes);

void BuildErrorResponse(uint8_t code_of_error, uint8_t function_code);

void MainLoop(void);

#endif
