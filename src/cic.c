//
// si/cic.c: PIF CIC security/lock out algorithms.
//
// CEN64: Cycle-Accurate Nintendo 64 Emulator.
// Copyright (C) 2015, Tyler J. Stachecki.
//
// This file is subject to the terms and conditions defined in
// 'LICENSE', which is part of this source code package.
//

//#include "common.h"
//#include "si/cic.h"
#include <stdint.h>
#include <stddef.h>
#include "cic.h"


// CIC seeds and status bits passed from PIF to IPL through PIF RAM
// Bits     | Description
// 00080000 | ROM type (0 = Game Pack, 1 = DD)
// 00040000 | Version
// 00020000 | Reset Type (0 = cold reset, 1 = NMI)
// 0000FF00 | CIC IPL3 seed value
// 000000FF | CIC IPL2 seed value
// #define CIC_SEED_NUS_5101 0x0000AC00U
// #define CIC_SEED_NUS_6101 0x00043F3FU
// #define CIC_SEED_NUS_6102 0x00003F3FU
// #define CIC_SEED_NUS_6103 0x0000783FU
// #define CIC_SEED_NUS_6105 0x0000913FU
// #define CIC_SEED_NUS_6106 0x0000853FU
// #define CIC_SEED_NUS_8303 0x0000DD00U

#define CRC_NUS_5101 0x587BD543U
#define CRC_NUS_6101 0x6170A4A1U
#define CRC_NUS_7102 0x009E9EA3U
#define CRC_NUS_6102 0x90BB6CB5U
#define CRC_NUS_6103 0x0B050EE0U
#define CRC_NUS_6105 0x98BC2C86U
#define CRC_NUS_6106 0xACC8580AU
#define CRC_NUS_8303 0x0E018159U

static uint32_t si_crc32(const uint8_t *data, size_t size);

// Determines the CIC seed for a cart, given the ROM data.
//int get_cic_seed(const uint8_t *rom_data, uint32_t *cic_seed) {
int get_cic(unsigned char *rom_data) {
    uint32_t crc = si_crc32(rom_data + 0x40, 0x1000 - 0x40);
    uint32_t aleck64crc = si_crc32(rom_data + 0x40, 0xC00 - 0x40);

    if (aleck64crc == CRC_NUS_5101) 
        return 4;//*cic_seed = CIC_SEED_NUS_5101;
    else 
    {
        switch (crc) {
        case CRC_NUS_6101:
        case CRC_NUS_7102:
        //*cic_seed = CIC_SEED_NUS_6101;
        return 1;
        break;

        case CRC_NUS_6102:
        //*cic_seed = CIC_SEED_NUS_6102;
        return 2;
        break;

        case CRC_NUS_6103:
        //*cic_seed = CIC_SEED_NUS_6103;
        return 3;
        break;

        case CRC_NUS_6105:
        //*cic_seed = CIC_SEED_NUS_6105;
        return 5;
        break;

        case CRC_NUS_6106:
        //*cic_seed = CIC_SEED_NUS_6106;
        return 6;
        break;

        //case CRC_NUS_8303: //not sure if this is necessary as we are using cart conversions
        //*cic_seed = CIC_SEED_NUS_8303;
        //return 7;
        //break;

        default:
        break;
        }
    }
    return 2;
}

uint32_t si_crc32(const uint8_t *data, size_t size) {
  uint32_t table[256];
  unsigned n, k;
  uint32_t c;

  for (n = 0; n < 256; n++) {
    c = (uint32_t) n;

    for (k = 0; k < 8; k++) {
      if (c & 1)
        c = 0xEDB88320L ^ (c >> 1);
      else
        c = c >> 1;
    }

    table[n] = c;
  }

  c = 0L ^ 0xFFFFFFFF;

  for (n = 0; n < size; n++)
    c = table[(c ^ data[n]) & 0xFF] ^ (c >> 8);

  return c ^ 0xFFFFFFFF;
}
