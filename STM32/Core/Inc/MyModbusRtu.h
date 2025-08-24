#ifndef MY_MODBUS_RTU_H
#define MY_MODBUS_RTU_H

#include <stdint.h>
#include "stm32f0xx_hal.h"
#include <string.h>

// --------------------------- The role of the device ----------------------------------------------------------------------------
enum ModbusRole { 
  Master, 
  Slave
};

// -------------------------- Main state of Modbus -------------------------------------------------------------------------------
struct Modbus {
    enum ModbusRole role;
    
    uint8_t device_address; 
    
    uint32_t request_start_time_ms;       // For Master timeout tracking
};

// ------------------------- Status for first checking of the message ------------------------------------------------------------

typedef enum {
    MODBUS_OK                     = 0x00, 

    MODBUS_ERR_CRC               = 0x11, // CRC check failed
    MODBUS_ERR_SLAVE_ID          = 0x12, // Invalid or mismatched slave address
    MODBUS_ERR_FRAME_INCOMPLETE  = 0x14, // Incomplete or too short Modbus frame   
} ModbusStatus;

// ------------------------- Request and Response --------------------------------------------------------------------------------

typedef struct 
{
  uint8_t device_address;
  uint8_t function_code;
  uint16_t register_address;
  uint16_t number_of_registers;
  uint8_t data[250];                    // Data and CRC are in this array 
                                        // If this is ReadRequest: CRC Lo, CRC Hi.
                                        // If this is WriteRequest: count_of_bytes, Value Hi, Value Lo, ..., CRC Lo, CRC Hi.
} Request;

typedef struct
{
  uint8_t device_address;
  uint8_t function_code;
  uint8_t data[254];                    // Data and CRC are in this array. Data can have count_of_bytes, and values for ReadResponse 
                                        // or register address (Hi, Lo), number of registers (Hi, Lo) for WriteResponse 
                                        // or if it is error - the fields which correspond ErrorResponse
} Response;                             

// To operate Request and Response structs it need to know the length of the packets. 
  

// -------------------------- Function Codes ---------------------------------------------------------------------------------------
typedef enum {
  WriteRegisters0x10 = 0x10,
  ReadRegisters0x03 = 0x03
} FunctionCode;


// -------------------------- Functions --------------------------------------------------------------------------------------------
// Global example of state
extern struct Modbus modbus;

// Initialization
void InitMyModbusRtu(void);

// Checks CRC and validates message length and slave address
// Returns MODBUS_OK or appropriate error code
ModbusStatus CheckMessage(uint8_t *buffer, uint8_t length);

// Calculate CRC16 Modbus
uint16_t CalculateCRC(uint8_t *data, uint8_t length_without_crc);

// Parses raw Modbus request buffer into Request struct and performs validation (CRC, address, length)
// Returns MODBUS_OK or error status
ModbusStatus ProcessRequest(Request *request, uint8_t *buffer, uint8_t length);

// Serializes Response struct into byte buffer for transmission
// Adds CRC at the end of the buffer
void ProcessResponse(Response *response, uint8_t *buffer, uint8_t length);

// Build Modbus error response
uint8_t BuildErrorResponse(Response *response, uint8_t device_address, uint8_t function_code, uint8_t code_of_error);

// Serializes a Modbus Request struct into byte buffer with CRC for sending
// Returns length of serialized request. Use for Master mode
uint8_t SerializeRequest(Request *request, uint8_t *buffer);

void UpdateSlaveAddress(uint8_t address);


#endif // MY_MODBUS_RTU_H
