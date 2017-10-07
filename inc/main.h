//
// Copyright (c) 2017 The Altra64 project contributors
// Portions (c) 2013 saturnu (Alt64) based on libdragon, Neo64Menu, ED64IO, libn64-hkz, libmikmod
// See LICENSE file in the project root for full license information.
//

//protos maybe some aren't necessary any longer
void PI_DMAWait(void);
void evd_writeReg(u8 reg, u16 val);
void bootRom(display_context_t disp, int silent);
void loadrom(display_context_t disp, u8 *buff, int fast);
void loadgbrom(display_context_t disp, u8 *buff);
void view_mpk_file(display_context_t disp, char *mpk_filename);
char TranslateNotes( char *bNote, char *Text );
inline char CountBlocks( char *bMemPakBinary, char *aNoteSizes );
void view_mpk(display_context_t disp);
void evd_ulockRegs(void);
u8 fatReadCluster(void *dst);
u8 fatGetNextCluster(u32 *cluster);
u8 fatSkipCluster();
void memRomWrite32(u32 addr, u32 val);
u32 memRomRead32(u32 addr);
uint32_t translate_color(char *hexstring);
void evd_init(void);
void memSpiSetDma(u8 mode);
u16 evd_getFirmVersion(void);
u8 evd_isSDMode(void);
void dma_write_s(void * ram_address, unsigned long pi_address, unsigned long len);
void dma_read_s(void * ram_address, unsigned long pi_address, unsigned long len);
void readSDcard(display_context_t disp, char *directory);
int readConfigFile(void);
//void readCheatFile(display_context_t disp);
int readCheatFile(char *filename, u32 *cheat_lists[2]);
static int configHandler(void* user, const char* section, const char* name, const char* value);
int saveTypeToSd(display_context_t disp, char* save_filename ,int tpye);
int saveTypeFromSd(display_context_t disp, char* rom_name, int stype);
int backupSaveData(display_context_t disp);
void romInfoScreen(display_context_t disp, u8 *buff, int silent);
void checksum_sdram(void);
void drawShortInfoBox(display_context_t disp, char* text, u8 mode);
void drawToplistSelection(display_context_t disp,int l);
//boxes
void drawBox(short x, short y, short width, short height, display_context_t disp);
void drawBoxNumber(display_context_t disp, int box);

void printText(char *msg, int x, int y, display_context_t dcon);
void sleep(unsigned long int ms);
void clearScreen(display_context_t disp);

//mp3
void start_mp3(char *fname, long long *samples, int *rate, int *channels);

//character input functions
void drawTextInput(display_context_t disp,char *msg);
void drawInputAdd(display_context_t disp, char *msg);
void drawInputDel(display_context_t disp);

void readRomConfig(display_context_t disp, char* short_filename, char* full_filename);
void alterRomConfig(int type, int mode);
void drawConfigSelection(display_context_t disp,int l);
void drawRomConfigBox(display_context_t disp,int line);

#define MAX_LIST        20
#define REG_VER 		11
#define ED_CFG_SDRAM_ON 0

#define ishexchar(c) (((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'F')) || ((c >= 'a') && (c <= 'f')))


/**
 * @brief Return the uncached memory address of a cached address
 *
 * @param[in] x
 *            The cached address
 *uint32_t
 * @return The uncached address
 */
#define UNCACHED_ADDR(x)    ((void *)(((uint32_t)(x)) | 0xA0000000))

/**
 * @brief Align a memory address to 16 byte offset
 *
 * @param[in] x
 *            Unaligned memory address
 *
 * @return An aligned address guaranteed to be >= the unaligned address
 */
#define ALIGN_16BYTE(x)     ((void *)(((uint32_t)(x)+15) & 0xFFFFFFF0))
