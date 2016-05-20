/*
 * Memory Spaces Definitions.
 *
 * Need modifying for a specific board. 
 *   FLASH.ORIGIN: starting address of flash
 *   FLASH.LENGTH: length of flash
 *   RAM.ORIGIN: starting address of RAM bank 0
 *   RAM.LENGTH: length of RAM bank 0
 *
 * The values below can be addressed in further linker scripts
 * using functions like 'ORIGIN(RAM)' or 'LENGTH(RAM)'.
 */

MEMORY
{
    RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
    CCMRAM (xrw) : ORIGIN = 0x10000000, LENGTH = 64K
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
    FLASH_ISR (rx)  : ORIGIN = 0x08000000, LENGTH = 0x004000 /* sector 0, 16 KiB */
    FLASH_TEXT (rx) : ORIGIN = 0x08020000, LENGTH = 0x080000 /* sectors 5,6,7,8, 4*128KiB = 512 KiB (could increase it more) */
  
    FLASHB1 (rx) : ORIGIN = 0x00000000, LENGTH = 0
    EXTMEMB0 (rx) : ORIGIN = 0x00000000, LENGTH = 0
    EXTMEMB1 (rx) : ORIGIN = 0x00000000, LENGTH = 0
    EXTMEMB2 (rx) : ORIGIN = 0x00000000, LENGTH = 0
    EXTMEMB3 (rx) : ORIGIN = 0x00000000, LENGTH = 0
    MEMORY_ARRAY (xrw)  : ORIGIN = 0x20002000, LENGTH = 32
}

/*
 * For external ram use something like:

   RAM (xrw) : ORIGIN = 0x64000000, LENGTH = 2048K

 */
