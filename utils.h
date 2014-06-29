// rom.h
// rom tools - header inspect
#include <stdint.h>
#include <libdragon.h>

/*
 * 

0000h              (1 byte): initial PI_BSB_DOM1_LAT_REG value (0x80)
0001h              (1 byte): initial PI_BSB_DOM1_PGS_REG value (0x37)
0002h              (1 byte): initial PI_BSB_DOM1_PWD_REG value (0x12)
0003h              (1 byte): initial PI_BSB_DOM1_PGS_REG value (0x40)
0004h - 0007h     (1 dword): ClockRate
0008h - 000Bh     (1 dword): Program Counter (PC)
000Ch - 000Fh     (1 dword): Release
0010h - 0013h     (1 dword): CRC1
0014h - 0017h     (1 dword): CRC2
0018h - 001Fh    (2 dwords): Unknown (0x0000000000000000)
0020h - 0033h    (20 bytes): Image name
                             Padded with 0x00 or spaces (0x20)
0034h - 0037h     (1 dword): Unknown (0x00000000)
0038h - 003Bh     (1 dword): Manufacturer ID
                             0x0000004E = Nintendo ('N')
003Ch - 003Dh      (1 word): Cartridge ID
003Eh - 003Fh      (1 word): Country code
                             0x4400 = Germany ('D')
                             0x4500 = USA ('E')
                             0x4A00 = Japan ('J')
                             0x5000 = Europe ('P')
                             0x5500 = Australia ('U')
0040h - 0FFFh (1008 dwords): Boot code
*/

#define PI_BASE_REG		0x04600000
#define PIF_RAM_START		0x1FC007C0


/*
 * PI status register has 3 bits active when read from (PI_STATUS_REG - read)
 *	Bit 0: DMA busy - set when DMA is in progress
 *	Bit 1: IO busy  - set when IO is in progress
 *	Bit 2: Error    - set when CPU issues IO request while DMA is busy
 */

#define PI_STATUS_REG		(PI_BASE_REG+0x10)

/* PI DRAM address (R/W): starting RDRAM address */
#define PI_DRAM_ADDR_REG	(PI_BASE_REG+0x00)	/* DRAM address */

/* PI pbus (cartridge) address (R/W): starting AD16 address */
#define PI_CART_ADDR_REG	(PI_BASE_REG+0x04)

/* PI read length (R/W): read data length */
#define PI_RD_LEN_REG		(PI_BASE_REG+0x08)

/* PI write length (R/W): write data length */
#define PI_WR_LEN_REG		(PI_BASE_REG+0x0C)

/*
 * PI status (R): [0] DMA busy, [1] IO busy, [2], error
 *           (W): [0] reset controller (and abort current op), [1] clear intr
 */

#define PI_BSD_DOM1_LAT_REG	(PI_BASE_REG+0x14)

/* PI dom1 pulse width (R/W): [7:0] domain 1 device R/W strobe pulse width */
#define PI_BSD_DOM1_PWD_REG	(PI_BASE_REG+0x18)

/* PI dom1 page size (R/W): [3:0] domain 1 device page size */
#define PI_BSD_DOM1_PGS_REG	(PI_BASE_REG+0x1C)    /*   page size */

/* PI dom1 release (R/W): [1:0] domain 1 device R/W release duration */
#define PI_BSD_DOM1_RLS_REG	(PI_BASE_REG+0x20)
/* PI dom2 latency (R/W): [7:0] domain 2 device latency */
#define PI_BSD_DOM2_LAT_REG	(PI_BASE_REG+0x24)    /* Domain 2 latency */

/* PI dom2 pulse width (R/W): [7:0] domain 2 device R/W strobe pulse width */
#define PI_BSD_DOM2_PWD_REG	(PI_BASE_REG+0x28)    /*   pulse width */

/* PI dom2 page size (R/W): [3:0] domain 2 device page size */
#define PI_BSD_DOM2_PGS_REG	(PI_BASE_REG+0x2C)    /*   page size */

