#ifndef MY_SH1106_H
#define MY_SH1106_H

#include <stdint.h>
#include "stm32f0xx_hal.h"
#include <string.h>
#include "main.h"
#include "stdlib.h"
#include "string.h"
#include "fonts.h"

extern I2C_HandleTypeDef hi2c2;
#define I2C_2 &hi2c2

#define SH1106_SLAVE_ADDRESS            (0x3C << 1)

#define SH1106_CONTROL_BYTE_COMMAND     0x00
#define SH1106_CONTROL_BYTE_DATA        0x40

#define SH1106_SET_LOWER_COLUMN_ADDRESS                 0x00 
#define SH1106_SET_HIGHER_COLUMN_ADDRESS                0x10
#define SH1106_SET_PUMP_VOLTAGE                         0x32  // A1=1, A2=0, Vpp=8.0V (Power on)
#define SH1106_SET_DISPLAY_START_LINE                   0x40  // _ | address (6 bits)
#define SH1106_SET_CONTRAST                             0x81
#define SH1106_SET_SEGMENT_REMAP                        0xA0  // bits[0] - ADC bit: 0 - normal, 1 - reverse
#define SH1106_SET_ENTIRE_DISPLAY_OFF_ON                0xA4  // bits[0] - D bit: When D = “L”, the normal display status is provided. (POR). When D = “H”, the entire display ON status is provided. 
#define SH1106_SET_NORMAL_REVERSE_DISPLAY               0xA6  // bits[0] - D bit: When D = “L”, the RAM data is high, being OLED ON potential (normal display). (POR). When D = “H”, the RAM data is low, being OLED ON potential (reverse display)
#define SH1106_SET_MULTIPLEX_RATIO                      0xA8  // (Double Bytes Command)
#define SH1106_SET_DC_DC_CONTROL_MODE                   0xAD  // (Double Bytes Command)

#define SH1106_DISPLAY_OFF                              0xAE
#define SH1106_DISPLAY_ON                               0xAF

#define SH1106_SET_PAGE_START_ADDRESS                   0xB0

#define SH1106_SET_COMMON_OUTPUT_SCAN_DIR               0xC0
#define SH1106_SET_COMMON_OUTPUT_SCAN_DIR_REVERSE       0xC8

#define SH1106_SET_DISPLAY_OFFSET                       0xD3 // (Double Bytes Command)
#define SH1106_SET_DISPLAY_CLOCK_DIVIDE                 0xD5 // (Double Bytes Command)
#define SH1106_SET_PRECHARGE                            0xD9 // (Double Bytes Command)
#define SH1106_SET_COM_PINS                             0xDA // (Double Bytes Command)
#define SH1106_SET_VCOMH                                0xDB // (Double Bytes Command)

#define SH1106_READ_MODIFY_WRITE_START                  0xE0
#define SH1106_READ_MODIFY_WRITE_END                    0xEE

#define SH1106_NOP                                      0xE3
#define SH1106_WRITE_DISPLAY_DATA                       0xC0
#define SH1106_READ_STATUS                              0xF0
#define SH1106_READ_DISPLAY_DATA                        0xF1

#define SH1106_WIDTH 128
#define SH1106_HEIGHT 64
#define BUFFER_WIDTH 132

typedef enum {
    SH1106_COLOR_BLACK = 0x00,
    SH1106_COLOR_WHITE = 0x01
} SH1106_COLOR_t;

typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
} SH1106_t;

extern SH1106_t SH1106;

extern uint8_t SH1106_Buffer[SH1106_WIDTH * SH1106_HEIGHT / 8];
extern uint8_t image_buffer[SH1106_WIDTH * SH1106_HEIGHT / 8];
extern uint8_t clear_buffer[BUFFER_WIDTH * SH1106_HEIGHT / 8];

void SH1106_Write_Array(uint8_t address, uint8_t control_byte, uint8_t* data, uint16_t length);         // Send multiple bytes to SH1106 via I2C
void SH1106_I2C_Write_Single(uint8_t address, uint8_t control_byte, uint8_t data);                      // Send a single byte to SH1106 via I2C
void SH1106_Init(void);                                                                                 // Initialize the SH1106 display
void SH1106_Write_Page(uint8_t page, uint8_t column, uint8_t *data, uint8_t length);                    // Write data to a specific page and column
void SH1106_UpdateScreen(uint8_t *framebuffer);                                                         // Refresh the screen with framebuffer content
void Convert_Bitmap_To_Pages(const uint8_t* src, uint8_t* dst, uint16_t width, uint16_t height);        // Convert bitmap to SH1106 page format
void SH1106_Clear(void);                                                                                // Clear the display buffer
void SH1106_Display_Image(void);                                                                        // Show image from buffer on the screen
void SH1106_DrawPixel(uint16_t x, uint16_t y, SH1106_COLOR_t color);                                    // Draw a single pixel at (x,y)

char SH1106_Putc(char ch, FontDef_t* Font, SH1106_COLOR_t color);                                       // Draw a single character
char SH1106_Puts(char* str, FontDef_t* Font, SH1106_COLOR_t color);                                     // Draw a string of text


#endif 
