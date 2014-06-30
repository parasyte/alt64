#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#include <libdragon.h>
#include <n64sys.h>
#include "everdrive.h"
#include "sys.h"

#include "types.h"
#include "utils.h"
#include "sram.h"

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

extern short int gCheats;               /* 0 = off, 1 = select, 2 = all */
extern short int force_tv;
extern short int boot_country;

struct gscEntry {
    char *description;
    char *gscodes;
    u16  count;
    u16  state;
    u16  mask;
    u16  value;
};
typedef struct gscEntry gscEntry_t;

extern gscEntry_t gGSCodes[];

static u8 __attribute__((aligned(16))) dmaBuf[128*1024];
static volatile struct _PI_regs_s * const _PI_regs = (struct _PI_regs_s *)0xa4600000;

int is_valid_rom(unsigned char *buffer) {
    /* Test if rom is a native .z64 image with header 0x80371240. [ABCD] */
    if((buffer[0]==0x80)&&(buffer[1]==0x37)&&(buffer[2]==0x12)&&(buffer[3]==0x40))
        return 0;
    /* Test if rom is a byteswapped .v64 image with header 0x37804012. [BADC] */
    else if((buffer[0]==0x37)&&(buffer[1]==0x80)&&(buffer[2]==0x40)&&(buffer[3]==0x12))
        return 1;
    /* Test if rom is a wordswapped .n64 image with header  0x40123780. [DCBA] */
    else if((buffer[0]==0x40)&&(buffer[1]==0x12)&&(buffer[2]==0x37)&&(buffer[3]==0x80))
        return 2;
    else
        return 0;
}

void swap_header(unsigned char* header, int loadlength) {
    unsigned char temp;
    int i;

    /* Btyeswap if .v64 image. */
    if( header[0]==0x37) {
        for (i = 0; i < loadlength; i+=2) {
            temp= header[i];
            header[i]= header[i+1];
            header[i+1]=temp;
            }
        }
    /* Wordswap if .n64 image. */
    else if( header[0]==0x40) {
        for (i = 0; i < loadlength; i+=4) {
            temp= header[i];
            header[i]= header[i+3];
            header[i+3]=temp;
            temp= header[i+1];
            header[i+1]= header[i+2];
            header[i+2]=temp;
        }
    }
}

u8 getCicType(u8 bios_cic) {
    u8 cic_buff[2048];
    volatile u8 cic_chip;
    volatile u32 val;
    if (bios_cic) {
        evd_setCfgBit(ED_CFG_SDRAM_ON, 0);
        sleep(10);
        val = *(u32 *) 0xB0000170;
        dma_read_s(cic_buff, 0xB0000040, 1024);
        cic_chip = get_cic(cic_buff);
        evd_setCfgBit(ED_CFG_SDRAM_ON, 1);
        sleep(10);
    }
    else {
        val = *(u32 *) 0xB0000170;
        dma_read_s(cic_buff, 0xB0000040, 1024);
        cic_chip = get_cic(cic_buff);
    }

    return cic_chip;
}



int get_cic(unsigned char *buffer) {
    unsigned int crc;
    // figure out the CIC
    crc = CRC_Calculate(0, buffer, 1000);
    switch(crc) {
    case 0x303faac9:
    case 0xf0da3d50:
        return 1;
    case 0xf3106101:
        return 2;
    case 0xe7cd9d51:
        return 3;
    case 0x7ae65c9:
        return 5;
    case 0x86015f8f:
        return 6;
    }

    return 2;
}

