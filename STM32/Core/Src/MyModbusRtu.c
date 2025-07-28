#include "MyModbusRtu.h"
#include "MyUSART.h"

struct Modbus modbus;

extern struct MyUSART my_usart;

void InitMyModbusRtu()
{
                                                                // write 35 in RTOR
  modbus.device_address = 0x01;
  
  modbus.role = Slave;
  
}

uint16_t CalculateCRC(uint8_t *data, uint8_t length)
{
  
  uint16_t crc = 0xFFFF;
  
  for(int i = 0; i < length - 2; i++)
  {
    crc ^= *data++;
    for (int i = 0; i < 8; i++)
    {
      if (crc & 0x01)
      {
        crc = (crc >> 1u) ^ 0xA001;
      }
      else {
        crc = crc >> 1u;
      }
    }
  }
  return crc;
}



ModbusStatus ProcessRequest(Request *request, uint8_t *buffer, uint8_t length)
{
  ModbusStatus status = CheckMessage(buffer, length);

  if(status != MODBUS_OK) return status;

  request->device_address = buffer[0];
  request->function_code = buffer[1];
  request->register_address = ((uint16_t)buffer[2] << 8) | ((uint16_t)buffer[3]);
  request->number_of_registers = ((uint16_t)buffer[4] << 8) | ((uint16_t)buffer[5]);
  
  for (int i = 0; i < length - 6; i++)
  {
    request->data[i] = buffer[6 + i];                                                   // Data in order Hi, Lo!
  }
  
  return MODBUS_OK;
}

void ProcessResponse(Response *response, uint8_t *buffer, uint8_t length)
{
  buffer[0] = response->device_address;
  buffer[1] = response->function_code;
  
  for(int i = 0; i < length - 4; i++)
  {   
    buffer[2 + i] = response->data[i];
  }
  
  uint16_t crc = CalculateCRC(buffer, length);
  
  buffer[length - 2] = (uint8_t)(crc);
  buffer[length - 1] = (uint8_t)(crc >> 8);
}


uint8_t BuildErrorResponse(Response *response, uint8_t device_address, uint8_t function_code, uint8_t code_of_error) 
{
  response->device_address = device_address;
  response->function_code = function_code | 0x80;
  response->data[0] = code_of_error;

  return 5;
}

  

ModbusStatus CheckMessage(uint8_t *buffer, uint8_t length)
{
  if (length < 4) return MODBUS_ERR_FRAME_INCOMPLETE;
  if (buffer[0] != modbus.device_address) return MODBUS_ERR_SLAVE_ID;
  
  uint16_t high = buffer[length - 1];
  uint16_t low = buffer[length - 2];
  uint16_t received_crc = ((high << 8) | low);
  
  uint16_t crc = CalculateCRC(buffer, length);
  
  if (crc != received_crc) return MODBUS_ERR_CRC;
    
  return MODBUS_OK;
     
}


uint8_t SerializeRequest(Request *request, uint8_t *buffer)
{
    buffer[0] = request->device_address;
    buffer[1] = request->function_code;
    buffer[2] = (uint8_t)(request->register_address >> 8);
    buffer[3] = (uint8_t)(request->register_address);
    buffer[4] = (uint8_t)(request->number_of_registers >> 8);
    buffer[5] = (uint8_t)(request->number_of_registers);

    if (request->function_code == WriteRegisters0x10)
    {
        uint8_t count_of_bytes = request->number_of_registers * 2;
        buffer[6] = count_of_bytes;

        for (int i = 0; i < count_of_bytes; ++i)
        {
            buffer[7 + i] = request->data[i];
        }

        uint16_t crc = CalculateCRC(buffer, 7 + count_of_bytes);
        buffer[7 + count_of_bytes] = (uint8_t)(crc);
        buffer[8 + count_of_bytes] = (uint8_t)(crc >> 8);

        return 9 + count_of_bytes - 1;
    }
    else if (request->function_code == ReadRegisters0x03)
    {
        uint16_t crc = CalculateCRC(buffer, 6);
        buffer[6] = (uint8_t)(crc);
        buffer[7] = (uint8_t)(crc >> 8);
        return 8;
    }

    return 0; 
}





