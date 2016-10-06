
#ifndef SC4_HSM_HW
#define SC4_HSM_HW

#include <inttypes.h>
typedef unsigned char u8;

#ifdef __cplusplus
extern "C" {
#endif

//#define FLASH_PAGE_SIZE         ((uint32_t)0x00000800)   /* FLASH Page Size */
// We use Sector 2 for user data. This is a 16k sector.
#define FLASH_USER_START_ADDR   ((uint32_t)0x08008000)   /* Start @ of user Flash area */
#define FLASH_USER_SIZE 16384          // bytes of flash in the user sector
#define FLASH_USER_SECTOR FLASH_SECTOR_2
#ifdef STM32F303xE
#define FLASH_USER_END_ADDR     ((uint32_t)0x08080000)   /* End @ of user Flash area */
#else
#define FLASH_USER_END_ADDR     ((uint32_t)(FLASH_USER_START_ADDR + FLASH_PAGE_SIZE))   /* End @ of user Flash area */
#endif
#define DATA_32                 ((uint32_t)0x12345678)

/* #define DVCID_ADDR			0x1FFFF7AC */
#define DVCID_ADDR			0x1FFF7A10

#define STM32_UUID ((uint32_t *)DVCID_ADDR)

uint32_t millis(void);      ///< read ms since reset
void delay(uint32_t ms);    ///< blocking delay in ms
uint32_t read_rng(void);         ///< read random number generator

uint32_t erase_user_flash();
void write_flash(uint8_t *data, uint16_t addr, uint16_t sz);
uint8_t serial_available(void);
uint16_t serial_read(void);
void serial_write(char *, int);
void serial_flush(void);
uint32_t user_buttons();

void set_led(uint8_t);
#define RED 4
#define GREEN 2
#define BLUE 1
#define YELLOW 6
#define CYAN 3
#define PURPLE 5
#define OFF 0

#ifdef __cplusplus
}
#endif


#endif /* SC4_HSM_HW */
