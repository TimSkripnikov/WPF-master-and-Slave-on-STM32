#include "MyModbusRtu.h"

struct Modbus modbus;

volatile uint32_t sys_tick_ms = 0;

uint32_t GetTickMs(void)
{
    return sys_tick_ms;
}

void InitMyModbusRtu()
{
  
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;         // turn on Clock Enable of USART
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;            // pins for usart
  
  GPIOA->MODER &= ~(0b111111 << (9 * 2));       // reset mode on PA9, PA10, PA11
  GPIOA->MODER |=  (0b10 << (9 * 2));           // PA9  - AF (TX)
  GPIOA->MODER |= (0b10 << (10 * 2));           // PA10 - AF (RX)
  GPIOA->MODER |= (0b01 << (11 * 2));           // PA11 - output
  
  GPIOA->OTYPER &= ~(0b1 << 9);
  GPIOA->OTYPER &= ~(0b1 << 10);
  GPIOA->OTYPER &= ~(0b1 << 11);                // all in push-pull
  
  GPIOA->AFR[1] &= ~((0xF << 4) | (0xF << 8));  // PA9, PA10
  GPIOA->AFR[1] |=  (1 << 4) | (1 << 8);        // AF1

  
  USART1->CR1 &= ~((0b1 << 28) | (0b1 << 12));  // m = 00, word = 8 bit
  USART1->BRR = (8000000/9600);
  USART1->CR2 &= ~(0b11 << 12);                 // 1 stop bit

  USART1->CR1 |= (0b1);                         // USART_CR1_UE; 
  USART1->CR1 |= (0b1 << 3);                    // transmit enable
  USART1->CR1 |= (0b1 << 2);                    // receiver enable
  USART1->CR1 |= (0b1 << 5);                    //RXNE Interrupts Enable
  
  NVIC->ISER[0] |= (1 << 27);  
  
  USART1->CR2 |= (1 << 23);                     // activate RTO for cycle pause 
  USART1->CR1 |= (1 << 26);                     // RTO interrupts
  USART1->RTOR &= 0xFF000000;
  USART1->RTOR |= 0x23;                         // write 35 in RTOR
  
  modbus.device_address = 0x01;
  
  modbus.role = Slave;
  
}

uint16_t CalculateCRC(uint8_t *data, uint8_t length_without_crc)
{
  uint16_t crc = 0xFFFF;
  while (length_without_crc--)
  {
    crc ^= *data++;
    for (int i = 0; i < 8; i++)
    {
      if (crc & 0x01)
      {
        crc = (crc >> 1u) ^ 0xA001;
      } else {
        crc = crc >> 1u;
      }
    }
    
  }
  return crc;
}

void UpdateRegisters()
{
  modbus.registers[0] = GPIOA->ODR;                     //
  modbus.registers[1] = GPIOC->ODR;                          
}
  
void ApplyRegisters()
{
    GPIOA->ODR = modbus.registers[0];
    GPIOC->ODR = modbus.registers[1];
}


void BuildResponse_0x03(uint16_t start_address, uint16_t number_of_registers)
{
  if(start_address > sizeof(modbus.registers)) 
  {
    BuildErrorResponse(0x02, modbus.rx_buffer[1]);
    return;
  }
                            
  modbus.tx_buffer[0] = modbus.device_address;
  modbus.tx_buffer[1] = 0x03;
  modbus.tx_buffer[2] = (uint8_t)number_of_registers * 2;
  
  UpdateRegisters();
  
  for (int i = 0; i < number_of_registers; ++i)
  {
    uint16_t current_value = modbus.registers[start_address + i];
      
    modbus.tx_buffer[3 + i * 2] = (uint8_t)(current_value >> 8);
    modbus.tx_buffer[4 + i * 2] = (uint8_t)(current_value);
  }
  
  uint8_t length_without_crc = 3 + modbus.tx_buffer[2];
  uint8_t length = length_without_crc + 2;
  
  modbus.tx_length = length;
  
  uint8_t data_without_crc[256];
  
  memcpy(data_without_crc, modbus.tx_buffer, length_without_crc);
  
  uint16_t crc = CalculateCRC(data_without_crc, length_without_crc);
  
  modbus.tx_buffer[length - 2] = (uint8_t)crc;
  modbus.tx_buffer[length - 1] = (uint8_t)(crc >> 8);
  
  ModbusSend();

}
  
