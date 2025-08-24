#include "MySH1106.h"
#include "fonts.h"
#include "MyUSART.h"                                                            // for Update BRR
#include "MyModbusRtu.h"                                                        // for Update Slave Address

SH1106_t SH1106;

uint8_t tx_complete_flag = 0;
uint8_t tx_in_process = 0;

int current_page = 0;

uint8_t frame_buffer[SH1106_WIDTH  * SH1106_HEIGHT / 8];

uint8_t dma_page_buffer[SH1106_WIDTH + 7];

uint8_t init_array[] = {
    SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_COMMAND, SH1106_DISPLAY_OFF,
    SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_COMMAND, (SH1106_SET_PAGE_START_ADDRESS | 0x00),
    SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_COMMAND, SH1106_SET_CONTRAST,
    SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_COMMAND, 0xFF,
    SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_COMMAND, SH1106_SET_DC_DC_CONTROL_MODE,
    SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_COMMAND, 0x8B,
    SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_COMMAND, SH1106_SET_PUMP_VOLTAGE,
    SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_COMMAND, SH1106_DISPLAY_ON
};

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2) 
    {
        tx_complete_flag = 1;   
    }
}

void SH1106_Init_Page_Buffer() 
{
  uint8_t col_offset = 2; 
  
  dma_page_buffer[0] = SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_COMMAND;

  dma_page_buffer[2] = SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_COMMAND;
  dma_page_buffer[3] = (SH1106_SET_LOWER_COLUMN_ADDRESS | ((0 + col_offset) & 0x0F));

  dma_page_buffer[4] = SH1106_CTRL_BYTE_DATA_AND_CONTROL_FOR_COMMAND;
  dma_page_buffer[5] = (SH1106_SET_HIGHER_COLUMN_ADDRESS | ((0 + col_offset) >> 4));

  dma_page_buffer[6] = SH1106_CTRL_BYTE_ONLY_DATA_FOR_RAM;
}

void SH1106_Init() {
   HAL_I2C_Master_Transmit(I2C_2, SH1106_SLAVE_ADDRESS, init_array, sizeof(init_array), 5);
   SH1106_Init_Page_Buffer();
}

void SH1106_I2C_Update_Page_Buffer(int page)
{
  dma_page_buffer[1] = (SH1106_SET_PAGE_START_ADDRESS | page);

  memcpy(&dma_page_buffer[7], &frame_buffer[page * SH1106_WIDTH], SH1106_WIDTH);
}

void SH1106_I2C_Update_Frame_loop()
{
  if(tx_complete_flag && tx_in_process)
  {
    tx_complete_flag = 0;
    
    SH1106_I2C_Update_Page_Buffer(current_page);
    
    HAL_I2C_Master_Transmit_DMA(I2C_2, SH1106_SLAVE_ADDRESS, dma_page_buffer, SH1106_WIDTH + 7);
  
    current_page++;
    
    if(current_page >= 8)
    {
      current_page = 0;
      tx_in_process = 0;
    }
  }
}

void SH1106_Start_Update()
{
    current_page = 0;
    tx_in_process = 1;
    tx_complete_flag = 1;
}

void Convert_Bitmap_To_Pages(const uint8_t* src, uint8_t* dst, uint16_t width, uint16_t height) {
    uint16_t pages = height / 8;
    uint16_t byteWidth = (width + 7) / 8;

    for (uint16_t page = 0; page < pages; page++) {
        for (uint16_t x = 0; x < width; x++) {
            uint8_t byte = 0;
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint16_t y = page * 8 + bit;
                uint16_t srcIndex = y * byteWidth + x / 8;
                uint8_t srcBit = 7 - (x % 8);

                uint8_t pixel = (src[srcIndex] >> srcBit) & 0x01;
                byte |= (pixel << bit);
            }
            dst[page * width + x] = byte;
        }
    }
}