int get_cic_save(char *cartid, int *cic, int *save) {
    // variables
    int NUM_CARTS = 137;
    int i;

    //data arrays
    /*
    char *names[] = {
        "gnuboy64lite", "FRAMTestRom", "SRAMTestRom", "Worms Armageddon",
        "Super Smash Bros.", "Banjo-Tooie", "Blast Corps", "Bomberman Hero",
        "Body Harvest", "Banjo-Kazooie", "Bomberman 64",
        "Bomberman 64: Second Attack", "Command & Conquer", "Chopper Attack",
        "NBA Courtside 2 featuring Kobe Bryant", "Penny Racers",
        "Chameleon Twist", "Cruis'n USA", "Cruis'n World",
        "Legend of Zelda: Majora's Mask, The", "Donkey Kong 64",
        "Donkey Kong 64", "Donald Duck: Goin' Quackers",
        "Loony Toons: Duck Dodgers", "Diddy Kong Racing", "PGA European Tour",
        "Star Wars Episode 1 Racer", "AeroFighters Assault", "Bass Hunter 64",
        "Conker's Bad Fur Day", "F-1 World Grand Prix", "Star Fox 64",
        "F-Zero X", "GT64 Championship Edition", "GoldenEye 007", "Glover",
        "Bomberman 64", "Indy Racing 2000",
        "Indiana Jones and the Infernal Machine", "Jet Force Gemini",
        "Jet Force Gemini", "Earthworm Jim 3D", "Snowboard Kids 2",
        "Kirby 64: The Crystal Shards", "Fighters Destiny",
        "Major League Baseball featuring Ken Griffey Jr.",
        "Killer Instinct Gold", "Ken Griffey Jr's Slugfest", "Mario Kart 64",
        "Mario Party", "Lode Runner 3D", "Megaman 64", "Mario Tennis",
        "Mario Golf", "Mission: Impossible", "Mickey's Speedway USA",
        "Monopoly", "Paper Mario", "Multi-Racing Championship",
        "Big Mountain 2000", "Mario Party 3", "Mario Party 2", "Excitebike 64",
        "Dr. Mario 64", "Star Wars Episode 1: Battle for Naboo",
        "Kobe Bryant in NBA Courtside", "Excitebike 64",
        "Ogre Battle 64: Person of Lordly Caliber", "Pokémon Stadium 2",
        "Pokémon Stadium 2", "Perfect Dark", "Pokémon Snap",
        "Hey you, Pikachu!", "Pokémon Snap", "Pokémon Puzzle League",
        "Pokémon Stadium", "Pokémon Stadium", "Pilotwings 64",
        "Top Gear Overdrive", "Resident Evil 2", "New Tetris, The",
        "Star Wars: Rogue Squadron", "Ridge Racer 64",
        "Star Soldier: Vanishing Earth", "AeroFighters Assault",
        "Starshot Space Circus", "Super Mario 64", "Starcraft 64",
        "Rocket: Robot on Wheels", "Space Station Silicon Valley",
        "Star Wars: Shadows of the Empire", "Tigger's Honey Hunt",
        "1080º Snowboarding", "Tom & Jerry in Fists of Furry",
        "Mischief Makers", "All-Star Tennis '99", "Tetrisphere",
        "V-Rally Edition '99", "V-Rally Edition '99", "WCW/NWO Revenge",
        "WWF: No Mercy", "Waialae Country Club: True Golf Classics",
        "Wave Race 64", "Worms Armageddon", "WWF: Wrestlemania 2000",
        "Cruis'n Exotica", "Yoshi's Story", "Harvest Moon 64",
        "Legend of Zelda: Ocarina of Time, The",
        "Legend of Zelda: Majora's Mask, The", "Airboarder 64",
        "Bakuretsu Muteki Bangaioh", "Choro-Q 64 II", "Custom Robo",
        "Custom Robo V2", "Densha de Go! 64", "Doraemon: Mittsu no Seireiseki",
        "Dezaemon 3D", "Transformers Beast Wars",
        "Transformers Beast Wars Metals", "64 Trump Collection", "Bass Rush",
        "ECW Hardcore Revolution", "40 Winks", "Aero Gauge",
        "Aidyn Chronicles The First Mage", "Derby Stallion 64",
        "Doraemon 2 - Hikari no Shinden", "Doraemon 3 - Nobi Dai No Machi SOS",
        "F-1 World Grand Prix II", "Fushigi no Dungeon - Furai no Shiren 2",
        "Heiwa Pachinko World 64", "Neon Genesis Evangelion",
        "Racing Simulation", "Tsumi to Batsu", "Sonic Wings Assault",
        "Virtual Pro Wrestling", "Virtual Pro Wrestling 2", "Wild Choppers"
    };
    */
    char *cartIDs[] = {
        "DZ", "B6", "ZY", "ZZ", "AD", "AL", "B7", "BC", "BD", "BH", "BK", "BM",
        "BV", "CC", "CH", "CK", "CR", "CT", "CU", "CW", "DL", "DO", "DP", "DQ",
        "DU", "DY", "EA", "EP", "ER", "FH", "FU", "FW", "FX", "FZ", "GC", "GE",
        "GV", "HA", "IC", "IJ", "JD", "JF", "JM", "K2", "K4", "KA", "KG", "KI",
        "KJ", "KT", "LB", "LR", "M6", "M8", "MF", "MI", "ML", "MO", "MQ", "MR",
        "MU", "MV", "MW", "MX", "N6", "NA", "NB", "NX", "OB", "P2", "P3", "PD",
        "PF", "PG", "PH", "PN", "PO", "PS", "PW", "RC", "RE", "RI", "RS", "RZ",
        "S6", "SA", "SC", "SM", "SQ", "SU", "SV", "SW", "T9", "TE", "TJ", "TM",
        "TN", "TP", "VL", "VY", "W2", "W4", "WL", "WR", "WU", "WX", "XO", "YS",
        "YW", "ZL", "ZS", "AB", "BN", "CG", "CX", "CZ", "D6", "DR", "DZ", "OH",
        "TB", "TC", "VB", "WI", "4W", "AG", "AY", "DA", "D2", "3D", "F2", "SI",
        "HP", "EV", "MG", "GU", "SA", "VP", "A2", "WC"
    };

    /*
    int saveTypes[] = {
        5, 1, 6, 5, 5, 5, 5, 5, 5, 4, 5, 4, 5, 5, 5, 6, 4, 6, 6, 5, 5, 5, 5, 6,
        5, 5, 6, 5, 5, 1, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 1, 5, 4, 5, 5, 5,
        4, 6, 1, 5, 5, 5, 4, 5, 5, 6, 5, 6, 5, 5, 6, 6, 1, 4, 4, 6, 4, 5, 4, 4,
        4, 4, 5, 5, 1, 1, 5, 6, 5, 5, 5, 5, 4, 5, 5, 5, 4, 1, 5, 5, 5, 5, 5, 5,
        1, 4, 5, 5, 5, 1, 5, 6, 1, 1, 4, 5, 5, 5, 5, 6, 1, 5, 1, 5, 5, 5, 1, 1,
        5, 5, 1, 1, 6, 6, 6, 4, 5, 6, 5, 5, 5, 1, 1, 5
    };
    */
    // Banjo-Tooie B7 -> set to sram 'cause crk converts ek16->sram
    int saveTypes[] = {
        2, 1, 5, 1, 3, 1, 1, 3, 3, 3, 3, 3, 3, 5, 3, 5, 3, 3, 3, 4, 5, 4, 4, 3,
        3, 3, 3, 4, 3, 3, 4, 3, 3, 1, 3, 3, 3, 3, 3, 3, 5, 5, 3, 3, 3, 3, 1, 3,
        5, 3, 3, 3, 5, 4, 1, 3, 3, 3, 5, 3, 3, 4, 3, 4, 3, 3, 4, 4, 1, 5, 5, 4,
        5, 3, 5, 5, 5, 5, 3, 3, 1, 1, 3, 4, 3, 3, 3, 3, 5, 3, 3, 3, 5, 1, 3, 3,
        3, 3, 3, 3, 1, 5, 3, 3, 3, 1, 3, 4, 1, 1, 5, 3, 3, 3, 3, 4, 1, 3, 1, 3,
        3, 3, 1, 1, 3, 3, 1, 1, 4, 4, 4, 5, 3, 4, 3, 3, 3, 1, 1, 3
    };

    //bt cic to 2 pos6 was 5
    int cicTypes[] = {
        2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 6, 5, 5, 5, 2,
        2, 3, 2, 2, 2, 2, 5, 2, 1, 6, 2, 2, 2, 2, 2, 2, 5, 5, 2, 2, 3, 2, 3, 2,
        3, 2, 2, 2, 2, 2, 2, 2, 5, 2, 3, 2, 2, 2, 2, 3, 2, 2, 3, 3, 2, 3, 3, 5,
        3, 2, 3, 2, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6, 2, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
    };

    // search for cartid
    for (i=0; i<NUM_CARTS; i++)
        if (strcmp(cartid, cartIDs[i]) == 0)
            break;

    if (i == NUM_CARTS) {
        // cart not in list
        *cic = 2;
        *save = 0;
        return 0; // not found
    }

    // cart found
    *cic = cicTypes[i];
    *save = saveTypes[i];

    return 1; // found
}


const char* saveTypeToExtension(int type, int etype) {
    static char* str = "SAV";

    if(etype==0){
        switch(type) {
            case 0:  str = "OFF"; break;
            case 1:  str = "SRM"; break;
            case 2:  str = "128"; break;
            case 3:  str = "E4K";  break;
            case 4:  str = "E16"; break;
            case 5:  str = "FLA"; break;
            default: str = "SAV";
        }
    }
    else {
        switch(type) {
            case 0:  str = "OFF"; break;
            case 1:  str = "SRA"; break;
            case 2:  str = "SRA"; break;
            case 3:  str = "EEP";  break;
            case 4:  str = "EEP"; break;
            case 5:  str = "FLA"; break;
            default: str = "SAV";
        }
    }

    return str;
}

int saveTypeToSize(int type) {
    switch(type) {
        case 0: return 0; 					break;
        case 1: return SAVE_SIZE_SRAM; 		break;
        case 2: return SAVE_SIZE_SRAM128;	break;
        case 3: return SAVE_SIZE_EEP4k;		break;
        case 4: return SAVE_SIZE_EEP16k; 	break;
        case 5: return SAVE_SIZE_FLASH; 	break;
        default: return 0;
    }
}

/*
#define SAVE_TYPE_OFF 0
#define SAVE_TYPE_SRAM 1
#define SAVE_TYPE_SRAM128 2
#define SAVE_TYPE_EEP4k 3
#define SAVE_TYPE_EEP16k 4
#define SAVE_TYPE_FLASH 5
*/


//switch to the correct dump function
int getSaveFromCart(int stype, uint8_t *buffer) {
    int ret=0;

    switch(stype) {
        case 0: return 0;
        case 1: ret = getSRAM32(buffer);	break;
        case 2: ret = getSRAM128(buffer);   break;
        case 3: ret = getEeprom4k(buffer);  break;
        case 4: ret = getEeprom16k(buffer); break;
        case 5: ret = getFlashRAM(buffer);  break;
        default: return 0;
    }

    return ret;
}


//switch to the correct upload function
int pushSaveToCart(int stype, uint8_t *buffer){
    int ret=0;

    switch(stype) {
        case 0: return 0;
        case 1: ret = setSRAM32(buffer);	break;
        case 2: ret = setSRAM128(buffer);   break;
        case 3: ret = setEeprom4k(buffer);  break;
        case 4: ret = setEeprom16k(buffer); break;
        case 5: ret = setFlashRAM(buffer);  break;
        default: return 0;
    }

    return ret;
}


int getSRAM( uint8_t *buffer, int size){
    while (dma_busy()) ;

    IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x05);
    IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x0C);
    IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x0D);
    IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x02);

    while (dma_busy()) ;

    PI_Init();

    sleep(1000);

    while (dma_busy()) ;

    PI_DMAFromSRAM(buffer, 0, size) ;

    while (dma_busy()) ;

    IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x40);
    IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x12);
    IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x07);
    IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x03);

    return 1;
}


