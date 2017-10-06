//
// Copyright (c) 2017 The Altra64 project contributors
// Portions (c) 2013 saturnu (Alt64) based on libdragon, Neo64Menu, ED64IO, libn64-hkz, libmikmod
// See LICENSE file in the project root for full license information.
//

// rom.h
// rom tools - header inspect
#include <stdint.h>
#include <libdragon.h>
#include "rom.h"






#if !defined(MIN)
    #define MIN(a, b) ({ \
        __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a < _b ? _a : _b; \
    })
#endif

#if !defined(MAX)
    #define MAX(a, b) ({ \
        __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a > _b ? _a : _b; \
    })
#endif

sprite_t *loadImage32DFS(char *fname);
sprite_t *loadImageDFS(char *fname);
sprite_t *loadImage32(u8 *tbuf, int size);
void drawImage(display_context_t dcon, sprite_t *sprite);
void _sync_bus(void);
void _data_cache_invalidate_all(void);
void printText(char *msg, int x, int y, display_context_t dcon);

// End ...


void restoreTiming(void);

void simulate_boot(u32 boot_cic, u8 bios_cic, u32 *cheat_list[2]);


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