void SH1106_Clear_Frame_Buffer()
{
  for(int page = 0; page < 8; page++)
  {
    for(int byte = 0; byte < 128; byte++){ 
      frame_buffer[page * 128 + byte] = 0;
    }
  }
}

void SH1106_DrawPixel(uint16_t x, uint16_t y, SH1106_COLOR_t color) {
    if (x >= SH1106_WIDTH || y >= SH1106_HEIGHT) return;

    if (color == SH1106_COLOR_WHITE) {
        frame_buffer[x + (y / 8) * SH1106_WIDTH] |= 1 << (y % 8);
    } else {
        frame_buffer[x + (y / 8) * SH1106_WIDTH] &= ~(1 << (y % 8));
    }
}

char SH1106_Putc(char ch, FontDef_t* Font, SH1106_COLOR_t color) {
    if (SH1106_WIDTH <= (SH1106.CurrentX + Font->FontWidth) || SH1106_HEIGHT <= (SH1106.CurrentY + Font->FontHeight)) {
        return 0;
    }

    for (uint32_t i = 0; i < Font->FontHeight; i++) {
        uint16_t b = Font->data[(ch - 32) * Font->FontHeight + i];
        for (uint32_t j = 0; j < Font->FontWidth; j++) {
            if ((b << j) & 0x8000) {
                SH1106_DrawPixel(SH1106.CurrentX + j, SH1106.CurrentY + i, color);
            } else {
                SH1106_DrawPixel(SH1106.CurrentX + j, SH1106.CurrentY + i, (SH1106_COLOR_t)!color);
            }
        }
    }

    SH1106.CurrentX += Font->FontWidth;
    return ch;
}

char SH1106_Puts(char* str, FontDef_t* Font, SH1106_COLOR_t color) {
    while (*str) {
        if (SH1106_Putc(*str, Font, color) != *str) {
            return *str;
        }
        str++;
    }
    return 0;
}

void SH1106_GotoXY(uint16_t x, uint16_t y) {
	SH1106.CurrentX = x;
	SH1106.CurrentY = y;
}

void SH1106_DrawHorizontalLine(uint16_t length, SH1106_COLOR_t color)
{
  uint16_t x = 0;
  
    for (uint16_t i = 0; i < length; i++)
    {
        x = SH1106.CurrentX + i;
        
        if (x >= SH1106_WIDTH) break; 
        
        SH1106_DrawPixel(x, SH1106.CurrentY, color);
    }
    
}

void SH1106_DrawUpArrow(SH1106_COLOR_t color)
{
    uint16_t x = SH1106.CurrentX;
    
    uint16_t y = SH1106.CurrentY;
    
    SH1106_DrawPixel(x, y, color);

    for(int i = -1; i <= 1; i++)
        SH1106_DrawPixel(x + i, y + 1, color);

    for(int i = -2; i <= 2; i++)
        SH1106_DrawPixel(x + i, y + 2, color);
}


// -------------------------------------------------- Base Screen -------------------------------------------------------------

Button_t current_button = BUTTON_NONE;

Screen_t *current_screen;

void Switch_Screens_Loop() 
{
  Splash_Screen_Loop();
  
  if(current_button != BUTTON_NONE)
  {
    current_screen->Handle_Button();
    current_screen->Render_Screen_on_Frame_Buffer();
    SH1106_Start_Update();
  }
}

  
// -------------------------------------------------- Menu Screen -------------------------------------------------------------
 
Position_in_Menu_t current_position_in_menu = PASSWORD;

Screen_t Menu_Screen = {
  .Render_Screen_on_Frame_Buffer = Menu_Render,
  .Handle_Button = Menu_Handle
};