int getSRAM32(  uint8_t *buffer) {
    return getSRAM(buffer, SAVE_SIZE_SRAM);
}

int getSRAM128( uint8_t *buffer) {
    return getSRAM(buffer, SAVE_SIZE_SRAM128);
}


//working hurray :D
int getEeprom4k(  uint8_t *buffer) {
    if(eeprom_present()){
        int blocks=SAVE_SIZE_EEP4k/8;
        for( int b = 0; b < blocks; b++ ) {
            eeprom_read( b, &buffer[b * 8] );
        }
        return 1;
    }

    return 0;
}

int getEeprom16k(  uint8_t *buffer){
    int blocks=SAVE_SIZE_EEP16k/8;
    for( int b = 0; b < blocks; b++ ) {
        eeprom_read( b, &buffer[b * 8] );
    }

    return 1;
}

int getFlashRAM( uint8_t *buffer){
    evd_setSaveType(SAVE_TYPE_SRAM128); //2
    sleep(10);

    int s = getSRAM(buffer, SAVE_SIZE_SRAM128);
    data_cache_hit_writeback_invalidate(buffer,SAVE_SIZE_SRAM128);

    sleep(10);
    evd_setSaveType(SAVE_TYPE_FLASH); //5

    return 1;
}


/*
sram upload
*/
int setSRAM(  uint8_t *buffer,int size){
     //half working
    PI_DMAWait();
    //Timing
    PI_Init_SRAM();

    //Readmode
    PI_Init();

    data_cache_hit_writeback_invalidate(buffer,size);
     while (dma_busy());
    PI_DMAToSRAM(buffer, 0, size);
    data_cache_hit_writeback_invalidate(buffer,size);

    //Wait
    PI_DMAWait();
    //Restore evd Timing
    setSDTiming();

    return 1;
}

int setSRAM32( uint8_t *buffer){
    return setSRAM(buffer, SAVE_SIZE_SRAM);
}

int setSRAM128( uint8_t *buffer){
    return setSRAM(buffer, SAVE_SIZE_SRAM128);
}


//working hurray :D
int setEeprom4k( uint8_t *buffer){
    if(eeprom_present()){
        int blocks=SAVE_SIZE_EEP4k/8;
        for( int b = 0; b < blocks; b++ ) {
            eeprom_write( b, &buffer[b * 8] );
        }
        return 1;
    }

    return 0;
}

int setEeprom16k(uint8_t *buffer){
    int blocks=SAVE_SIZE_EEP16k/8;
    for( int b = 0; b < blocks; b++ ) {
        eeprom_write( b, &buffer[b * 8] );
    }

    return 1;
}


//isn't working nor finished
int setFlashRAM(uint8_t *buffer){
    evd_setSaveType(SAVE_TYPE_SRAM128); //2
    sleep(10);

    int s = setSRAM(buffer, SAVE_SIZE_SRAM128);

    evd_setSaveType(SAVE_TYPE_FLASH); //5

    return 1;
}

void setSDTiming(void){
/*
PI_BSD_DOM1_LAT_REG (0x04600014) write word 0x000000FF
PI_BSD_DOM1_PWD_REG (0x04600018) write word 0x000000FF
PI_BSD_DOM1_PGS_REG (0x0460001C) write word 0x0000000F
PI_BSD_DOM1_RLS_REG (0x04600020) write word 0x00000003
*
PI_BSD_DOM1_LAT_REG (0x04600014) write word 0x00000040
PI_BSD_DOM1_PWD_REG (0x04600018) write word 0x00803712
PI_BSD_DOM1_PGS_REG (0x0460001C) write word 0x00008037
PI_BSD_DOM1_RLS_REG (0x04600020) write word 0x00000803
*/
    // PI_DMAWait();
    IO_WRITE(PI_BSD_DOM1_LAT_REG, 0x40);
    IO_WRITE(PI_BSD_DOM1_PWD_REG, 0x12);
    IO_WRITE(PI_BSD_DOM1_PGS_REG, 0x07);
    IO_WRITE(PI_BSD_DOM1_RLS_REG, 0x03);

    IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x40);
    IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x12);
    IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x07);
    IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x03);
}


void _data_cache_invalidate_all(void) {
    asm(
        "li $8,0x80000000\n"
        "li $9,0x80000000\n"
        "addu $9,$9,0x1FF0\n"
        "cacheloop:\n"
        "cache 1,0($8)\n"
        "cache 1,16($8)\n"
        "cache 1,32($8)\n"
        "cache 1,48($8)\n"
        "cache 1,64($8)\n"
        "cache 1,80($8)\n"
        "cache 1,96($8)\n"
        "addu $8,$8,112\n"
        "bne $8,$9,cacheloop\n"
        "cache 1,0($8)\n"
    : // no outputs
    : // no inputs
    : "$8", "$9" // trashed registers
    );
}

void restoreTiming(void) {
    //n64 timing restore :>
    IO_WRITE(PI_BSD_DOM1_LAT_REG, 0x40);
    IO_WRITE(PI_BSD_DOM1_PWD_REG, 0x12);
    IO_WRITE(PI_BSD_DOM1_PGS_REG, 0x07);
    IO_WRITE(PI_BSD_DOM1_RLS_REG, 0x03);

    IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x40);
    IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x12);
    IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x07);
    IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x03);
}

/*
 * Load an image from the rom filesystem, returning a pointer to the
 * sprite that hold the image.
 */
sprite_t *loadImageDFS(char *fname) {
    int size, x, y, n, fd;
    u8 *tbuf;
    u8 *ibuf;
    sprite_t *sbuf;

    fd = dfs_open(fname);
    if (fd < 0)
        return 0;                       // couldn't open image

    size = dfs_size(fd);
    tbuf = malloc(size);
    if (!tbuf) {
        dfs_close(fd);
        return 0;                       // out of memory
    }

    dfs_read(tbuf, 1, size, fd);
    dfs_close(fd);

    ibuf = stbi_load_from_memory(tbuf, size, &x, &y, &n, 4);
    free(tbuf);
    if (!ibuf)
        return 0;                       // couldn't decode image

    sbuf = (sprite_t*)malloc(sizeof(sprite_t) + x * y * 2);
    if (!sbuf) {
        stbi_image_free(ibuf);
        return 0;                       // out of memory
    }
    sbuf->width = x;
    sbuf->height = y;
    sbuf->bitdepth = 2;
    sbuf->format = 0;
    sbuf->hslices = x / 32;
    sbuf->vslices = y / 16;

    color_t *src = (color_t*)ibuf;
    u16 *dst = (u16*)((u32)sbuf + sizeof(sprite_t));

    for (int j=0; j<y; j++)
        for (int i=0; i<x; i++)
            dst[i + j*x] = graphics_convert_color(src[i + j*x]) & 0x0000FFFF;

    /* Invalidate data associated with sprite in cache */
    data_cache_hit_writeback_invalidate( sbuf->data, sbuf->width * sbuf->height * sbuf->bitdepth );

    stbi_image_free(ibuf);
    return sbuf;
}


sprite_t *loadImage32(u8 *png, int size) {
    int x, y, n, fd;
    u8 *tbuf;
    u32 *ibuf;
    sprite_t *sbuf;

    tbuf = malloc(size);
    memcpy(tbuf,png,size);

    ibuf = (u32*)stbi_load_from_memory(tbuf, size, &x, &y, &n, 4);
    free(tbuf);
    if (!ibuf)
        return 0;                       // couldn't decode image

    sbuf = (sprite_t*)malloc(sizeof(sprite_t) + x * y * 4);
    if (!sbuf) {
        stbi_image_free(ibuf);
        return 0;                       // out of memory
    }

    sbuf->width = x;
    sbuf->height = y;
    sbuf->bitdepth = 4;
    sbuf->format = 0;
    sbuf->hslices = x / 32;
    sbuf->vslices = y / 32;

   // color_t *src = (color_t*)ibuf;
    u32 *dst = (u32*)((u32)sbuf + sizeof(sprite_t));

    for (int j=0; j<y; j++)
        for (int i=0; i<x; i++)
            dst[i + j*x] = ibuf[i + j*x];

    /* Invalidate data associated with sprite in cache */
    data_cache_hit_writeback_invalidate( sbuf->data, sbuf->width * sbuf->height * sbuf->bitdepth );

    stbi_image_free(ibuf);
    return sbuf;
}

