#ifndef MY_MODBUS_RTU_H
#define MY_MODBUS_RTU_H

#include <stdint.h>
#include "stm32f0xx_hal.h"
#include <string.h>

// The role of the device
enum ModbusRole { 
  Master, 
  Slave
};

// Main state of Modbus
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
    
    uint32_t request_start_time_ms;  // for Master timeout tracking
};

// Global example of state
extern struct Modbus modbus;

// Initialization
void InitMyModbusRtu(void);

// USART interrupt handler logic
void MyModbusRtuHandler(void);

// Check CRC and message validity
uint8_t CheckMessage(void);

// Process incoming message (Slave)
void ProcessMessage(void);

// Send prepared response or request
void ModbusSend(void);

// Calculate CRC16 Modbus
uint16_t CalculateCRC(uint8_t *data, uint8_t length_without_crc);

// Update modbus registers from hardware inputs
void UpdateRegisters(void);

// Apply modbus registers to hardware outputs
void ApplyRegisters(void);

// Build Modbus response for function 0x03 (Read Holding Registers)
void BuildResponse_0x03(uint16_t start_address, uint16_t number_of_registers);

// Build Modbus response for function 0x10 (Write Multiple Registers)
void BuildResponse_0x10(uint16_t start_address, uint16_t number_of_registers, uint8_t count_of_bytes);

// Build Modbus error response
void BuildErrorResponse(uint8_t code_of_error, uint8_t function_code);

// Main loop, handles Master and Slave logic
void MainLoop(void);

// Handle response processing in Master mode
void HandleMasterResponse(void);

// Master: send read holding registers request
void ModbusRequestRead(uint8_t slave_address, uint16_t start_address, uint16_t num_registers);

// Master: send write multiple registers request
void ModbusRequestWrite(uint8_t slave_address, uint16_t start_address, uint16_t num_registers);

#endif // MY_MODBUS_RTU_H
