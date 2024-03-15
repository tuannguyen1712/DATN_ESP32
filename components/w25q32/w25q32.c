#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "w25q32.h"
#include "soc/soc_caps.h"
	
spi_device_handle_t spi;
esp_err_t ret1;
uint8_t dum_byte = 0xff;
uint8_t w25q32_data[100] = "";

void init_spi_bus() {
    ESP_LOGI("W25Q32", "Init spi bus");
	gpio_reset_pin(CS);
	gpio_set_direction( CS, GPIO_MODE_DEF_OUTPUT );
	gpio_set_level(CS, 0);

    spi_device_interface_config_t dvccfg = {
		// .clock_source = SPI_CLK_SRC_DEFAULT,
    	.clock_speed_hz = 2000000,
		.spics_io_num = -1,
    	.queue_size = 1,
		.command_bits = 0,
		.address_bits = 0,
    	.mode = 0
	};
    
    spi_bus_config_t buscfg = { 
        .miso_io_num = MISO,
        .mosi_io_num = MOSI,
        .sclk_io_num = CLK,
        .max_transfer_sz = 32,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    ret1 = spi_bus_initialize(VSPI_HOST, &buscfg, 1);
	if (ret1) {
		ESP_LOGI("W25Q32", "Init spi fail 1");
	}
    ret1 = spi_bus_add_device(VSPI_HOST, &dvccfg, &spi);
	if (ret1 == ESP_ERR_NOT_FOUND ) {
		ESP_LOGI("W25Q32", "Init spi fail");
	}
}

void W25Q32_Send_Receive(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len) {
    spi_transaction_t trans = {
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
        .length = len * 8,
		.rxlength = len * 8
    };

	ret1 = spi_device_polling_transmit(spi, &trans);
	if (ret1) 
		ESP_LOGI("W25Q32", "Spi transmit fail");
		
}

void WriteEnable() {
	uint8_t *tsm = malloc(sizeof(uint8_t));
	uint8_t *rev = malloc(sizeof(uint8_t));
	tsm[0] = WRITE_EN;

	W25Q32_CS_LOW();
	W25Q32_Send_Receive(tsm, rev, 1);
	W25Q32_CS_HIGH();

	free(tsm);
	free(rev);
}

void WriteDisable() {
	uint8_t *tsm = (uint8_t*) malloc(sizeof(uint8_t));
	uint8_t *rev = (uint8_t*) malloc(sizeof(uint8_t));
	tsm[0] = WRITE_DIS;

	W25Q32_CS_LOW();
	W25Q32_Send_Receive(tsm, rev, 1);
	W25Q32_CS_HIGH();

	free(tsm);
	free(rev);
}

uint8_t* W25Q32_ReadData(uint8_t *pBuffer, uint32_t ReadAddr, uint16_t len) {
    uint8_t *cmd = malloc(sizeof(uint8_t) * (4 + len));
	uint8_t *r = malloc(sizeof(uint8_t) * len);
	cmd[0] = READ_DATA;
	cmd[1] = (ReadAddr & 0x00FF0000) >> 16;
	cmd[2] = (ReadAddr & 0x0000FF00) >> 8;
	cmd[3] = ReadAddr & 0x000000FF;

	W25Q32_CS_LOW();
    W25Q32_Send_Receive(cmd, pBuffer , len + 4);
	W25Q32_CS_HIGH();
    pBuffer = pBuffer + 4;
	pBuffer[len] = 0;

    free(cmd);
	free(r);
    ESP_LOGI("W25Q32", "Read data from w25q32 at %lu: %s", ReadAddr, pBuffer);
	return pBuffer;
}

void W25Q32_WriteData(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t len) {
    uint8_t *cmd = malloc(sizeof(uint8_t) * (4 + len));
	uint8_t *data = malloc(sizeof(uint8_t) * (4 + len));

	WriteEnable();

	cmd[0] = PAGE_PROGRAM;
	cmd[1] = (WriteAddr & 0x00FF0000) >> 16;
	cmd[2] = (WriteAddr & 0x0000FF00) >> 8;
	cmd[3] = WriteAddr & 0x000000FF;

	memcpy(cmd + 4, pBuffer, strlen((char*) pBuffer));
	W25Q32_CS_LOW();
	W25Q32_Send_Receive(cmd, data, len + 4);
	W25Q32_CS_HIGH();
	W25Q32_WaitEndCycle();
	WriteDisable();
	free(cmd);
	free(data);
    ESP_LOGI("W25Q32", "Write data to w25q32 at %lu: %s", WriteAddr, pBuffer);
}

void W25Q32_erase4k(uint32_t add) {
    uint8_t *cmd = (uint8_t*) malloc(sizeof(uint8_t) * 4);
	uint8_t *r = (uint8_t*) malloc(sizeof(uint8_t) * 4);
	cmd[0] = ERASE_SECTOR;
	cmd[1] = (add & 0x00FF0000) >> 16;
	cmd[2] = (add & 0x0000FF00) >> 8;
	cmd[3] = add & 0x0000000FF;

	WriteEnable();
	W25Q32_CS_LOW();
	W25Q32_Send_Receive(cmd, r, 4);
	W25Q32_CS_HIGH();
	W25Q32_WaitEndCycle();
	WriteDisable();

	free(cmd);
	free(r);
    ESP_LOGI("W25Q32", "Erase 4k of w25q32 at %lu", add);
}

void W25Q32_CS_LOW() 
{
	gpio_set_level(CS, 0);
}
void W25Q32_CS_HIGH() {
	gpio_set_level(CS, 1);
}

void W25Q32_WaitEndCycle() {
	W25Q32_CS_LOW();
	uint8_t check;
	uint8_t *tx = (uint8_t*) malloc(sizeof(uint8_t));
	uint8_t *rx = (uint8_t*) malloc(sizeof(uint8_t));
	*tx = RDSR1;

	W25Q32_CS_LOW();
	W25Q32_Send_Receive(tx, rx, 1);
	do {
		*tx = dum_byte;
		W25Q32_Send_Receive(tx, rx, 1);
		check = *rx;
	} while (check & 0x01);
	W25Q32_CS_HIGH();
	free(tx);
	free(rx);
}

w25q32_t W25Q32_check_wifi_info() {
	w25q32_t w25q32;
	uint8_t chk[17];
	strcpy((char*) chk, (char*) W25Q32_ReadData(chk, 0, 17));
	ESP_LOGI("W25Q32", "wifi data location: %s", chk);

	if (strncmp((char*) chk, "save", 4) == 0 && strlen((char*) chk) == 17) {
		sscanf((char*) chk, "save\ti:%04hhu\tl:%03hhu", &w25q32.i, &w25q32.l);
		w25q32.result = 1;
		ESP_LOGI("W25Q32", "wifi data at sector %d, len %d", w25q32.i, w25q32.l);
		return w25q32;
	}
	else {
		ESP_LOGI("W25Q32", "There has never been wifi data written to the w25q32 before");
		w25q32.result = 0;
		return w25q32;
	} 
}

void W25Q32_get_wifi_info(uint8_t i, uint8_t l) {
	strcpy((char*) w25q32_data, (char*) W25Q32_ReadData(w25q32_data, 1 * SECTOR_SIZE, l));
	ESP_LOGI("W25Q32", "get wifi info: %s", (char*) w25q32_data);
}

uint8_t W25Q32_write_wifi_info(uint8_t *ssid, uint8_t *pass, int i) {
	uint8_t l = strlen((char*) ssid) + strlen((char*) pass) + 5;
	uint8_t *wf_in = (uint8_t*) malloc(100);
	*(wf_in + l) = 0;
	sprintf((char*) wf_in, "w:%s\tp:%s", (char*) ssid, (char*) pass);
	ESP_LOGI("W25Q32", "write wifi info: %s", (char*) wf_in);
	W25Q32_erase4k(i * SECTOR_SIZE);
	W25Q32_WriteData(wf_in, i * SECTOR_SIZE, l);
	free(wf_in);
	return l;
}

void W25Q32_update_wifi_info(int i, int l) {
	uint8_t *lct = (uint8_t*) malloc(20);
	sprintf((char*) lct, "save\ti:%d\tl:%d", i, l);
	ESP_LOGI("W25Q32", "update info location: i:%d l:%d", i, l);
	W25Q32_erase4k(i * SECTOR_SIZE);
	W25Q32_WriteData(lct, 0, strlen((char*) lct));
	free(lct);
}