void Menu_Handle()
{
  switch(current_button)
  {
  case BUTTON_DOWN:
    current_position_in_menu = (current_position_in_menu + NUM_MENU_ITEMS - 1) % NUM_MENU_ITEMS;
    break;
  case BUTTON_UP:
    current_position_in_menu = (current_position_in_menu + 1) % NUM_MENU_ITEMS;
    break;
  case BUTTON_ENTER:
    {
    switch(current_position_in_menu) 
    {
    case PASSWORD:
      current_screen = &Password_Screen;
      break;
    case ADDRESS:
      if(Check_Password() == 0) { current_screen = &Address_Screen; }
      else { current_screen = &Enter_Correct_Password_Screen; }
      break;
    case SPEED:
      if(Check_Password() == 0) { current_screen = &Speed_Screen; }
      else { current_screen = &Enter_Correct_Password_Screen; }
      break;
    case MENU_RESET:
      NVIC_SystemReset();
      break;
    }
    break;
    }
  case BUTTON_EXIT:
    current_screen = & Information_Screen;
    break;
  }
  
  current_button = BUTTON_NONE;
}

void Menu_Render() 
{
  SH1106_Clear_Frame_Buffer();
  
  SH1106_GotoXY(10, 0);
  SH1106_Puts("Password", &Font_7x10, SH1106_COLOR_WHITE);
  
  SH1106_GotoXY(10, 15);
  SH1106_Puts("Address", &Font_7x10, SH1106_COLOR_WHITE);
  
  SH1106_GotoXY(10, 30);
  SH1106_Puts("Speed", &Font_7x10, SH1106_COLOR_WHITE);
  
  SH1106_GotoXY(10, 45);
  SH1106_Puts("Reset", &Font_7x10, SH1106_COLOR_WHITE);
  
  switch(current_position_in_menu) 
  {
  case PASSWORD:
    SH1106_GotoXY(0, 0);
    SH1106_Puts(">", &Font_7x10, SH1106_COLOR_WHITE);
    break;
  case ADDRESS:
    SH1106_GotoXY(0, 15);
    SH1106_Puts(">", &Font_7x10, SH1106_COLOR_WHITE);
    break;
  case SPEED:
    SH1106_GotoXY(0, 30);
    SH1106_Puts(">", &Font_7x10, SH1106_COLOR_WHITE);
    break;
  case MENU_RESET:
    SH1106_GotoXY(0, 45);
    SH1106_Puts(">", &Font_7x10, SH1106_COLOR_WHITE);
    break;
    
  }
}

// -------------------------------------------------- Password Screen ----------------------------------------------------------

Screen_t Password_Screen = {
  .Render_Screen_on_Frame_Buffer = Password_Render,
  .Handle_Button = Password_Handle
};

uint8_t current_position_password = 0;

int password[PASSWORD_LENGTH] = {0, 0, 0, 0, 0, 0, 0, 0};

State_on_Screen_t password_state = ENTER_POSITION;

void Password_Handle()
{
  if(password_state == ENTER_POSITION){
    switch(current_button) 
    {
    case BUTTON_UP:
      current_position_password = (current_position_password + 1) % PASSWORD_LENGTH;
      break;
    case BUTTON_DOWN:
      current_position_password = (current_position_password + PASSWORD_LENGTH - 1) % PASSWORD_LENGTH;
      break;
    case BUTTON_ENTER:
      password_state = ENTER_NUMBER;
      break;
    case BUTTON_EXIT:
      current_screen = &Menu_Screen;
      current_position_password = 0;
      break;
    }
    current_button = BUTTON_NONE;
  }
  
  else if(password_state == ENTER_NUMBER) {
    switch(current_button)
    {
    case BUTTON_UP:
      password[current_position_password] = (password[current_position_password] + 1) % 10;
      break;
    case BUTTON_DOWN:
      password[current_position_password] = (password[current_position_password] + 10 - 1) % 10;
      break;
    case BUTTON_ENTER:
    case BUTTON_EXIT:
      password_state = ENTER_POSITION;
      break;
    }
    current_button = BUTTON_NONE;
  }
}