void BuildResponse_0x10(uint16_t start_address, uint16_t number_of_registers, uint8_t count_of_bytes)
{
    if(start_address > sizeof(modbus.registers)) 
    {
    BuildErrorResponse(0x02, modbus.rx_buffer[1]);
    return;
    }
                       
    for (int i = 0; i < number_of_registers; ++i)
    {
        uint16_t hi = modbus.rx_buffer[7 + i * 2];
        uint16_t lo = modbus.rx_buffer[8 + i * 2];
        uint16_t value = (hi << 8) | lo;

        modbus.registers[start_address + i] = value;
        
    }
  
    ApplyRegisters();

    modbus.tx_buffer[0] = modbus.device_address;
    modbus.tx_buffer[1] = 0x10;
    modbus.tx_buffer[2] = (uint8_t)(start_address >> 8);
    modbus.tx_buffer[3] = (uint8_t)(start_address);
    modbus.tx_buffer[4] = (uint8_t)(number_of_registers >> 8);
    modbus.tx_buffer[5] = (uint8_t)(number_of_registers);

    uint8_t length_without_crc = 6;
    uint8_t length = length_without_crc + 2;

    modbus.tx_length = length;

    uint8_t data_without_crc[256];
    memcpy(data_without_crc, modbus.tx_buffer, length_without_crc);

    uint16_t crc = CalculateCRC(data_without_crc, length_without_crc);

    modbus.tx_buffer[length - 2] = (uint8_t)(crc);
    modbus.tx_buffer[length - 1] = (uint8_t)(crc >> 8);

    ModbusSend();
}

void BuildErrorResponse(uint8_t code_of_error, uint8_t function_code) 
{
  modbus.tx_buffer[0] = modbus.device_address;
  modbus.tx_buffer[1] = function_code | 0x80;
  modbus.tx_buffer[2] = code_of_error;
  
  uint8_t data_without_crc[3];
  memcpy(data_without_crc, modbus.tx_buffer, 3);
  
  uint16_t crc = CalculateCRC(data_without_crc, 3);
  modbus.tx_buffer[3] = (uint8_t)(crc);
  modbus.tx_buffer[4] = (uint8_t)(crc >> 8);
  
  ModbusSend();
}
  

void ProcessMessage()
{
  uint8_t function = modbus.rx_buffer[1];
  
  switch (function)
  {
    case 0x03:
    {
        uint16_t start_address = (modbus.rx_buffer[2] << 8) | modbus.rx_buffer[3];
        uint16_t number_of_registers = (modbus.rx_buffer[4] << 8) | modbus.rx_buffer[5];

        BuildResponse_0x03(start_address, number_of_registers);
        break;
    }
    
    case 0x10:
    {
      uint16_t start_address = (modbus.rx_buffer[2] << 8) | modbus.rx_buffer[3];
      uint16_t number_of_registers = (modbus.rx_buffer[4] << 8) | modbus.rx_buffer[5];
      
      if ((start_address + number_of_registers) >= sizeof(modbus.registers)/sizeof(modbus.registers[0]))
      {
        BuildErrorResponse(0x02, function);
        break;
      } 
  
      uint8_t count_of_bytes = modbus.rx_buffer[6];
      
      if (count_of_bytes != number_of_registers * 2) 
      {
        BuildErrorResponse(0x03, function); 
        return;
      }
      
      BuildResponse_0x10(start_address, number_of_registers, count_of_bytes);
      break;
    }
      
    default:
    {
      BuildErrorResponse(0x01, function);
      break;
    }
  }
}

uint8_t CheckMessage()
{
  if (modbus.rx_index < 4) return 0;
  if (modbus.rx_buffer[0] != modbus.device_address) return 0;
  
  uint8_t length = modbus.rx_index;
  uint8_t length_without_crc = length - 2;
  
  uint16_t high = modbus.rx_buffer[length - 1];
  uint16_t low = modbus.rx_buffer[length - 2];
  uint16_t received_crc = ((high << 8) | low);
  
  uint8_t data_without_crc[256];
  
  memcpy(data_without_crc, modbus.rx_buffer, length_without_crc);
  
  uint16_t crc = CalculateCRC(data_without_crc, length_without_crc);
  
  if (crc != received_crc) return 0;                                    //check crc

  return 1;
     
}


void MyModbusRtuHandler()
{ 
  // ------------------Receiving---------------------
  if((USART1->ISR & (1 << 5)) && (USART1->CR1 & (1 << 5))) 
  {
    if (modbus.rx_index < sizeof(modbus.rx_buffer))
    {
      modbus.rx_buffer[modbus.rx_index++] = USART1->RDR;
    }
    else
    {
      uint8_t tmp = USART1->RDR;
    }
  }
        
  if((USART1->ISR & (1 << 11) && (USART1->CR2 & (1 << 23))))
  {
    USART1->ICR |= (1 << 11);
    
    modbus.rx_done = 1; 
  }
   
  // ------------------Transmission------------------
  
    if ((USART1->ISR & (1 << 7)) && (USART1->CR1 & (1 << 7))) // TXE
    {
      if (modbus.tx_index < modbus.tx_length)
      {
          USART1->TDR = modbus.tx_buffer[modbus.tx_index++];
      }
      else
      {
          USART1->CR1 &= ~(1 << 7);  
          USART1->CR1 |=  (1 << 6);
      }
    }

    if ((USART1->ISR & (1 << 6)) && (USART1->CR1 & (1 << 6))) // TC
    {
        USART1->CR1 &= ~(1 << 6);       
        GPIOA->ODR &= ~(1 << 11);      
        modbus.tx_busy = 0;      
    }

}
        
