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

#define SH1106_SLAVE_ADDRESS                            (0x3C << 1)

// -------------------------------------------------- Control Bytes -----------------------------------------------------
// To understand this control bytes see page 14 of SH1106 documentation:
#define SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_COMMAND   0x80
#define SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_RAM       0xC0
#define SH1106_CTRL_BYTE_ONLY_DATA_FOR_COMMAND          0x00   
#define SH1106_CTRL_BYTE_ONLY_DATA_FOR_RAM              0x40   

// -------------------------------------------------- Display commands --------------------------------------------------
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

// -------------------------------------------------- Display parameters --------------------------------------------------
#define SH1106_WIDTH                                    128
#define SH1106_HEIGHT                                   64
#define DMA_PAGE_BUFFER_SIZE                            (SH1106_WIDTH + 7)

// -------------------------------------------------- Pixel color ---------------------------------------------------------
typedef enum {
    SH1106_COLOR_BLACK = 0x00,
    SH1106_COLOR_WHITE = 0x01
} SH1106_COLOR_t;

// -------------------------------------------------- Current State -------------------------------------------------------
typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
} SH1106_t;

// -------------------------------------------------- Variables and functions ---------------------------------------------
extern SH1106_t SH1106;
extern uint8_t tx_in_process;
extern uint8_t tx_complete_flag;

extern uint8_t frame_buffer[SH1106_WIDTH  * SH1106_HEIGHT / 8];
extern uint8_t dma_page_buffer[DMA_PAGE_BUFFER_SIZE];

void SH1106_Init(void);                                                                                 // Initialize the SH1106 display
void SH1106_Init_Page_Buffer(void);                                                                     // Writing of control bytes  

void SH1106_I2C_Update_Page_Buffer(int page);                                                           // Updating 1 page: write bytes from frame_buffer to dma_page_buffer
void SH1106_I2C_Update_Frame_loop(void);
void SH1106_Start_Update(void);

void Convert_Bitmap_To_Pages(const uint8_t* src, uint8_t* dst, uint16_t width, uint16_t height);        // Convert bitmap to SH1106 page format

void SH1106_Clear_Frame_Buffer(void);                                                                   // Clear the frame buffer
void SH1106_DrawPixel(uint16_t x, uint16_t y, SH1106_COLOR_t color);                                    // Draw a single pixel at (x,y)

char SH1106_Putc(char ch, FontDef_t* Font, SH1106_COLOR_t color);                                       // Draw a single character
char SH1106_Puts(char* str, FontDef_t* Font, SH1106_COLOR_t color);                                     // Draw a string of text

// -------------------------------------------------- STRUCTURES, VARIABLES, FUNCTIONS FOR MENU ---------------------------

// -------------------------------------------------- Base structure for screen -------------------------------------------

typedef enum {
  BUTTON_NONE,
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_ENTER,
  BUTTON_EXIT
} Button_t;

typedef struct {
  void (*Render_Screen_on_Frame_Buffer)(void);
  void (*Handle_Button)(void);
} Screen_t;

extern Button_t current_button;

extern Screen_t *current_screen;

void Switch_Screens_Loop(void);

void Init_States_of_Screens(void);

// -------------------------------------------------- Menu Screen -------------------------------------------------------------

void Menu_Render(void);
void Menu_Handle(void);

typedef enum {
  PASSWORD,
  ADDRESS,
  SPEED,
  MENU_RESET
} Position_in_Menu_t;

#define NUM_MENU_ITEMS 4

extern Position_in_Menu_t current_position_in_menu;

extern Screen_t Menu_Screen;

// -------------------------------------------------- Password Screen ----------------------------------------------------------

typedef enum {
  NO_SCREEN,
  ENTER_POSITION,
  ENTER_NUMBER
} State_on_Screen_t;

void Password_Render(void);
void Password_Handle(void);

#define PASSWORD_LENGTH 8

#define ACTUAL_PASSWORD 00000000

extern int password[PASSWORD_LENGTH];

extern Screen_t Password_Screen;

uint8_t Check_Password(void);

// -------------------------------------------------- Enter_Correct_Password Screen ---------------------------------------------

void Enter_Correct_Password_Render(void);
void Enter_Correct_Password_Handle(void); 

extern Screen_t Enter_Correct_Password_Screen;

// -------------------------------------------------- Address Screen ------------------------------------------------------------

void Address_Render(void);
void Address_Handle(void);

extern uint8_t address;
extern Screen_t Address_Screen;

#define NUMBER_OF_ADDRESSES 10

// -------------------------------------------------- Speed Screen --------------------------------------------------------------

void Speed_Render(void);
void Speed_Handle(void);

typedef enum {
    BAUD_1200    = 1200,
    BAUD_2400    = 2400,
    BAUD_4800    = 4800,
    BAUD_9600    = 9600,
    BAUD_19200   = 19200,
    BAUD_38400   = 38400,
    BAUD_57600   = 57600,
    BAUD_115200  = 115200
} Baud_Rate_t;

#define NUM_BAUD_RATES 8

extern Baud_Rate_t baud_rates[NUM_BAUD_RATES];

extern Baud_Rate_t current_speed;
extern Screen_t Speed_Screen;

// -------------------------------------------------- Welcome Screen ------------------------------------------------------------

void Welcome_Render(void);
void Welcome_Handle(void);

void Splash_Screen_Loop(void);

extern uint32_t splash_start_time;
extern uint8_t splash_drawn;

extern Screen_t Welcome_Screen;

//  -------------------------------------------------- Information Screen  ------------------------------------------------------

void Information_Render(void);
void Information_Handle(void);

extern Screen_t Information_Screen;

#endif 
