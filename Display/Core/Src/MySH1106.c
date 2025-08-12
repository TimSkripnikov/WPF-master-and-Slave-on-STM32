#include "MySH1106.h"
#include "fonts.h"

SH1106_t SH1106;

uint8_t SH1106_Buffer[SH1106_WIDTH  * SH1106_HEIGHT / 8];

uint8_t image_buffer[SH1106_WIDTH * SH1106_HEIGHT / 8];

uint8_t clear_buffer[BUFFER_WIDTH * SH1106_HEIGHT / 8];

void SH1106_Write_Array(uint8_t address, uint8_t control_byte, uint8_t* data, uint16_t length) {
    if (length + 1 > sizeof(uint8_t) * 256) return;

    uint8_t tmp[256];
    tmp[0] = control_byte;

    for(uint16_t i = 0; i < length; i++)   
        tmp[i+1] = data[i];

    HAL_I2C_Master_Transmit(I2C_2, address, tmp, length + 1, 100);
}

void SH1106_I2C_Write_Single(uint8_t address, uint8_t control_byte, uint8_t data) {
    uint8_t tmp[2] = {control_byte, data};
    HAL_I2C_Master_Transmit(I2C_2, address, tmp, 2, 100);
}

void SH1106_Init() {
    SH1106_I2C_Write_Single(SH1106_SLAVE_ADDRESS, SH1106_CONTROL_BYTE_COMMAND, SH1106_DISPLAY_OFF);
    
    SH1106_I2C_Write_Single(SH1106_SLAVE_ADDRESS, SH1106_CONTROL_BYTE_COMMAND, SH1106_SET_PAGE_START_ADDRESS | 0x00);

    SH1106_I2C_Write_Single(SH1106_SLAVE_ADDRESS, SH1106_CONTROL_BYTE_COMMAND, SH1106_SET_CONTRAST);
    SH1106_I2C_Write_Single(SH1106_SLAVE_ADDRESS, SH1106_CONTROL_BYTE_COMMAND, 0xFF);
    
    SH1106_I2C_Write_Single(SH1106_SLAVE_ADDRESS, SH1106_CONTROL_BYTE_COMMAND, SH1106_SET_DC_DC_CONTROL_MODE);
    SH1106_I2C_Write_Single(SH1106_SLAVE_ADDRESS, SH1106_CONTROL_BYTE_COMMAND, 0x8B);  
    
    SH1106_I2C_Write_Single(SH1106_SLAVE_ADDRESS, SH1106_CONTROL_BYTE_COMMAND, SH1106_SET_PUMP_VOLTAGE);

    SH1106_I2C_Write_Single(SH1106_SLAVE_ADDRESS, SH1106_CONTROL_BYTE_COMMAND, SH1106_DISPLAY_ON);
}

void SH1106_Write_Page(uint8_t page, uint8_t column, uint8_t *data, uint8_t length)
{
  uint8_t col_offset = 2;  
  
  SH1106_I2C_Write_Single(SH1106_SLAVE_ADDRESS, SH1106_CONTROL_BYTE_COMMAND, (SH1106_SET_PAGE_START_ADDRESS | page));
  SH1106_I2C_Write_Single(SH1106_SLAVE_ADDRESS, SH1106_CONTROL_BYTE_COMMAND, (SH1106_SET_LOWER_COLUMN_ADDRESS | ((column + col_offset) & 0x0F)));
  SH1106_I2C_Write_Single(SH1106_SLAVE_ADDRESS, SH1106_CONTROL_BYTE_COMMAND, (SH1106_SET_HIGHER_COLUMN_ADDRESS | ((column + col_offset) >> 4)));
  
  SH1106_Write_Array(SH1106_SLAVE_ADDRESS, SH1106_CONTROL_BYTE_DATA, data, length);
}


void SH1106_UpdateScreen(uint8_t *framebuffer)
{
    for (uint8_t page = 0; page < 8; page++) {
        SH1106_Write_Page(page, 0, &framebuffer[page * SH1106_WIDTH], SH1106_WIDTH);
    }
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

void SH1106_Clear()
{
    uint32_t total = BUFFER_WIDTH * (SH1106_HEIGHT / 8);
    for (uint32_t i = 0; i < total; i++) {
        clear_buffer[i] = 0x00;
    }

    for (uint8_t page = 0; page < 8; page++) {
        SH1106_Write_Page(page, 0, &clear_buffer[page * BUFFER_WIDTH], BUFFER_WIDTH);
    }
}

void SH1106_Display_Image()
{
    for (uint8_t page = 0; page < 8; page++) {
        SH1106_Write_Page(page, 0, &image_buffer[page * SH1106_WIDTH], SH1106_WIDTH);
    }
}

void SH1106_DrawPixel(uint16_t x, uint16_t y, SH1106_COLOR_t color) {
    if (x >= SH1106_WIDTH || y >= SH1106_HEIGHT) return;

    if (color == SH1106_COLOR_WHITE) {
        SH1106_Buffer[x + (y / 8) * SH1106_WIDTH] |= 1 << (y % 8);
    } else {
        SH1106_Buffer[x + (y / 8) * SH1106_WIDTH] &= ~(1 << (y % 8));
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
