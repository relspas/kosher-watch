#pragma once

#include "movement.h"

watch_lcd_type_t watch_get_lcd_type(void);
void watch_clear_display(void);
void watch_set_pixel(uint8_t com, uint8_t seg);
void watch_clear_pixel(uint8_t com, uint8_t seg);
void watch_display_character(uint8_t character, uint8_t position);
void watch_display_text_with_fallback(watch_position_t location, const char *string, const char *fallback);
void watch_set_decimal_if_available(void);
