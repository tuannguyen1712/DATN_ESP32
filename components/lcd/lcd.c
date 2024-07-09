#include "lcd.h"

uint8_t RS, RW, EN;             // control pin
uint8_t D0, D1, D2, D3, D4, D5, D6, D7;     //data pin (8 pin mode)

void Init_pin_8bit_mode(uint8_t rs, uint8_t rw, uint8_t en, uint8_t d0, uint8_t d1, uint8_t d2, 
              uint8_t d3, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7) 
{
    RS = rs;
    RW = rw;
    EN = en;
    D0 = d0;
    D1 = d1;
    D2 = d2;
    D3 = d3;
    D4 = d4;
    D5 = d5;
    D6 = d6;
    D7 = d7;
    gpio_config_t io_config;
    io_config.pin_bit_mask = (1 << RS) | (1 << RW) | (1 << EN) | (1 << D0) | (1 << D1) | (1 << D2)
                            | (1 << D3) | (1 << D4) | (1 << D5) | (1 << D6) | (1 << D7) | (1 << 2);
    io_config.mode = GPIO_MODE_OUTPUT;
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_config.pull_up_en = GPIO_PULLUP_DISABLE;
    io_config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_config);
}

uint8_t get_bit(uint8_t bit, uint8_t val) 
{
    uint8_t result = 0;
    uint8_t mask = 1 << bit;
    result = (mask & val) >> bit;
    return result;
}
void set_pin_8bit_mode(uint8_t val) {
    gpio_set_level(D0, get_bit(0, val));
    gpio_set_level(D1, get_bit(1, val));
    gpio_set_level(D2, get_bit(2, val));
    gpio_set_level(D3, get_bit(3, val));
    gpio_set_level(D4, get_bit(4, val));
    gpio_set_level(D5, get_bit(5, val));
    gpio_set_level(D6, get_bit(6, val));
    gpio_set_level(D7, get_bit(7, val));
}

void LCD_Init_8bit_mode() 
{
    vTaskDelay(20 / portTICK_PERIOD_MS);
    LCD_Command_8bit_mode (0x38);	                /* Initialization of 16X2 LCD in 8bit mode */
	LCD_Command_8bit_mode (0x0C);	                /* Display ON Cursor OFF */
	LCD_Command_8bit_mode (0x06);	                /* Auto Increment cursor */
	LCD_Command_8bit_mode (0x01);	                /* clear display */
	LCD_Command_8bit_mode (0x80);	                /* cursor at home position */    
}
void LCD_Command_8bit_mode(uint8_t cmd)
{
    set_pin_8bit_mode(cmd);
    gpio_set_level(RS, 0);
    gpio_set_level(RW, 0);
    gpio_set_level(EN, 1);
    ets_delay_us(1);
    gpio_set_level(EN, 0);
    vTaskDelay(3 / portTICK_PERIOD_MS);
}
void LCD_Char_8bit_mode(uint8_t chr)
{
    set_pin_8bit_mode(chr);
    gpio_set_level(RS, 1);
    gpio_set_level(RW, 0);
    gpio_set_level(EN, 1);
    ets_delay_us(1);
    gpio_set_level(EN, 0);
    vTaskDelay(1 / portTICK_PERIOD_MS);
}
void LCD_String_8bit_mode(uint8_t *str)
{
    int i;
	for(i=0;str[i]!=0;i++)  /* send each char of string till the NULL */
	{
		LCD_Char_8bit_mode(str[i]);  /* call LCD data write */
	}
}
void LCD_String_xy_8bit_mode(uint8_t row, uint8_t pos, uint8_t *str) 
{
    if (row == 0 && pos < 16)
	LCD_Command_8bit_mode((pos & 0x0F) | 0x80);	/* Command of first row and required position<16 */
	else if (row == 1 && pos < 16)
	LCD_Command_8bit_mode((pos & 0x0F) | 0xC0);	/* Command of first row and required position<16 */
	LCD_String_8bit_mode(str);		            /* Call LCD string function */
}

void LCD_Clear_8bit_mode()
{
    LCD_Command_8bit_mode(0x01);		/* clear display */
	LCD_Command_8bit_mode(0x80);		/* cursor at home position */
}