/* PI dom2 release (R/W): [1:0] domain 2 device R/W release duration */
#define PI_BSD_DOM2_RLS_REG	(PI_BASE_REG+0x30)    /*   release duration */


#define	PI_DOMAIN1_REG		PI_BSD_DOM1_LAT_REG
#define	PI_DOMAIN2_REG		PI_BSD_DOM2_LAT_REG


#define	PI_STATUS_ERROR		0x04
#define	PI_STATUS_IO_BUSY	0x02
#define	PI_STATUS_DMA_BUSY	0x01

#define	PHYS_TO_K0(x)	((u32)(x)|0x80000000)	/* physical to kseg0 */
#define	K0_TO_PHYS(x)	((u32)(x)&0x1FFFFFFF)	/* kseg0 to physical */
#define	PHYS_TO_K1(x)	((u32)(x)|0xA0000000)	/* physical to kseg1 */
#define	K1_TO_PHYS(x)	((u32)(x)&0x1FFFFFFF)	/* kseg1 to physical */

#define	IO_READ(addr)		(*(volatile u32*)PHYS_TO_K1(addr))
#define	IO_WRITE(addr,data)	(*(volatile u32*)PHYS_TO_K1(addr)=(u32)(data))

#define SAVE_SIZE_SRAM 		32768
#define SAVE_SIZE_SRAM128 	131072
#define SAVE_SIZE_EEP4k 	4096
#define SAVE_SIZE_EEP16k 	16384
#define SAVE_SIZE_FLASH 	131072

#define ROM_ADDR 		0xb0000000


#define FRAM_EXECUTE_CMD		0xD2000000
#define FRAM_STATUS_MODE_CMD	0xE1000000
#define FRAM_ERASE_OFFSET_CMD	0x4B000000
#define FRAM_WRITE_OFFSET_CMD	0xA5000000 
#define FRAM_ERASE_MODE_CMD		0x78000000
#define FRAM_WRITE_MODE_CMD		0xB4000000
#define FRAM_READ_MODE_CMD		0xF0000000

#define FRAM_STATUS_REG	0xA8000000
#define FRAM_COMMAND_REG 0xA8010000


#define CIC_6101 1
#define CIC_6102 2
#define CIC_6103 3
#define CIC_6104 4
#define CIC_6105 5
#define CIC_6106 6

sprite_t *loadImage32DFS(char *fname);
sprite_t *loadImageDFS(char *fname);
sprite_t *loadImage32(u8 *tbuf, int size);
void drawImage(display_context_t dcon, sprite_t *sprite);
void _sync_bus(void);
void _data_cache_invalidate_all(void);
void printText(char *msg, int x, int y, display_context_t dcon);

// End ...
int is_valid_rom(unsigned char *buffer);
void swap_header(unsigned char* header, int loadlength);

void restoreTiming(void);

void simulate_boot(u32 boot_cic, u8 bios_cic);
void globalTest(void);


u8 getCicType(u8 bios_cic);
int get_cic(unsigned char *buffer);
int get_cic_save(char *cartid, int *cic, int *save);
//const char* saveTypeToExtension(int type);
const char* saveTypeToExtension(int type, int etype);
int saveTypeToSize(int type);
int getSaveFromCart(int stype, uint8_t *buffer);
int pushSaveToCart(int stype, uint8_t *buffer);

int getSRAM(  uint8_t *buffer,int size);
int getSRAM32(  uint8_t *buffer);
int getSRAM128(  uint8_t *buffer);
int getEeprom4k(  uint8_t *buffer);
int getEeprom16k(  uint8_t *buffer);
int getFlashRAM(  uint8_t *buffer);
		
int setSRAM(uint8_t *buffer,int size);
int setSRAM32(  uint8_t *buffer);
int setSRAM128(  uint8_t *buffer);
int setEeprom4k(  uint8_t *buffer);
int setEeprom16k( uint8_t *buffer);
int setFlashRAM(  uint8_t *buffer);
