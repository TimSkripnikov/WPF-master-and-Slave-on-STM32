#ifndef FONTS_H
#define FONTS_H 

#include <stdint.h>

typedef struct {
	uint8_t FontWidth;    
	uint8_t FontHeight;   
	const uint16_t *data; 
} FontDef_t;

extern FontDef_t Font_7x10;

#endif
