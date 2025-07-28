#ifndef MY_USART_H
#define MY_USART_H

#include <stdint.h>
#include "stm32f0xx_hal.h"
#include <string.h>

#define USART1_CR1 (USART1->CR1)
#define USART1_CR2 (USART1->CR2)
#define USART1_BRR (USART1->BRR)
#define USART1_RTOR (USART1->RTOR)
#define USART1_RDR (USART1->RDR)
#define USART1_ISR (USART1->ISR)
#define USART1_ICR (USART1->ICR)
#define USART1_TDR (USART1->TDR)

#define USART2_CR1 (USART2->CR1)
#define USART2_CR2 (USART2->CR2)
#define USART2_BRR (USART2->BRR)
#define USART2_RTOR (USART2->RTOR)
#define USART2_RDR (USART2->RDR)
#define USART2_ISR (USART2->ISR)
#define USART2_ICR (USART2->ICR)
#define USART2_TDR (USART2->TDR)

#define USART_CR1_UE_MASK               (0b1U << 0)
#define USART_CR1_UESM_MASK             (0b1U << 1)
#define USART_CR1_RE_MASK               (0b1U << 2)
#define USART_CR1_TE_MASK               (0b1U << 3)
#define USART_CR1_IDLEIE_MASK           (0b1U << 4)
#define USART_CR1_RXNEIE_MASK           (0b1U << 5)
#define USART_CR1_TCIE_MASK             (0b1U << 6)
#define USART_CR1_TXEIE_MASK            (0b1U << 7)
#define USART_CR1_PEIE_MASK             (0b1U << 8)
#define USART_CR1_PS_MASK               (0b1U << 9)
#define USART_CR1_PCE_MASK              (0b1U << 10)
#define USART_CR1_WAKE_MASK             (0b1U << 11)
#define USART_CR1_M0_MASK               (0b1U << 12)
#define USART_CR1_MME_MASK              (0b1U << 13)
#define USART_CR1_CMIE_MASK             (0b1U << 14)
#define USART_CR1_OVER8_MASK            (0b1U << 15)
#define USART_CR1_DEDT_MASK             (0b11111U << 16)
#define USART_CR1_DEAT_MASK             (0b11111U << 21)
#define USART_CR1_RTOIE_MASK            (0b1U << 26)
#define USART_CR1_EOBIE_MASK            (0b1U << 27)
#define USART_CR1_M1_MASK               (0b1U << 28)

#define USART_CR2_ADDM7_MASK            (0b1U << 4)
#define USART_CR2_LBDL_MASK             (0b1U << 5)
#define USART_CR2_LBDIE_MASK            (0b1U << 6)
#define USART_CR2_LBCL_MASK             (0b1U << 8)
#define USART_CR2_CPHA_MASK             (0b1U << 9)
#define USART_CR2_CPOL_MASK             (0b1U << 10)
#define USART_CR2_CLKEN_MASK            (0b1U << 11)
#define USART_CR2_STOP_MASK             (0b11U << 12)
#define USART_CR2_LINEN_MASK            (0b1U << 14)
#define USART_CR2_SWAP_MASK             (0b1U << 15)
#define USART_CR2_RXINV_MASK            (0b1U << 16)
#define USART_CR2_TXINV_MASK            (0b1U << 17)
#define USART_CR2_DATAINV_MASK          (0b1U << 18)
#define USART_CR2_MSBFIRST_MASK         (0b1U << 19)
#define USART_CR2_ABREN_MASK            (0b1U << 20)
#define USART_CR2_ABRMODE_MASK          (0b11U << 21)
#define USART_CR2_RTOEN_MASK            (0b1U << 23)
#define USART_CR2_ADD_MASK              (0xFFU << 24)

// ------------------ USART Configuration Constants --------------------------------------
#define F_8MH                           8000000UL
#define BAUDE_RATE_9600                 9600
#define USART_BRR_VALUE_9600            (F_8MH / BAUDE_RATE_9600)

#define NVIC_ISER0                      (NVIC->ISER[0])
#define NVIC_USART1                     27
#define NVIC_ENABLE(n)                  (NVIC_ISER0 |= (1 << (n)))

// ------------------ Receiver Timeout ---------------------------------------------------
#define USART_RTOR_RTO_MASK             0x00FFFFFFFU
#define RTO_PAUSE_VAL                   0x23                    // 35 


#define USART_CR1_TX_ENABLE             (0b1U << 11)
#define TXE_ENABLE                      (0b1U << 7)
#define TX_MODE_ENABLE                  (0b1U << 11)


#define USART_ISR_PE_MASK               (0b1U << 0)
#define USART_ISR_FE_MASK               (0b1U << 1)
#define USART_ISR_NE_MASK               (0b1U << 2)
#define USART_ISR_ORE_MASK              (0b1U << 3)
#define USART_ISR_IDLE_MASK             (0b1U << 4)
#define USART_ISR_RXNE_MASK             (0b1U << 5)
#define USART_ISR_TC_MASK               (0b1U << 6)
#define USART_ISR_TXE_MASK              (0b1U << 7)
#define USART_ISR_LBD_MASK              (0b1U << 8)
#define USART_ISR_CTSIF_MASK            (0b1U << 9)
#define USART_ISR_CTS_MASK              (0b1U << 10)
#define USART_ISR_RTOF_MASK             (0b1U << 11)
#define USART_ISR_EOBF_MASK             (0b1U << 12)
#define USART_ISR_ABRE_MASK             (0b1U << 14)
#define USART_ISR_ABRF_MASK             (0b1U << 15)
#define USART_ISR_BUSY_MASK             (0b1U << 16)
#define USART_ISR_CMF_MASK              (0b1U << 17)
#define USART_ISR_SBKF_MASK             (0b1U << 18)
#define USART_ISR_RWU_MASK              (0b1U << 19)
#define USART_ISR_WUF_MASK              (0b1U << 20)
#define USART_ISR_TEACK_MASK            (0b1U << 21)
#define USART_ISR_REACK_MASK            (0b1U << 22)

// ------------------ MyUSART State Structure ---------------------------------------------
// Structure holding TX/RX state and buffers for USART
struct MyUSART {
  uint8_t tx_buffer[256];
  uint8_t rx_buffer[256];
  
  uint8_t number_of_transmitted_bytes;
  uint8_t number_of_received_bytes;
  
  uint8_t rx_complete;
  uint8_t tx_busy;
  
  uint8_t tx_length;
  
  uint8_t IsRead;
};

// ------------------ Functions ------------------------------------------------------------
// Initializes USART1 and related GPIO/interrupts/settings
void InitMyUSART(void);

// Starts transmission of tx_buffer via USART
void SendUSART(void);

// Main USART interrupt handler to manage TX/RX 
void MyUSART_Handler(void);

#endif