sprite_t *loadImage32DFS(char *fname) {
    int size, x, y, n, fd;
    u8 *tbuf;
    u32 *ibuf;
    sprite_t *sbuf;

    fd = dfs_open(fname);
    if (fd < 0)
        return 0;                       // couldn't open image

    size = dfs_size(fd);
    tbuf = malloc(size);
    if (!tbuf) {
        dfs_close(fd);
        return 0;                       // out of memory
    }

    dfs_read(tbuf, 1, size, fd);
    dfs_close(fd);

    ibuf = (u32*)stbi_load_from_memory(tbuf, size, &x, &y, &n, 4);
    free(tbuf);
    if (!ibuf)
        return 0;                       // couldn't decode image

    sbuf = (sprite_t*)malloc(sizeof(sprite_t) + x * y * 4);
    if (!sbuf) {
        stbi_image_free(ibuf);
        return 0;                       // out of memory
    }

    sbuf->width = x;
    sbuf->height = y;
    sbuf->bitdepth = 4;
    sbuf->format = 0;
    sbuf->hslices = x / 32;
    sbuf->vslices = y / 32;

   // color_t *src = (color_t*)ibuf;
    u32 *dst = (u32*)((u32)sbuf + sizeof(sprite_t));

    for (int j=0; j<y; j++)
        for (int i=0; i<x; i++)
            dst[i + j*x] = ibuf[i + j*x];

    /* Invalidate data associated with sprite in cache */
    data_cache_hit_writeback_invalidate( sbuf->data, sbuf->width * sbuf->height * sbuf->bitdepth );

    stbi_image_free(ibuf);
    return sbuf;
}

/*
 * Draw an image to the screen using the sprite passed.
 */
void drawImage(display_context_t dcon, sprite_t *sprite) {
    int x, y = 0;

    rdp_sync(SYNC_PIPE);
    rdp_set_default_clipping();
    rdp_enable_texture_copy();
    rdp_attach_display(dcon);
    // Draw image
    for (int j=0; j<sprite->vslices; j++) {
        x = 0;
        for (int i=0; i<sprite->hslices; i++) {
            rdp_sync(SYNC_PIPE);
            rdp_load_texture_stride(0, 0, MIRROR_DISABLED, sprite, j*sprite->hslices + i);
            rdp_draw_sprite(0, x, y);
            x += 32;
        }
        y += 16;
    }
    rdp_detach_display();
}


#define CIC_6101 1
#define CIC_6102 2
#define CIC_6103 3
#define CIC_6104 4
#define CIC_6105 5
#define CIC_6106 6


void globalTest(void) {
    gCheats=1;
}


