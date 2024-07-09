#include <stdint.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <rom/ets_sys.h>

uint8_t get_bit(uint8_t bit, uint8_t val);

void set_pin_8bit_mode(uint8_t val);
void Init_pin_8bit_mode(uint8_t rs, uint8_t rw, uint8_t en, uint8_t d0, uint8_t d1, uint8_t d2, 
              uint8_t d3, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7) ;
void LCD_Init_8bit_mode();
void LCD_Command_8bit_mode(uint8_t cmd);
void LCD_Char_8bit_mode(uint8_t chr);
void LCD_String_8bit_mode(uint8_t *str);
void LCD_String_xy_8bit_mode(uint8_t row, uint8_t pos, uint8_t *str);
void LCD_Clear_8bit_mode();

void set_pin_4bit_mode(uint8_t val);
void Init_pin_4bit_mode(uint8_t rs, uint8_t rw, uint8_t en, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);
void LCD_Init_4bit_mode();
void LCD_Command_4bit_mode(uint8_t cmd);
void LCD_Char_4bit_mode(uint8_t chr);
void LCD_String_4bit_mode(uint8_t *str);
void LCD_String_xy_4bit_mode(uint8_t row, uint8_t pos, uint8_t *str);
void LCD_Clear_4bit_mode();