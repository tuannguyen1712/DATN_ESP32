#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "soc/soc_caps.h"

#define CS          14
#define MISO        27
#define CLK         26
#define MOSI        25  

#define WRITE_EN				0x06
#define WRITE_DIS				0x04
#define READ_DATA				0x03
#define PAGE_PROGRAM			0x02
#define ERASE_SECTOR			0x20
#define ERASE_BLOCK_32			0x52
#define ERASE_BLOCK_64			0xD8
#define ERASE_CHIP				0x60
#define RDSR1					0x05

#define SECTOR_SIZE				4096

typedef struct 
{
    uint8_t i;
    uint8_t l;
    uint8_t result;
} w25q32_t;


void init_spi_bus();
void W25Q32_CS_LOW();
void W25Q32_CS_HIGH();
void W25Q32_Send_Receive(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);
void WriteEnable();
void WriteDisable();
uint8_t* W25Q32_ReadData(uint8_t *pBuffer, uint32_t ReadAddr, uint16_t len);
void W25Q32_WriteData(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t len);
void W25Q32_erase4k(uint32_t add);
void W25Q32_WaitEndCycle();

w25q32_t W25Q32_check_wifi_info();              // get location of wifi info in w25q32: i-sector num,l-data len
void W25Q32_get_wifi_info(uint8_t i, uint8_t l);                            // get data (format "w:<ssid>\tp:<pass>")
uint8_t W25Q32_write_wifi_info(uint8_t *ssid, uint8_t *pass, int i);    // write data to sector i
void W25Q32_update_wifi_info(int i, int l);                             // update new location of data in w25q32