void simulate_boot(u32 cic_chip, u8 gBootCic) {
    if (cic_chip == CIC_6104)
    cic_chip = CIC_6102;

    u32 ix, sz, cart, country;
    vu32 *src, *dst;
    u32 info = *(vu32 *)0xB000003C;
    vu64 *gGPR = (vu64 *)0xA03E0000;
    vu32 *codes = (vu32 *)0xA0000180;
    u32 bootAddr = 0xA4000040;
    char *cp, *vp, *tp;
    char temp[8];
    int i, type, val;
    int curr_cheat = 0;

    cart = info >> 16;
    country = (info >> 8) & 0xFF;

    if(boot_country!=0){
        switch(boot_country){
            case 1: country = 0x45; break; //ntsc region
            case 2: country = 0x50; break; //pal region
            default: break;
        }
    }

    // clear XBUS/Flush/Freeze
    ((vu32 *)0xA4100000)[3] = 0x15;

    sz = (gBootCic != CIC_6105) ? *(vu32 *)0xA0000318 : *(vu32 *)0xA00003F0;
    if (cic_chip == CIC_6105)
        *(vu32 *)0xA00003F0 = sz;
    else
        *(vu32 *)0xA0000318 = sz;

    // clear some OS globals for cleaner boot
    *(vu32 *)0xA000030C = 0;             // cold boot
    memset((void *)0xA000031C, 0, 64);   // clear app nmi buffer

    if (gCheats) {
        u16 xv, yv, zv;
        u32 xx;
        vu32 *sp, *dp;
        // get rom os boot segment - note, memcpy won't work for copying rom
        sp = (vu32 *)0xB0001000;
        dp = (vu32 *)0xA02A0000;
        for (ix=0; ix<0x100000; ix++)
            *dp++ = *sp++;
        // default boot address with cheats
        sp = (vu32 *)0xB0000008;
        bootAddr = 0x00000000 | *sp;

        // move general int handler
        sp = (vu32 *)0xA0000180;
        dp = (vu32 *)0xA0000120;
        for (ix=0; ix<0x60; ix+=4)
            *dp++ = *sp++;

        // insert new general int handler prologue
        *codes++ = 0x401a6800;  // mfc0     k0,c0_cause
        *codes++ = 0x241b005c;  // li       k1,23*4
        *codes++ = 0x335a007c;  // andi     k0,k0,0x7c
        *codes++ = 0x175b0012;  // bne      k0,k1,0x1d8
        *codes++ = 0x00000000;  // nop
        *codes++ = 0x40809000;  // mtc0     zero,c0_watchlo
        *codes++ = 0x401b7000;  // mfc0     k1,c0_epc
        *codes++ = 0x8f7a0000;  // lw       k0,0(k1)
        *codes++ = 0x3c1b03e0;  // lui      k1,0x3e0
        *codes++ = 0x035bd024;  // and      k0,k0,k1
        *codes++ = 0x001ad142;  // srl      k0,k0,0x5
        *codes++ = 0x3c1ba000;  // lui      k1,0xa000
        *codes++ = 0x8f7b01cc;  // lw       k1,0x01cc(k1)
        *codes++ = 0x035bd025;  // or       k0,k0,k1
        *codes++ = 0x3c1ba000;  // lui      k1,0xa000
        *codes++ = 0xaf7a01cc;  // sw       k0,0x01cc(k1)
        *codes++ = 0x3c1b8000;  // lui      k1,0x8000
        *codes++ = 0xbf7001cc;  // cache    0x10,0x01cc(k1)
        *codes++ = 0x3c1aa000;  // lui      k0,0xa000
        *codes++ = 0x37400120;  // ori      zero,k0,0x120
        *codes++ = 0x42000018;  // eret
        *codes++ = 0x00000000;  // nop

        // process cheats
        while (gGSCodes[curr_cheat].count != 0xFFFF) {
            if (!gGSCodes[curr_cheat].state || !gGSCodes[curr_cheat].count) {
                // cheat not enabled or no codes, skip
                curr_cheat++;
                continue;
            }

            for (i=0; i<gGSCodes[curr_cheat].count; i++) {
                cp = &gGSCodes[curr_cheat].gscodes[i*16 + 0];
                vp = &gGSCodes[curr_cheat].gscodes[i*16 + 9];

                temp[0] = cp[0];
                temp[1] = cp[1];
                temp[2] = 0;
                type = strtol(temp, (char **)NULL, 16);

                switch(type) {
                    case 0x80:
                        // write 8-bit value to (cached) ram continuously
                        // 80XXYYYY 00ZZ
                        temp[0] = cp[2];
                        temp[1] = cp[3];
                        temp[2] = 0;
                        xv = strtol(temp, (char **)NULL, 16);
                        temp[0] = cp[4];
                        temp[1] = cp[5];
                        temp[2] = cp[6];
                        temp[3] = cp[7];
                        temp[4] = 0;
                        yv = strtol(temp, (char **)NULL, 16);
                        if (yv & 0x8000) xv++; // adjust for sign extension of yv
                        if (gGSCodes[curr_cheat].mask) {
                            zv = gGSCodes[curr_cheat].value & gGSCodes[curr_cheat].mask;
                        }
                        else {
                            temp[0] = vp[2];
                            temp[1] = vp[3];
                            temp[2] = 0;
                            zv = strtol(temp, (char **)NULL, 16);
                        }
                        *codes++ = 0x3c1a8000 | xv; // lui  k0,80xx
                        *codes++ = 0x241b0000 | zv; // li   k1,00zz
                        *codes++ = 0xa35b0000 | yv; // sb   k1,yyyy(k0)
                        break;

                    case 0x81:
                        // write 16-bit value to (cached) ram continuously
                        // 81XXYYYY 00ZZ
                        temp[0] = cp[2];
                        temp[1] = cp[3];
                        temp[2] = 0;
                        xv = strtol(temp, (char **)NULL, 16);
                        temp[0] = cp[4];
                        temp[1] = cp[5];
                        temp[2] = cp[6];
                        temp[3] = cp[7];
                        temp[4] = 0;
                        yv = strtol(temp, (char **)NULL, 16);
                        if (yv & 0x8000) xv++; // adjust for sign extension of yv
                        if (gGSCodes[curr_cheat].mask) {
                            zv = gGSCodes[curr_cheat].value & gGSCodes[curr_cheat].mask;
                        }
                        else
                        {
                            temp[0] = vp[0];
                            temp[1] = vp[1];
                            temp[2] = vp[2];
                            temp[3] = vp[3];
                            temp[4] = 0;
                            zv = strtol(temp, (char **)NULL, 16);
                        }
                        *codes++ = 0x3c1a8000 | xv; // lui  k0,80xx
                        *codes++ = 0x241b0000 | zv; // li   k1,zzzz
                        *codes++ = 0xa75b0000 | yv; // sh   k1,yyyy(k0)
                        break;

                    case 0x88:
                        // write 8-bit value to (cached) ram on GS button pressed - unimplemented
                        // 88XXYYYY 00ZZ
                        break;

                    case 0x89:
                        // write 16-bit value to (cached) ram on GS button pressed - unimplemented
                        // 89XXYYYY ZZZZ
                        break;

                    case 0xA0:
                        // write 8-bit value to (uncached) ram continuously
                        // A0XXYYYY 00ZZ
                        temp[0] = cp[2];
                        temp[1] = cp[3];
                        temp[2] = 0;
                        xv = strtol(temp, (char **)NULL, 16);
                        temp[0] = cp[4];
                        temp[1] = cp[5];
                        temp[2] = cp[6];
                        temp[3] = cp[7];
                        temp[4] = 0;
                        yv = strtol(temp, (char **)NULL, 16);
                        if (yv & 0x8000) xv++; // adjust for sign extension of yv
                        if (gGSCodes[curr_cheat].mask) {
                            zv = gGSCodes[curr_cheat].value & gGSCodes[curr_cheat].mask;
                        }
                        else {
                            temp[0] = vp[2];
                            temp[1] = vp[3];
                            temp[2] = 0;
                            zv = strtol(temp, (char **)NULL, 16);
                        }
                        *codes++ = 0x3c1aa000 | xv; // lui  k0,A0xx
                        *codes++ = 0x241b0000 | zv; // li   k1,00zz
                        *codes++ = 0xa35b0000 | yv; // sb   k1,yyyy(k0)
                        break;

                    case 0xA1:
                        // write 16-bit value to (uncached) ram continuously
                        // A1XXYYYY 00ZZ
                        temp[0] = cp[2];
                        temp[1] = cp[3];
                        temp[2] = 0;
                        xv = strtol(temp, (char **)NULL, 16);
                        temp[0] = cp[4];
                        temp[1] = cp[5];
                        temp[2] = cp[6];
                        temp[3] = cp[7];
                        temp[4] = 0;
                        yv = strtol(temp, (char **)NULL, 16);
                        if (yv & 0x8000) xv++; // adjust for sign extension of yv
                        if (gGSCodes[curr_cheat].mask) {
                            zv = gGSCodes[curr_cheat].value & gGSCodes[curr_cheat].mask;
                        }
                        else {
                            temp[0] = vp[0];
                            temp[1] = vp[1];
                            temp[2] = vp[2];
                            temp[3] = vp[3];
                            temp[4] = 0;
                            zv = strtol(temp, (char **)NULL, 16);
                        }
                        *codes++ = 0x3c1aa000 | xv; // lui  k0,A0xx
                        *codes++ = 0x241b0000 | zv; // li   k1,zzzz
                        *codes++ = 0xa75b0000 | yv; // sh   k1,yyyy(k0)
                        break;

                    case 0xCC:
                        // deactivate expansion ram using 3rd method
                        // CC000000 0000
                        if (cic_chip == CIC_6105)
                            *(vu32 *)0xA00003F0 = 0x00400000;
                        else
                            *(vu32 *)0xA0000318 = 0x00400000;
                        break;

                    case 0xD0:
                        // do next gs code if ram location is equal to 8-bit value
                        // D0XXYYYY 00ZZ
                        temp[0] = cp[2];
                        temp[1] = cp[3];
                        temp[2] = 0;
                        xv = strtol(temp, (char **)NULL, 16);
                        temp[0] = cp[4];
                        temp[1] = cp[5];
                        temp[2] = cp[6];
                        temp[3] = cp[7];
                        temp[4] = 0;
                        yv = strtol(temp, (char **)NULL, 16);
                        if (yv & 0x8000) xv++; // adjust for sign extension of yv
                        temp[0] = vp[2];
                        temp[1] = vp[3];
                        temp[2] = 0;
                        zv = strtol(temp, (char **)NULL, 16);
                        *codes++ = 0x3c1a8000 | xv; // lui  k0,0x80xx
                        *codes++ = 0x835a0000 | yv; // lb   k0,yyyy(k0)
                        *codes++ = 0x241b0000 | zv; // li   k1,00zz
                        *codes++ = 0x175b0004;      // bne  k0,k1,4
                        *codes++ = 0x00000000;      // nop
                        break;

                    case 0xD1:
                        // do next gs code if ram location is equal to 16-bit value
                        // D1XXYYYY 00ZZ
                        temp[0] = cp[2];
                        temp[1] = cp[3];
                        temp[2] = 0;
                        xv = strtol(temp, (char **)NULL, 16);
                        temp[0] = cp[4];
                        temp[1] = cp[5];
                        temp[2] = cp[6];
                        temp[3] = cp[7];
                        temp[4] = 0;
                        yv = strtol(temp, (char **)NULL, 16);
                        if (yv & 0x8000) xv++; // adjust for sign extension of yv
                        temp[0] = vp[0];
                        temp[1] = vp[1];
                        temp[2] = vp[2];
                        temp[3] = vp[3];
                        temp[4] = 0;
                        zv = strtol(temp, (char **)NULL, 16);
                        *codes++ = 0x3c1a8000 | xv; // lui  k0,0x80xx
                        *codes++ = 0x875a0000 | yv; // lh   k0,yyyy(k0)
                        *codes++ = 0x241b0000 | zv; // li   k1,zzzz
                        *codes++ = 0x175b0004;      // bne  k0,k1,4
                        *codes++ = 0x00000000;      // nop
                        break;

                    case 0xD2:
                        // do next gs code if ram location is not equal to 8-bit value
                        // D2XXYYYY 00ZZ
                        temp[0] = cp[2];
                        temp[1] = cp[3];
                        temp[2] = 0;
                        xv = strtol(temp, (char **)NULL, 16);
                        temp[0] = cp[4];
                        temp[1] = cp[5];
                        temp[2] = cp[6];
                        temp[3] = cp[7];
                        temp[4] = 0;
                        yv = strtol(temp, (char **)NULL, 16);
                        if (yv & 0x8000) xv++; // adjust for sign extension of yv
                        temp[0] = vp[2];
                        temp[1] = vp[3];
                        temp[2] = 0;
                        zv = strtol(temp, (char **)NULL, 16);
                        *codes++ = 0x3c1a8000 | xv; // lui  k0,0x80xx
                        *codes++ = 0x835a0000 | yv; // lb   k0,yyyy(k0)
                        *codes++ = 0x241b0000 | zv; // li   k1,00zz
                        *codes++ = 0x135b0004;      // beq  k0,k1,4
                        *codes++ = 0x00000000;      // nop
                        break;

                    case 0xD3:
                        // do next gs code if ram location is not equal to 16-bit value
                        // D3XXYYYY 00ZZ
                        temp[0] = cp[2];
                        temp[1] = cp[3];
                        temp[2] = 0;
                        xv = strtol(temp, (char **)NULL, 16);
                        temp[0] = cp[4];
                        temp[1] = cp[5];
                        temp[2] = cp[6];
                        temp[3] = cp[7];
                        temp[4] = 0;
                        yv = strtol(temp, (char **)NULL, 16);
                        if (yv & 0x8000) xv++; // adjust for sign extension of yv
                        temp[0] = vp[0];
                        temp[1] = vp[1];
                        temp[2] = vp[2];
                        temp[3] = vp[3];
                        temp[4] = 0;
                        zv = strtol(temp, (char **)NULL, 16);
                        *codes++ = 0x3c1a8000 | xv; // lui  k0,0x80xx
                        *codes++ = 0x875a0000 | yv; // lh   k0,yyyy(k0)
                        *codes++ = 0x241b0000 | zv; // li   k1,zzzz
                        *codes++ = 0x135b0004;      // beq  k0,k1,4
                        *codes++ = 0x00000000;      // nop
                        break;

                    case 0xDD:
                        // deactivate expansion ram using 2nd method
                        // DD000000 0000
                        if (cic_chip == CIC_6105)
                            *(vu32 *)0xA00003F0 = 0x00400000;
                        else
                            *(vu32 *)0xA0000318 = 0x00400000;
                        break;

                    case 0xDE:
                        // set game boot address
                        // DEXXXXXX 0000 => boot address = 800XXXXX, msn ignored
                        temp[0] = cp[2];
                        temp[1] = cp[3];
                        temp[2] = cp[4];
                        temp[3] = cp[5];
                        temp[4] = cp[6];
                        temp[5] = cp[7];
                        temp[6] = 0;
                        val = strtol(temp, (char **)NULL, 16);
                        bootAddr = 0x80000000 | (val & 0xFFFFF);
                        break;

                    case 0xEE:
                        // deactivate expansion ram using 1st method
                        // EE000000 0000
                        if (cic_chip == CIC_6105)
                            *(vu32 *)0xA00003F0 = 0x00400000;
                        else
                            *(vu32 *)0xA0000318 = 0x00400000;
                        break;

                    case 0xF0:
                        // write 8-bit value to ram before boot
                        // F0XXXXXX 00YY
                        temp[0] = cp[2];
                        temp[1] = cp[3];
                        temp[2] = cp[4];
                        temp[3] = cp[5];
                        temp[4] = cp[6];
                        temp[5] = cp[7];
                        temp[6] = 0;
                        val = strtol(temp, (char **)NULL, 16);
                        val -= (bootAddr & 0xFFFFFF);
                        tp = (char *)(0xA02A0000 + val);
                        if (gGSCodes[curr_cheat].mask) {
                            val = gGSCodes[curr_cheat].value & gGSCodes[curr_cheat].mask;
                        }
                        else {
                            temp[0] = vp[2];
                            temp[1] = vp[3];
                            temp[2] = 0;
                            val = strtol(temp, (char **)NULL, 16);
                        }
                        *tp = val & 0x00FF;
                        break;

                    case 0xF1:
                        // write 16-bit value to ram before boot
                        // F1XXXXXX YYYY
                        temp[0] = cp[2];
                        temp[1] = cp[3];
                        temp[2] = cp[4];
                        temp[3] = cp[5];
                        temp[4] = cp[6];
                        temp[5] = cp[7];
                        temp[6] = 0;
                        val = strtol(temp, (char **)NULL, 16);
                        val -= (bootAddr & 0xFFFFFF);
                        tp = (char *)(0xA02A0000 + val);
                        if (gGSCodes[curr_cheat].mask) {
                            val = gGSCodes[curr_cheat].value & gGSCodes[curr_cheat].mask;
                        }
                        else {
                            temp[0] = vp[0];
                            temp[1] = vp[1];
                            temp[2] = vp[2];
                            temp[3] = vp[3];
                            temp[4] = 0;
                            val = strtol(temp, (char **)NULL, 16);
                        }
                        *tp++ = (val >> 8) & 0x00FF;
                        *tp = val & 0x00FF;
                        break;

                    case 0xFF:
                        // set code base
                        // FFXXXXXX 0000
                        temp[0] = cp[2];
                        temp[1] = cp[3];
                        temp[2] = cp[4];
                        temp[3] = cp[5];
                        temp[4] = cp[6];
                        temp[5] = cp[7];
                        temp[6] = 0;
                        val = strtol(temp, (char **)NULL, 16);
                        //codes = (vu32 *)(0xA0000000 | (val & 0xFFFFFF));
                        break;
                }
            }
            curr_cheat++;
        }

        // generate jump to moved general int handler
        *codes++ = 0x3c1a8000;  // lui  k0,0x8000
        *codes++ = 0x375a0120;  // ori  k0,k0,0x120
        *codes++ = 0x03400008;  // jr   k0
        *codes++ = 0x00000000;  // nop

        // flush general int handler memory
        data_cache_hit_writeback_invalidate((void *)0x80000120, 0x2E0);
        inst_cache_hit_invalidate((void *)0x80000120, 0x2E0);

        // flush os boot segment
        data_cache_hit_writeback_invalidate((void *)0x802A0000, 0x100000);

        // flush os boot segment memory
        data_cache_hit_writeback_invalidate((void *)bootAddr, 0x100000);
        inst_cache_hit_invalidate((void *)bootAddr, 0x100000);
    }

    // Copy low 0x1000 bytes to DMEM
    //copy bootcode to RSP DMEM starting at 0xA4000040; <- bootaddr
    src = (vu32 *)0xB0000000; //i think 0xB0000040; is the right value
    dst = (vu32 *)0xA4000000; //0xA4000040;
    for (ix=0; ix<(0x1000>>2); ix++)
        dst[ix] = src[ix];


    // Need to copy crap to IMEM for CIC-6105 boot.
    dst = (vu32 *)0xA4001000;

    // register values due to pif boot for CiC chip and country code, and IMEM crap
    gGPR[0]=0x0000000000000000LL;
    gGPR[6]=0xFFFFFFFFA4001F0CLL;
    gGPR[7]=0xFFFFFFFFA4001F08LL;
    gGPR[8]=0x00000000000000C0LL;
    gGPR[9]=0x0000000000000000LL;
    gGPR[10]=0x0000000000000040LL;
    gGPR[11]=bootAddr; // 0xFFFFFFFFA4000040LL;
    gGPR[16]=0x0000000000000000LL;
    gGPR[17]=0x0000000000000000LL;
    gGPR[18]=0x0000000000000000LL;
    gGPR[19]=0x0000000000000000LL;
    gGPR[21]=0x0000000000000000LL;
    gGPR[26]=0x0000000000000000LL;
    gGPR[27]=0x0000000000000000LL;
    gGPR[28]=0x0000000000000000LL;
    gGPR[29]=0xFFFFFFFFA4001FF0LL;
    gGPR[30]=0x0000000000000000LL;

    switch (country) {
        case 0x44: //Germany
        case 0x46: //french
        case 0x49: //Italian
        case 0x50: //Europe
        case 0x53: //Spanish
        case 0x55: //Australia
        case 0x58: // ????
        case 0x59: // X (PAL)
            if(force_tv!=0)
                *(u32 *) 0x80000300 = force_tv; //pal
            else
                *(u32 *) 0x80000300 = 2;

            switch (cic_chip) {
                case CIC_6102:
                    gGPR[5]=0xFFFFFFFFC0F1D859LL;
                    gGPR[14]=0x000000002DE108EALL;
                    gGPR[24]=0x0000000000000000LL;
                    break;

                case CIC_6103:
                    gGPR[5]=0xFFFFFFFFD4646273LL;
                    gGPR[14]=0x000000001AF99984LL;
                    gGPR[24]=0x0000000000000000LL;
                    break;

                case CIC_6105:
                    dst[0x04>>2] = 0xBDA807FC;
                    gGPR[5]=0xFFFFFFFFDECAAAD1LL;
                    gGPR[14]=0x000000000CF85C13LL;
                    gGPR[24]=0x0000000000000002LL;
                    break;

                case CIC_6106:
                    gGPR[5]=0xFFFFFFFFB04DC903LL;
                    gGPR[14]=0x000000001AF99984LL;
                    gGPR[24]=0x0000000000000002LL;
                    break;
            }

            gGPR[20]=0x0000000000000000LL;
            gGPR[23]=0x0000000000000006LL;
            gGPR[31]=0xFFFFFFFFA4001554LL;
            break;

        case 0x37: // 7 (Beta)
        case 0x41: // ????
        case 0x45: //USA
        case 0x4A: //Japan
            if(force_tv!=0)
                *(u32 *) 0x80000300 = force_tv; //pal
            else
                *(u32 *) 0x80000300 = 1; //ntsc

            // Fall-through

        default:
            switch (cic_chip) {
                case CIC_6102:
                    gGPR[5]=0xFFFFFFFFC95973D5LL;
                    gGPR[14]=0x000000002449A366LL;
                    break;

                case CIC_6103:
                    gGPR[5]=0xFFFFFFFF95315A28LL;
                    gGPR[14]=0x000000005BACA1DFLL;
                    break;

                case CIC_6105:
                    dst[0x04>>2] = 0x8DA807FC;
                    gGPR[5]=0x000000005493FB9ALL;
                    gGPR[14]=0xFFFFFFFFC2C20384LL;
                    break;

                case CIC_6106:
                    gGPR[5]=0xFFFFFFFFE067221FLL;
                    gGPR[14]=0x000000005CD2B70FLL;
                    break;
            }

            gGPR[20]=0x0000000000000001LL;
            gGPR[23]=0x0000000000000000LL;
            gGPR[24]=0x0000000000000003LL;
            gGPR[31]=0xFFFFFFFFA4001550LL;
            break;
    }

    switch (cic_chip) {
        case CIC_6101:
            gGPR[22]=0x000000000000003FLL;
            break;

        case CIC_6102:
            gGPR[1]=0x0000000000000001LL;
            gGPR[2]=0x000000000EBDA536LL;
            gGPR[3]=0x000000000EBDA536LL;
            gGPR[4]=0x000000000000A536LL;
            gGPR[12]=0xFFFFFFFFED10D0B3LL;
            gGPR[13]=0x000000001402A4CCLL;
            gGPR[15]=0x000000003103E121LL;
            gGPR[22]=0x000000000000003FLL;
            gGPR[25]=0xFFFFFFFF9DEBB54FLL;
            break;

        case CIC_6103:
            gGPR[1]=0x0000000000000001LL;
            gGPR[2]=0x0000000049A5EE96LL;
            gGPR[3]=0x0000000049A5EE96LL;
            gGPR[4]=0x000000000000EE96LL;
            gGPR[12]=0xFFFFFFFFCE9DFBF7LL;
            gGPR[13]=0xFFFFFFFFCE9DFBF7LL;
            gGPR[15]=0x0000000018B63D28LL;
            gGPR[22]=0x0000000000000078LL;
            gGPR[25]=0xFFFFFFFF825B21C9LL;
            break;

        case CIC_6105:
            dst[0x00>>2] = 0x3C0DBFC0;
            dst[0x08>>2] = 0x25AD07C0;
            dst[0x0C>>2] = 0x31080080;
            dst[0x10>>2] = 0x5500FFFC;
            dst[0x14>>2] = 0x3C0DBFC0;
            dst[0x18>>2] = 0x8DA80024;
            dst[0x1C>>2] = 0x3C0BB000;
            gGPR[1]=0x0000000000000000LL;
            gGPR[2]=0xFFFFFFFFF58B0FBFLL;
            gGPR[3]=0xFFFFFFFFF58B0FBFLL;
            gGPR[4]=0x0000000000000FBFLL;
            gGPR[12]=0xFFFFFFFF9651F81ELL;
            gGPR[13]=0x000000002D42AAC5LL;
            gGPR[15]=0x0000000056584D60LL;
            gGPR[22]=0x0000000000000091LL;
            gGPR[25]=0xFFFFFFFFCDCE565FLL;
            break;

        case CIC_6106:
            gGPR[1]=0x0000000000000000LL;
            gGPR[2]=0xFFFFFFFFA95930A4LL;
            gGPR[3]=0xFFFFFFFFA95930A4LL;
            gGPR[4]=0x00000000000030A4LL;
            gGPR[12]=0xFFFFFFFFBCB59510LL;
            gGPR[13]=0xFFFFFFFFBCB59510LL;
            gGPR[15]=0x000000007A3C07F4LL;
            gGPR[22]=0x0000000000000085LL;
            gGPR[25]=0x00000000465E3F72LL;
            break;
    }

    // set HW registers
    IO_WRITE(PI_STATUS_REG, 0x03);
    switch (cart) {
        case 0x4258: // 'BX' - Battle Tanx
            IO_WRITE(PI_BSD_DOM1_LAT_REG, 0x80);
            IO_WRITE(PI_BSD_DOM1_RLS_REG, 0x37);
            IO_WRITE(PI_BSD_DOM1_PWD_REG, 0x12);
            IO_WRITE(PI_BSD_DOM1_PGS_REG, 0x40);
            break;

        case 0x4237: // 'B7' - Banjo Tooie
            IO_WRITE(PI_BSD_DOM1_LAT_REG, 0x80);
            IO_WRITE(PI_BSD_DOM1_RLS_REG, 0x37);
            IO_WRITE(PI_BSD_DOM1_PWD_REG, 0x12);
            IO_WRITE(PI_BSD_DOM1_PGS_REG, 0x40);
            break;

        case 0x5A4C: // 'ZL' - Zelda OOT
/*
            IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x00000005);
            IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x0000000C);
            IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x0000000D);
            IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x00000002);
*/

/* unstable
            IO_WRITE(PI_BSD_DOM1_LAT_REG, 0x00000040);
            IO_WRITE(PI_BSD_DOM1_RLS_REG, 0x00803712);
            IO_WRITE(PI_BSD_DOM1_PWD_REG, 0x00008037);
            IO_WRITE(PI_BSD_DOM1_PGS_REG, 0x00000803);
*/
            break;

        default:
            break;
    }

/*
load immediate:

li	register_destination, value
#load immediate value into destination register

 ---

 Registers in coprocessor 0 cannot be used directly by MIPS instructions. Instead, there are two
 instructions that work much like load and store instructions. The mfc0 (move from coprocessor 0) instruction
 loads data from a coprocessor 0 register into a CPU register. The mtc0 likewise stores data in a cp0 register.

Note

The mtc0 instruction, like the store instruction has the destination last. This is especially important to note,
 since the syntax for cp0 registers looks the same as the syntax for CPU registers. For example, the following copies
 the contents of CPU register 13 to cp0 register 12.
    mtc0    $13, $12


     mthi	010001	MoveTo	hi = $s


 LUI -- Load upper immediate

Description:
The immediate value is shifted left 16 bits and stored in the register. The lower 16 bits are zeroes.
Operation:
$t = (imm << 16); advance_pc (4);
Syntax:
lui $t, imm
Encoding:
0011 11-- ---t tttt iiii iiii iiii iiii

 * */

    // now set MIPS registers - set CP0, and then GPRs, then jump thru gpr11 (which is usually 0xA400040)
    if (!gCheats)
        asm(".set noat\n\t"
            ".set noreorder\n\t"
            "li $8,0x34000000\n\t"
            "mtc0 $8,$12\n\t"
            "nop\n\t"
            "nop\n\t"
            "li $9,0x0006E463\n\t"
            "mtc0 $9,$16\n\t"
            "nop\n\t"
            "nop\n\t"
            "li $8,0x00005000\n\t"
            "mtc0 $8,$9\n\t"
            "nop\n\t"
            "nop\n\t"
            "li $9,0x0000005C\n\t"
            "mtc0 $9,$13\n\t"
            "nop\n\t"
            "nop\n\t"
            "li $8,0x007FFFF0\n\t"
            "mtc0 $8,$4\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "li $9,0xFFFFFFFF\n\t"
            "mtc0 $9,$14\n\t"
            "nop\n\t"
            "nop\n\t"
            "mtc0 $9,$8\n\t"
            "nop\n\t"
            "nop\n\t"
            "mtc0 $9,$30\n\t"
            "nop\n\t"
            "nop\n\t"
            "lui $8,0\n\t"
            "mthi $8\n\t"
            "nop\n\t"
            "nop\n\t"
            "mtlo $8\n\t"
            "nop\n\t"
            "nop\n\t"
            "ctc1 $8,$31\n\t"
            "nop\n\t"
            "lui $31,0xA03E\n\t"
            //"lui $31,0xA008\n\t"
            "ld $1,0x08($31)\n\t"
            "ld $2,0x10($31)\n\t"
            "ld $3,0x18($31)\n\t"
            "ld $4,0x20($31)\n\t"
            "ld $5,0x28($31)\n\t"
            "ld $6,0x30($31)\n\t"
            "ld $7,0x38($31)\n\t"
            "ld $8,0x40($31)\n\t"
            "ld $9,0x48($31)\n\t"
            "ld $10,0x50($31)\n\t"
            "ld $11,0x58($31)\n\t"
            "ld $12,0x60($31)\n\t"
            "ld $13,0x68($31)\n\t"
            "ld $14,0x70($31)\n\t"
            "ld $15,0x78($31)\n\t"
            "ld $16,0x80($31)\n\t"
            "ld $17,0x88($31)\n\t"
            "ld $18,0x90($31)\n\t"
            "ld $19,0x98($31)\n\t"
            "ld $20,0xA0($31)\n\t"
            "ld $21,0xA8($31)\n\t"
            "ld $22,0xB0($31)\n\t"
            "ld $23,0xB8($31)\n\t"
            "ld $24,0xC0($31)\n\t"
            "ld $25,0xC8($31)\n\t"
            "ld $26,0xD0($31)\n\t"
            "ld $27,0xD8($31)\n\t"
            "ld $28,0xE0($31)\n\t"
            "ld $29,0xE8($31)\n\t"
            "ld $30,0xF0($31)\n\t"
            "ld $31,0xF8($31)\n\t"
            "jr $11\n\t"
            "nop"
            ::: "$8" );

    else
        asm(".set noreorder\n\t"
            "li $8,0x34000000\n\t"
            "mtc0 $8,$12\n\t"
            "nop\n\t"
            "li $9,0x0006E463\n\t"
            "mtc0 $9,$16\n\t"
            "nop\n\t"
            "li $8,0x00005000\n\t"
            "mtc0 $8,$9\n\t"
            "nop\n\t"
            "li $9,0x0000005C\n\t"
            "mtc0 $9,$13\n\t"
            "nop\n\t"
            "li $8,0x007FFFF0\n\t"
            "mtc0 $8,$4\n\t"
            "nop\n\t"
            "li $9,0xFFFFFFFF\n\t"
            "mtc0 $9,$14\n\t"
            "nop\n\t"
            "mtc0 $9,$30\n\t"
            "nop\n\t"
            "lui $8,0\n\t"
            "mthi $8\n\t"
            "nop\n\t"
            "mtlo $8\n\t"
            "nop\n\t"
            "ctc1 $8,$31\n\t"
            "nop\n\t"
            "li $9,0x00000183\n\t"
            "mtc0 $9,$18\n\t"
            "nop\n\t"
            "mtc0 $zero,$19\n\t"
            "nop\n\t"
            "lui $8,0xA03C\n\t"
            "la $9,2f\n\t"
            "la $10,9f\n\t"
            ".set noat\n"
            "1:\n\t"
            "lw $2,($9)\n\t"
            "sw $2,($8)\n\t"
            "addiu $8,$8,4\n\t"
            "addiu $9,$9,4\n\t"
            "bne $9,$10,1b\n\t"
            "nop\n\t"
            "lui $8,0xA03C\n\t"
            "jr $8\n\t"
            "nop\n"
            "2:\n\t"
            "lui $9,0xB000\n\t"
            "lw $9,8($9)\n\t"
            "lui $8,0x2000\n\t"
            "or $8,$8,$9\n\t"
            "lui $9,0xA02A\n\t"
            "lui $10,0xA03A\n\t"
            "3:\n\t"
            "lw $2,($9)\n\t"
            "sw $2,($8)\n\t"
            "addiu $8,$8,4\n\t"
            "addiu $9,$9,4\n\t"
            "bne $9,$10,3b\n\t"
            "nop\n\t"
            "lui $31,0xA03E\n\t"
            "ld $1,0x08($31)\n\t"
            "ld $2,0x10($31)\n\t"
            "ld $3,0x18($31)\n\t"
            "ld $4,0x20($31)\n\t"
            "ld $5,0x28($31)\n\t"
            "ld $6,0x30($31)\n\t"
            "ld $7,0x38($31)\n\t"
            "ld $8,0x40($31)\n\t"
            "ld $9,0x48($31)\n\t"
            "ld $10,0x50($31)\n\t"
            "ld $11,0x58($31)\n\t"
            "ld $12,0x60($31)\n\t"
            "ld $13,0x68($31)\n\t"
            "ld $14,0x70($31)\n\t"
            "ld $15,0x78($31)\n\t"
            "ld $16,0x80($31)\n\t"
            "ld $17,0x88($31)\n\t"
            "ld $18,0x90($31)\n\t"
            "ld $19,0x98($31)\n\t"
            "ld $20,0xA0($31)\n\t"
            "ld $21,0xA8($31)\n\t"
            "ld $22,0xB0($31)\n\t"
            "ld $23,0xB8($31)\n\t"
            "ld $24,0xC0($31)\n\t"
            "ld $25,0xC8($31)\n\t"
            "ld $26,0xD0($31)\n\t"
            "ld $27,0xD8($31)\n\t"
            "ld $28,0xE0($31)\n\t"
            "ld $29,0xE8($31)\n\t"
            "ld $30,0xF0($31)\n\t"
            "ld $31,0xF8($31)\n\t"
            "jr $11\n\t"
            "nop\n"
            "9:\n"
            ::: "$8" );
}
