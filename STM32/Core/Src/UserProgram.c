#include "UserProgram.h"
#include "MyUSART.h"
#include "MyModbusRtu.h"
#include "MyGPIO.h"

StateButtons state_button;

ProgramState program_state;

extern struct MyUSART my_usart;

void InitLEDS()
{
SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOCEN);
SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOAEN);

CLEAR_BIT(GPIOC->MODER, GPIO_MODER_MASK(GPIO_Px13_POSITION));
CLEAR_BIT(GPIOC->MODER, GPIO_MODER_MASK(GPIO_Px14_POSITION));

CLEAR_BIT(GPIOA->MODER, GPIO_MODER_MASK(GPIO_Px3_POSITION));
CLEAR_BIT(GPIOA->MODER, GPIO_MODER_MASK(GPIO_Px4_POSITION));
CLEAR_BIT(GPIOA->MODER, GPIO_MODER_MASK(GPIO_Px5_POSITION));

SET_BIT(GPIOC->MODER, GPIO_MODER_OUTPUT(GPIO_Px13_POSITION));
SET_BIT(GPIOC->MODER, GPIO_MODER_OUTPUT(GPIO_Px14_POSITION));

SET_BIT(GPIOA->MODER, GPIO_MODER_OUTPUT(GPIO_Px3_POSITION));
SET_BIT(GPIOA->MODER, GPIO_MODER_OUTPUT(GPIO_Px4_POSITION));
SET_BIT(GPIOA->MODER, GPIO_MODER_OUTPUT(GPIO_Px5_POSITION));

CLEAR_BIT(GPIOC->OTYPER, GPIO_OTYPER_MASK(GPIO_Px13_POSITION));
CLEAR_BIT(GPIOC->OTYPER, GPIO_OTYPER_MASK(GPIO_Px14_POSITION));

CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_MASK(GPIO_Px3_POSITION));
CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_MASK(GPIO_Px4_POSITION));
CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_MASK(GPIO_Px5_POSITION));

SET_BIT(GPIOA_ODR, (1 << GPIO_Px5_POSITION));

memset(&state_button, 0, sizeof(state_button));
 
}

void CheckingLEDS()
{
    if (state_button.button1)
        GPIOC->ODR ^=  (1 << 13);  

    if (state_button.button2)
        GPIOC->ODR ^=  (1 << 14);
    
    UpdateRegisters(); 
}

void UpdateRegisters()
{
  program_state.registers[0] = GPIOA->ODR;                 
  program_state.registers[1] = GPIOC->ODR;                          
}
  
void ApplyRegisters()
{
    GPIOA->ODR = program_state.registers[0];
    GPIOC->ODR = program_state.registers[1];
}

uint8_t RequestHandle(Request *request, Response *response, uint8_t length)
{
  uint8_t function_code = request->function_code;
  uint16_t start_address = request->register_address;
  uint16_t number_of_registers = request->number_of_registers;
  
  
  if ((start_address + number_of_registers) >= sizeof(program_state.registers)/sizeof(program_state.registers[0]))
      {
        return BuildErrorResponse(response, request->device_address, function_code, 0x02);
      } 
  
  switch(function_code)
  {
  case ReadRegisters0x03:
    {
      response->device_address = modbus.device_address;
      response->function_code = 0x03;
      
      uint8_t count_of_bytes = (uint8_t)request->number_of_registers * 2;
      
      UpdateRegisters();
      
      response->data[0] = count_of_bytes;
      
      uint16_t start_address = request->register_address;
      
      for (int i = 0; i < request->number_of_registers; ++i)
      {
        uint16_t current_value = program_state.registers[start_address + i];
          
        response->data[1 + i * 2] = (uint8_t)(current_value >> 8);
        response->data[2 + i * 2] = (uint8_t)(current_value);
      }
      
      return 5 + count_of_bytes;
    }
    
  case WriteRegisters0x10:
    {
      response->device_address = modbus.device_address;
      response->function_code = 0x10;
      
      
      for (int i = 0; i < number_of_registers; ++i)
      {
        uint16_t hi = request->data[1 + i * 2];
        uint16_t lo = request->data[2 + i * 2];
        uint16_t value = (hi << 8) | lo;

        program_state.registers[start_address + i] = value;
      }
      
      ApplyRegisters();
      
      response->data[0] = (uint8_t)(request->register_address >> 8);
      response->data[1] = (uint8_t)(request->register_address);
      response->data[2] = (uint8_t)(request->number_of_registers >> 8);
      response->data[3] = (uint8_t)(request->number_of_registers);
      
      return 8; 
    }
    
  default:
    return BuildErrorResponse(response, request->device_address, function_code, 0x01);
      
  }
 
}

void UserMainLoop()
{
    if (modbus.role == Slave)
    {
        if (my_usart.rx_complete)
        {
            my_usart.rx_complete = 0;

            Request request;
            Response response;
            
            ModbusStatus status = ProcessRequest(&request, my_usart.rx_buffer, my_usart.number_of_received_bytes);
            
            if(status == MODBUS_OK)                    
            {
              uint8_t length = RequestHandle(&request, &response, my_usart.number_of_received_bytes);
              ProcessResponse(&response, my_usart.tx_buffer, length);
              
              my_usart.tx_length = length;
              
              SendUSART(); 
            }
            

            my_usart.number_of_received_bytes = 0;
            memset(my_usart.rx_buffer, 0, sizeof(my_usart.rx_buffer));
        }
    }
    
   /* if (modbus.role == Master)
    {
          Firstly it need to form request
          Then process to Serialize Request
          Send request
          Check the time
          And get response
          
          
    }
    */
}

