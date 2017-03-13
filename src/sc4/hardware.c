
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "stm32f4xx_hal.h"
#include "usb_device.h"
#include "pinmap.h"
#include "usbd_cdc_if.h"
#include "stm32f4xx_it.h"
#include "hardware.h"
#include "init.h"

/// @brief  get the elapsed milliseconds (modulo 32) since reset
/// @retval modulo 32 bit ms since reset
uint32_t millis(void) { return ms_count; }

/// @brief blocking delay in ms units
void delay(uint32_t ms) {
  volatile uint32_t mark = ms_count;
  while ((ms_count - mark) < ms);
}

/// @brief Reads the hardware random number generator
/// @retval 32-bit random number
uint32_t read_rng(void) {
  uint32_t r;
  int status = HAL_RNG_GenerateRandomNumber(&hrng, &r);
  if (status == HAL_OK) return (int)r;
  else return abs(status)*-1;
}

uint32_t erase_user_flash() {
  FLASH_EraseInitTypeDef pEraseInit;
  uint32_t sector_error;

  pEraseInit.TypeErase = TYPEERASE_SECTORS;
  pEraseInit.Sector = FLASH_USER_SECTOR;
  pEraseInit.NbSectors = 1;
  pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3; // 2.7 .. 3.6V

  HAL_FLASH_Unlock();
  int status = HAL_FLASHEx_Erase(&pEraseInit, &sector_error);
  HAL_FLASH_Lock();
  return status;
}

/// @brief write to user flash sector

/// Source data need not be aligned in any way. Target alignment in the flash
/// is important, but is
/// taken care of by this function. Up to 3 byte accesses might be done
/// initially to get the target address aligned to 32 bits.
void write_flash(uint8_t *data, ///< [in] buffer containing data to write
                 uint16_t addr, ///< [in] address relative to start of user
                                /// sector, NOT actual target flash address.
                                /// 0 is the first address here.
                 uint16_t sz    ///< [in] number of bytes to write
                 ) {
  uint32_t flash_addr = FLASH_USER_START_ADDR + addr;
  
  // see if all the target bytes are erased. If not, the whole sector gets
  // erased.
  for (int i=0; i<sz; i++) {
    if (*((u8*)flash_addr + i) != 0xFF) {
      erase_user_flash();
      break;
    }
  }
  
  HAL_FLASH_Unlock();
  
  // get any non-longword alignment out of the way
  while (sz && (flash_addr & 3)) {
    HAL_FLASH_Program(TYPEPROGRAM_BYTE, flash_addr++, *data++);
    sz--;
  }

  uint32_t tmp=0;
  int16_t j;

  while (sz) {
    for (j = 0; (j < 4) && sz; j++, sz--) {
      tmp >>= 8;
      tmp |= *data++ << 24;
    }
    while (j++ < 4) tmp = (tmp >> 8) | 0xff000000;
    HAL_FLASH_Program(TYPEPROGRAM_WORD, flash_addr, tmp);
    flash_addr += 4;
  }

  /* Lock the Flash to disable flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) ********
     */
  HAL_FLASH_Lock();
}

/// @brief turn a user LED on
void set_led(u8 n) {
  HAL_GPIO_WritePin(LEDRG_PORT, LEDR_PIN, (n>>2)&1);
  HAL_GPIO_WritePin(LEDRG_PORT, LEDG_PIN, (n>>1)&1);
  HAL_GPIO_WritePin(LEDB_PORT, LEDB_PIN, n&1);
}

uint8_t serial_available(void) { return rx_fifo.count; }

/**
  * @brief  Read one byte from the usb rx data queue.
  * @retval One byte from the rx queue, or 0x100 if no data available in queue.
  */
uint16_t serial_read(void) {
  uint16_t ret = 0;

  __set_PRIMASK(1);
  if (rx_fifo.count) {
    ret = rx_fifo.data[rx_fifo.tail++];
    rx_fifo.count--;
    if (rx_fifo.tail >= rx_fifo.sz) {
      rx_fifo.tail = 0;
    }
  } else {
    ret = 0x100;
  }
  __set_PRIMASK(0);
  return ret;
}

void serial_write(char *buf, int n) {
  CDC_Transmit_FS((uint8_t *)buf, n);
}

void serial_flush(void) {
  USBD_CDC_HandleTypeDef *hcdc = hUsbDevice_0->pClassData;
  // This hangs when USB CDC has been reset
  set_led(BLUE);
  while (hcdc->TxState);
  set_led(OFF);
}

// Pat Nystrom recommended this but empirically it doesn't seem to do
// anything useful
void usb_reset() {
  USB_DevDisconnect(USB_OTG_FS);
  delay(100); // wait .1 sec
  USB_DevConnect(USB_OTG_FS);
}

uint32_t user_buttons() {
  int sw1 = HAL_GPIO_ReadPin(SW_PORT, SW1_PIN);
  int sw2 = HAL_GPIO_ReadPin(SW_PORT, SW2_PIN);
  return ((sw1==0)<<1) | (sw2==0);
}