void Password_Render() 
{
   SH1106_Clear_Frame_Buffer();
   
  //if(password_state == NO_SCREEN)
  //{
    SH1106_GotoXY(0, 0);
    SH1106_Puts("Password:", &Font_7x10, SH1106_COLOR_WHITE);
    //password_state = ENTER_POSITION;
  //}
  
  SH1106_GotoXY(0, 15);
  
  char pass_buffer[PASSWORD_LENGTH + 1];

  for(int position = 0; position < PASSWORD_LENGTH; position++)
  {
      pass_buffer[position] = '0' + (char)password[position];
  }

  pass_buffer[PASSWORD_LENGTH] = '\0';
  
  SH1106_Puts(pass_buffer, &Font_7x10, SH1106_COLOR_WHITE);
  
 if(password_state == ENTER_NUMBER) 
 {
   SH1106_GotoXY(current_position_password * Font_7x10.FontWidth, 26);
                 
   SH1106_DrawHorizontalLine(Font_7x10.FontWidth, SH1106_COLOR_WHITE);
 }
 else if(password_state == ENTER_POSITION) 
 {
   SH1106_GotoXY(current_position_password * Font_7x10.FontWidth + 3, 24);
   
   SH1106_DrawUpArrow(SH1106_COLOR_WHITE); 
   
   // SH1106_Puts("~", &Font_7x10, SH1106_COLOR_WHITE);
 }

}

uint8_t Check_Password(void)
{
    uint32_t entered_password = 0;

    for (int i = 0; i < PASSWORD_LENGTH; i++)
    {
        entered_password = entered_password * 10 + (uint32_t)password[i];
    }

    if (entered_password == ACTUAL_PASSWORD)
    {
        return 0; 
    }
    else
    {
        return 1;
    }
}

// -------------------------------------------------- Enter_Correct_Password Screen ---------------------------------------------

Screen_t Enter_Correct_Password_Screen = {
  .Render_Screen_on_Frame_Buffer = Enter_Correct_Password_Render,
  .Handle_Button = Enter_Correct_Password_Handle
};

void Enter_Correct_Password_Render()
{
  SH1106_Clear_Frame_Buffer();

  SH1106_GotoXY(4, 15);
  SH1106_Puts("Enter the correct", &Font_7x10, SH1106_COLOR_WHITE);
  
  SH1106_GotoXY(32, 30);
  
  SH1106_Puts("PASSWORD!", &Font_7x10, SH1106_COLOR_WHITE);
}

void Enter_Correct_Password_Handle()
{
  switch(current_button)
  {
  case BUTTON_DOWN:
  case BUTTON_UP:
  case BUTTON_ENTER:
  case BUTTON_EXIT:
    current_screen = &Menu_Screen;
    break;
  }
  
  current_button = BUTTON_NONE;
}
  

// -------------------------------------------------- Address Screen ------------------------------------------------------------

  Screen_t Address_Screen = {
  .Render_Screen_on_Frame_Buffer = Address_Render,
  .Handle_Button = Address_Handle
};

uint8_t address = 0;

void Address_Handle()
{
  switch(current_button)
  {
  case BUTTON_UP:
    address = (address + 1) % NUMBER_OF_ADDRESSES;
    break;
  case BUTTON_DOWN:
    address = (address + NUMBER_OF_ADDRESSES - 1) % NUMBER_OF_ADDRESSES;
    break;
  case BUTTON_ENTER:
  case BUTTON_EXIT:
    current_screen = &Menu_Screen;
    UpdateSlaveAddress(address);
  }
  
  current_button = BUTTON_NONE;
}



void Address_Render()
{
  
  SH1106_Clear_Frame_Buffer();

  SH1106_GotoXY(0, 0);
  SH1106_Puts("Address:", &Font_7x10, SH1106_COLOR_WHITE);
  
  SH1106_GotoXY(0, 15);
 
  char address_buffer[7];

  sprintf(address_buffer, "%d", address);
  
  SH1106_Puts(address_buffer, &Font_7x10, SH1106_COLOR_WHITE);
  
}