void Init_pin_4bit_mode(uint8_t rs, uint8_t rw, uint8_t en, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7) {
    ESP_LOGI("LCD", "Start init pin");
    RS = rs;
    RW = rw;
    EN = en;
    D4 = d4;
    D5 = d5;
    D6 = d6;
    D7 = d7;

    esp_rom_gpio_pad_select_gpio(RS);
    esp_rom_gpio_pad_select_gpio(RW);
    esp_rom_gpio_pad_select_gpio(EN);
    esp_rom_gpio_pad_select_gpio(D4);
    esp_rom_gpio_pad_select_gpio(D5);
    esp_rom_gpio_pad_select_gpio(D6);
    esp_rom_gpio_pad_select_gpio(D7);
    // esp_rom_gpio_pad_select_gpio(2);

    gpio_set_direction(RS, GPIO_MODE_OUTPUT);
    gpio_set_direction(RW, GPIO_MODE_OUTPUT);
    gpio_set_direction(EN, GPIO_MODE_OUTPUT);
    gpio_set_direction(D4, GPIO_MODE_OUTPUT);
    gpio_set_direction(D5, GPIO_MODE_OUTPUT);
    gpio_set_direction(D6, GPIO_MODE_OUTPUT);
    gpio_set_direction(D7, GPIO_MODE_OUTPUT);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    // gpio_config_t io_conf;
    // io_conf.intr_type = GPIO_INTR_DISABLE;
    // io_conf.mode = GPIO_MODE_OUTPUT;
    // io_conf.pin_bit_mask = (1ULL << RS) | (1ULL << RW) | (1ULL << EN) |
    //                        (1ULL << D4) | (1ULL << D5) | (1ULL << D6) | (1ULL << D7);
    // io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    // gpio_config(&io_conf);
    
}

void set_pin_4bit_mode(uint8_t val) {
    gpio_set_level(D4, get_bit(4, val));
    gpio_set_level(D5, get_bit(5, val));
    gpio_set_level(D6, get_bit(6, val));
    gpio_set_level(D7, get_bit(7, val));
    // ESP_LOGI("LCD", "%X", val);
    vTaskDelay(2 / portTICK_PERIOD_MS);
}

void LCD_Init_4bit_mode() {
    ESP_LOGI("LCD", "Start init LCD");
    gpio_set_level(RW, 0);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    LCD_Command_4bit_mode(0x02);		/* send for 4 bit initialization of LCD  */
	LCD_Command_4bit_mode(0x28);              /* 2 line, 5*7 matrix in 4-bit mode */
	LCD_Command_4bit_mode(0x0c);              /* Display on cursor off*/
	LCD_Command_4bit_mode(0x06);              /* Increment cursor (shift cursor to right)*/
	LCD_Command_4bit_mode(0x01);              /* Clear display screen*/
    vTaskDelay(2 / portTICK_PERIOD_MS);
}

void LCD_Command_4bit_mode(uint8_t cmd) {
    set_pin_4bit_mode(cmd & 0xF0);
    gpio_set_level(RS, 0);
    gpio_set_level(EN, 1);
    ets_delay_us(1);
    gpio_set_level(EN, 0);

    ets_delay_us(200);

    set_pin_4bit_mode(cmd << 4);
    gpio_set_level(EN, 1);
    ets_delay_us(1);
    gpio_set_level(EN, 0);

    vTaskDelay(2 / portTICK_PERIOD_MS);
}

void LCD_Char_4bit_mode(uint8_t chr) {
    set_pin_4bit_mode(chr & 0xF0);
    gpio_set_level(RS, 1);
    gpio_set_level(EN, 1);
    ets_delay_us(1);
    gpio_set_level(EN, 0);

    ets_delay_us(200);

    set_pin_4bit_mode(chr << 4);
    gpio_set_level(EN, 1);
    ets_delay_us(1);
    gpio_set_level(EN, 0);

    vTaskDelay(2 / portTICK_PERIOD_MS);
}

void LCD_String_4bit_mode(uint8_t *str) {
    int i;
	for(i = 0; str[i] !=0; i++)		/* Send each char of string till the NULL */
	{
		LCD_Char_4bit_mode(str[i]);
	}
}

void LCD_String_xy_4bit_mode(uint8_t row, uint8_t pos, uint8_t *str) {
    if (row == 0 && pos < 16)
	LCD_Command_4bit_mode((pos & 0x0F) | 0x80);	/* Command of first row and required position<16 */
	else if (row == 1 && pos < 16)
	LCD_Command_4bit_mode((pos & 0x0F) | 0xC0);	/* Command of first row and required position<16 */
	LCD_String_4bit_mode(str);	
}

void LCD_Clear_4bit_mode()
{
    LCD_Command_8bit_mode(0x01);		/* clear display */
    vTaskDelay(2 / portTICK_PERIOD_MS);
	LCD_Command_8bit_mode(0x80);		/* cursor at home position */
}