void ModbusSend()
{
    if (modbus.tx_busy) return;

    modbus.tx_index = 0;
    modbus.tx_busy = 1;

    GPIOA->ODR |= (1 << 11);  // TX mode

    USART1->CR1 |= (1 << 7);  // TXE enable
}

void HandleMasterResponse()
{
    if (!modbus.rx_done) return;

    modbus.rx_done = 0;

    if (!CheckMessage())
    {
        modbus.rx_index = 0;
        memset(modbus.rx_buffer, 0, sizeof(modbus.rx_buffer));
        return;
    }

    uint8_t function = modbus.rx_buffer[1];

    if (function & 0x80)
    {
        uint8_t error_code = modbus.rx_buffer[2];
    }
    else
    {
        switch (function)
        {
            case 0x03:
            {
                uint8_t byte_count = modbus.rx_buffer[2];

                if (modbus.rx_index < (3 + byte_count + 2))
                    break;

                uint8_t num_registers = byte_count / 2;
                for (int i = 0; i < num_registers && i < sizeof(modbus.registers)/2; ++i)
                {
                    uint8_t hi = modbus.rx_buffer[3 + i * 2];
                    uint8_t lo = modbus.rx_buffer[4 + i * 2];
                    modbus.registers[i] = (hi << 8) | lo;
                }

                ApplyRegisters();
                break;
            }


            case 0x10:
            {
              if (modbus.rx_index < 8)
                  break;

              uint16_t start_addr = (modbus.rx_buffer[2] << 8) | modbus.rx_buffer[3];
              uint16_t num_regs   = (modbus.rx_buffer[4] << 8) | modbus.rx_buffer[5];

              break;
            }

        }
    }

    modbus.rx_index = 0;
    memset(modbus.rx_buffer, 0, sizeof(modbus.rx_buffer));
}

void ModbusRequestRead(uint8_t slave_address, uint16_t start_address, uint16_t num_registers)
{
    if (modbus.tx_busy) return;

    modbus.tx_index = 0;
    modbus.tx_busy = 1;

    modbus.tx_buffer[0] = slave_address;
    modbus.tx_buffer[1] = 0x03; // Function code
    modbus.tx_buffer[2] = (start_address >> 8);
    modbus.tx_buffer[3] = start_address;
    modbus.tx_buffer[4] = (num_registers >> 8);
    modbus.tx_buffer[5] = num_registers;

    uint16_t crc = CalculateCRC(modbus.tx_buffer, 6);
    modbus.tx_buffer[6] = crc;
    modbus.tx_buffer[7] = (crc >> 8);

    modbus.tx_length = 8;

    GPIOA->ODR |= (1 << 11);      // TX mode
    USART1->CR1 |= (1 << 7);      // TXE interrupt enable
    
    modbus.request_start_time_ms = GetTickMs();

}

void ModbusRequestWrite(uint8_t slave_address, uint16_t start_address, uint16_t num_registers)
{
    if (modbus.tx_busy) return;

    modbus.tx_index = 0;
    modbus.tx_busy = 1;

    modbus.tx_buffer[0] = slave_address;
    modbus.tx_buffer[1] = 0x10;
    modbus.tx_buffer[2] = (start_address >> 8);
    modbus.tx_buffer[3] = start_address;
    modbus.tx_buffer[4] = (num_registers >> 8);
    modbus.tx_buffer[5] = num_registers;

    modbus.tx_buffer[6] = num_registers * 2; // Byte count

    for (int i = 0; i < num_registers; ++i)
    {
        modbus.tx_buffer[7 + i * 2] = (modbus.registers[start_address + i] >> 8);
        modbus.tx_buffer[8 + i * 2] = modbus.registers[start_address + i];
    }

    uint8_t length_without_crc = 7 + num_registers * 2;
    uint16_t crc = CalculateCRC(modbus.tx_buffer, length_without_crc);
    modbus.tx_buffer[length_without_crc] = crc;
    modbus.tx_buffer[length_without_crc + 1] = (crc >> 8);

    modbus.tx_length = length_without_crc + 2;

    GPIOA->ODR |= (1 << 11);     // TX mode
    USART1->CR1 |= (1 << 7);     // TXE interrupt enable
    
    modbus.request_start_time_ms = GetTickMs();

}


void MainLoop()
{
    if (modbus.role == Slave)
    {
        if (modbus.rx_done)
        {
            modbus.rx_done = 0;

            if (CheckMessage())
            {
                ProcessMessage();
            }

            modbus.rx_index = 0;
            memset(modbus.rx_buffer, 0, sizeof(modbus.rx_buffer));
        }
    }
    
    if (modbus.role == Master)
    {
        HandleMasterResponse();

        uint32_t now = GetTickMs();
        uint32_t elapsed = now - modbus.request_start_time_ms;

        if (modbus.tx_busy && elapsed > 500) 
        {
            modbus.tx_busy = 0;
            modbus.rx_index = 0;
            modbus.rx_done = 0;
            memset(modbus.rx_buffer, 0, sizeof(modbus.rx_buffer));
        }
    }

}