// -------------------------------------------------- Speed Screen --------------------------------------------------------------

Baud_Rate_t baud_rates[NUM_BAUD_RATES] = {
    BAUD_1200,
    BAUD_2400,
    BAUD_4800,
    BAUD_9600,
    BAUD_19200,
    BAUD_38400,
    BAUD_57600,
    BAUD_115200
};
  
Baud_Rate_t current_speed = 3;                  // 9600 default

Screen_t Speed_Screen = {
  .Render_Screen_on_Frame_Buffer = Speed_Render,
  .Handle_Button = Speed_Handle
};

void Speed_Handle()
{
  switch(current_button)
  {
  case BUTTON_DOWN:
    current_speed = (current_speed - 1 + NUM_BAUD_RATES) % NUM_BAUD_RATES;
    break;
  case BUTTON_UP:
    current_speed = (current_speed + 1) % NUM_BAUD_RATES;
    break;
  case BUTTON_ENTER:
  case BUTTON_EXIT:
    current_screen = &Menu_Screen;
    Update_Speed_BRR_Value(baud_rates[current_speed]);
    break;
  }
  
  current_button = BUTTON_NONE;
}

void Speed_Render()
{
    char buffer[16]; 
    
    SH1106_Clear_Frame_Buffer();

    SH1106_GotoXY(0, 0);
    SH1106_Puts("Speed (bps):", &Font_7x10, SH1106_COLOR_WHITE);
    SH1106_GotoXY(0, 15);

    sprintf(buffer, "%d", baud_rates[current_speed]);

    SH1106_Puts(buffer, &Font_7x10, SH1106_COLOR_WHITE);
}

// -------------------------------------------------- Welcome Screen ------------------------------------------------------------

uint32_t splash_start_time = 0;
uint8_t splash_drawn = 0;

Screen_t Welcome_Screen = {
  .Render_Screen_on_Frame_Buffer = Welcome_Render,
  .Handle_Button = Welcome_Handle
};

void Splash_Screen_Loop()
{
  if(!splash_drawn)
  {
    splash_start_time = HAL_GetTick();
    splash_drawn = 1;
  }
  
  if((HAL_GetTick() - splash_start_time >= WELCOME_PAUSE) && splash_drawn != 2)
  {
    current_screen = &Information_Screen;

    current_screen->Render_Screen_on_Frame_Buffer();
    SH1106_Start_Update();
    splash_drawn = 2;
    
  } 
}

void Welcome_Handle()
{
  return;
}

void Welcome_Render()
{
  SH1106_Clear_Frame_Buffer();
  
  SH1106_GotoXY(10, 0);
  SH1106_Puts("Name Company", &Font_7x10, SH1106_COLOR_WHITE);
  
  SH1106_GotoXY(10, 15);
  SH1106_Puts("Hardware: v1.0", &Font_7x10, SH1106_COLOR_WHITE);
  
  SH1106_GotoXY(10, 30);
  SH1106_Puts("Software: v1.0", &Font_7x10, SH1106_COLOR_WHITE);
}

//  -------------------------------------------------- Information Screen  ------------------------------------------------------
  
Screen_t Information_Screen = {
  .Render_Screen_on_Frame_Buffer = Information_Render,
  .Handle_Button = Information_Handle
};

void Information_Render(void)
{
  SH1106_Clear_Frame_Buffer();
  
  SH1106_GotoXY(10, 0);
  SH1106_Puts("Some Information", &Font_7x10, SH1106_COLOR_WHITE);
}

void Information_Handle()
{
  switch(current_button)
  {
  case BUTTON_ENTER:
    current_screen = &Menu_Screen;
    break;
  case BUTTON_DOWN:
  case BUTTON_UP:
  case BUTTON_EXIT:
    break;
  }
  
  current_button = BUTTON_NONE;
}

   

  
  
  
  
  
  
  
  
  
  
  
  
  