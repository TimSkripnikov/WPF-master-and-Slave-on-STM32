#include "MyUSART.h"
#include "MyGPIO.h"

struct MyUSART my_usart;

void InitMyUSART()
{
  
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;                                         // turn on Clock Enable of USART
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;                                            // pins for usart
  
  CLEAR_BIT(GPIOA->MODER, GPIO_MODER_MASK(GPIO_Px9_POSITION));
  
  CLEAR_BIT(GPIOA->MODER, GPIO_MODER_MASK(GPIO_Px10_POSITION));
  CLEAR_BIT(GPIOA->MODER, GPIO_MODER_MASK(GPIO_Px11_POSITION));
  
  SET_BIT(GPIOA->MODER, GPIO_MODER_ALTERNATE_FUNCTION(GPIO_Px9_POSITION));          
  SET_BIT(GPIOA->MODER, GPIO_MODER_ALTERNATE_FUNCTION(GPIO_Px10_POSITION));          
  SET_BIT(GPIOA->MODER, GPIO_MODER_OUTPUT(GPIO_Px11_POSITION));           
  
  CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_MASK(GPIO_Px9_POSITION));                // push-pull
  CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_MASK(GPIO_Px10_POSITION));               // push-pull
  CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_MASK(GPIO_Px11_POSITION));               // push-pull
  
  CLEAR_BIT(GPIOA->AFR[1], GPIO_AFR_MASK(1));
  CLEAR_BIT(GPIOA->AFR[1], GPIO_AFR_MASK(2));
  
  SET_BIT(GPIOA->AFR[1], GPIO_AFR_PERIPHERY(AF1, 1));                   // to find what needed, open RM on page 162 and datasheet of MC on page 44 ( Table 15. Alternate functions selected through GPIOA_AFR registers for port A )
  SET_BIT(GPIOA->AFR[1], GPIO_AFR_PERIPHERY(AF1, 2));
  
  
  CLEAR_BIT(USART1_CR1, USART_CR1_M0_MASK);                      // 8 bit
  CLEAR_BIT(USART1_CR1, USART_CR1_M1_MASK);                      // 8 bit
  CLEAR_BIT(USART1_CR2, USART_CR2_STOP_MASK);                    // 1 stop bit
            
            
  USART1_BRR = USART_BRR_VALUE_9600;
  
  SET_BIT(USART1_CR1, USART_CR1_UE_MASK);
  SET_BIT(USART1_CR1, USART_CR1_TE_MASK);
  SET_BIT(USART1_CR1, USART_CR1_RE_MASK);
  SET_BIT(USART1_CR1, USART_CR1_RXNEIE_MASK);
  
  NVIC_ENABLE(NVIC_USART1);
  
  SET_BIT(USART1_CR2, USART_CR2_RTOEN_MASK);
  SET_BIT(USART1_CR1, USART_CR1_RTOIE_MASK);
  
  MODIFY_REG(USART1_RTOR, USART_RTOR_RTO_MASK, RTO_PAUSE_VAL);                     // write 35 in RTOR
  
  
  my_usart.rx_complete = 0;
  
}

void SendUSART() {
  
    if (my_usart.tx_busy) return;

    my_usart.number_of_transmitted_bytes = 0;
    my_usart.tx_busy = 1;

    SET_BIT(GPIOA_ODR, TX_MODE_ENABLE);  

    SET_BIT(USART1_CR1, TXE_ENABLE); 
}

void MyUSART_Handler()
{ 
  // ------------------Receiving---------------------
  //if(my_usart.IsRead) 
  {
    
    if((USART1_ISR & USART_ISR_RXNE_MASK) && (USART1_CR1 & USART_CR1_RXNEIE_MASK)) 
    {
      if (my_usart.number_of_received_bytes < sizeof(my_usart.rx_buffer))
      {
        my_usart.rx_buffer[my_usart.number_of_received_bytes++] = USART1_RDR;
      }
      else
      {
        uint8_t tmp = USART1_RDR;
      }
    }  
    
    if((USART1_ISR & USART_ISR_RTOF_MASK) && (USART1_CR2 & USART_CR2_RTOEN_MASK))
    {
      USART1_ICR |= USART_ISR_RTOF_MASK;
      
      my_usart.rx_complete = 1; 
    }
  }
   
  // ------------------Transmission------------------
  
    if ((USART1_ISR & USART_ISR_TXE_MASK) && (USART1->CR1 & USART_CR1_TXEIE_MASK)) // TXE
    {
      if (my_usart.number_of_transmitted_bytes < my_usart.tx_length)
      {
          USART1_TDR = my_usart.tx_buffer[my_usart.number_of_transmitted_bytes++];
      }
      else
      {
          CLEAR_BIT(USART1_CR1, USART_CR1_TXEIE_MASK);  
          SET_BIT(USART1_CR1, USART_CR1_TCIE_MASK);
      }
    }

    if ((USART1_ISR & USART_ISR_TC_MASK) && (USART1_CR1 & USART_CR1_TCIE_MASK)) // TC
    {
        CLEAR_BIT(USART1_CR1, USART_CR1_TCIE_MASK);     
        
        CLEAR_BIT(GPIOA_ODR, USART_CR1_TX_ENABLE);      
        my_usart.tx_busy = 0;      
    }

}
 



