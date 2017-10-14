//
// Copyright (c) 2017 The Altra64 project contributors
// Portions (c) 2013 saturnu (Alt64) based on libdragon, Neo64Menu, ED64IO, libn64-hkz, libmikmod
// See LICENSE file in the project root for full license information.
//

#include "types.h"
#include "integer.h"
#include "sd.h"
#include "mem.h"
#include "everdrive.h"
#include "errors.h"
#include "sys.h"

#define CMD0  (0x40+0)      // GO_IDLE_STATE, Software reset.
#define CMD1  (0x40+1)      // SEND_OP_COND, Initiate initialization process.
#define CMD2  (0x40+2)      //read cid
#define CMD3  (0x40+3)      //read rca
#define CMD6  (0x40+6)
#define CMD7  (0x40+7)
#define CMD8  (0x40+8)      // SEND_IF_COND, For only SDC V2. Check voltage range.
#define CMD9  (0x40+9)      // SEND_CSD, Read CSD register.
#define CMD10 (0x40+10)     // SEND_CID, Read CID register.
#define CMD12 (0x40+12)     // STOP_TRANSMISSION, Stop to read data.
#define ACMD13  (0xC0+13)   // SD_STATUS (SDC)
#define CMD16 (0x40+16)     // SET_BLOCKLEN, Change R/W block size.
#define CMD17 (0x40+17)     // READ_SINGLE_BLOCK, Read a block.
#define CMD18 (0x40+18)     // READ_MULTIPLE_BLOCK, Read multiple blocks.
#define ACMD23  (0xC0+23)   // SET_WR_BLK_ERASE_COUNT (SDC)
#define CMD24 (0x40+24)     // WRITE_BLOCK, Write a block.
#define CMD25 (0x40+25)     // WRITE_MULTIPLE_BLOCK, Write multiple blocks.
#define CMD41 (0x40+41)     // SEND_OP_COND (ACMD)
#define ACMD41  (0xC0+41)   // SEND_OP_COND (SDC)
#define CMD55 (0x40+55)     // APP_CMD, Leading command of ACMD<n> command.
#define CMD58 (0x40+58)     // READ_OCR, Read OCR.

#define SD_V2 2
#define SD_HC 1



#define R1 1
#define R2 2
#define R3 3
#define R6 6
#define R7 7

u8 card_type;
u8 sd_resp_buff[18];
u32 disk_interface;



unsigned int diskCrc7(unsigned char *buff, unsigned int len);
void diskCrc16SD(const u8 *data, u16 *crc_out, u16 len);
u8 diskGetRespTypeSD(u8 cmd);
u8 diskCmdSD(u8 cmd, u32 arg);

u8 diskInitSD();
u8 diskReadSD(u32 sector, void *buff, u16 count);
u8 diskWriteSD(u32 sector, const u8 *buff, u16 count);
u8 diskStopRwSD();

u8 diskCmdSPI(u8 cmd, u32 arg);
u8 diskInitSPI();
u8 diskReadSPI(u32 sector, void *buff, u16 count);
u8 diskWriteSPI(u32 sector, const u8 *buff, u16 count);



const u16 sd_crc16_table[] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

void sdSetInterface(u32 interface) {

    disk_interface = interface;
}

u8 sdGetInterface() {

    return disk_interface;
}

u8 sdInit() {

    if (disk_interface == DISK_IFACE_SD) {
        return diskInitSD();
    } else {
        return diskInitSPI();
    }
}

u8 sdRead(u32 sector, u8 *buff, u16 count) {

    if (disk_interface == DISK_IFACE_SD) {
        return diskReadSD(sector, buff, count);
    } else {
        return diskReadSPI(sector, buff, count);
    }
}

u8 sdWrite(u32 sector, const u8 *buff, u16 count) {

    if (disk_interface == DISK_IFACE_SD) {
        return diskWriteSD(sector, buff, count);
    } else {
        return diskWriteSPI(sector, buff, count);
    }
}

void diskCrc16SD(const u8 *data, u16 *crc_out, u16 len) {

    ///u16 len = 512;
    u16 i, tmp1, u;
    u8 dat[4];
    u8 dat_tmp;
    u16 crc_buff[4];


    for (i = 0; i < 4; i++)crc_buff[i] = 0;
    for (i = 0; i < 4; i++)crc_out[i] = 0;
    //u16 dptr = 0;
    while (len) {
        len -= 4;
        for (i = 0; i < 4; i++)dat[i] = 0;

        for (u = 0; u < 4; u++) {
            dat_tmp = data[3 - u];
            //dat_tmp = 0xff;
            for (i = 0; i < 4; i++) {
                dat[i] >>= 1;
                dat[i] |= (dat_tmp & 1) << 7;
                dat_tmp >>= 1;
            }

            for (i = 0; i < 4; i++) {
                dat[i] >>= 1;
                dat[i] |= (dat_tmp & 1) << 7;
                dat_tmp >>= 1;
            }
        }

        data += 4;

        for (u = 0; u < 4; u++) {
            tmp1 = crc_buff[u];
            crc_buff[u] = sd_crc16_table[(tmp1 >> 8) ^ dat[u]];
            crc_buff[u] = crc_buff[u] ^ (tmp1 << 8);
        }

    }

    for (i = 0; i < 4 * 16; i++) {

        crc_out[3 - i / 16] >>= 1;
        crc_out[3 - i / 16] |= (crc_buff[i % 4] & 1) << 15;
        crc_buff[i % 4] >>= 1;
    }

}

unsigned int diskCrc7(unsigned char *buff, unsigned int len) {

    unsigned int a, crc = 0;

    while (len--) {
        crc ^= *buff++;
        a = 8;
        do {
            crc <<= 1;
            if (crc & (1 << 8)) crc ^= 0x12;
        } while (--a);
    }
    return (crc & 0xfe);
}

u8 diskGetRespTypeSD(u8 cmd) {

    switch (cmd) {
        case CMD3:
            return R6;
        case CMD8:
            return R7;
        case CMD2:
        case CMD9:
            return R2;
        case CMD58:
        case CMD41:
            return R3;

        default: return R1;
    }

}

u8 diskCmdSD(u8 cmd, u32 arg) {

    u8 resp_type = diskGetRespTypeSD(cmd);
    u8 p = 0;
    u8 buff[6];
    volatile u8 resp;
    volatile u32 i = 0;

    u8 resp_len = resp_type == R2 ? 17 : 6;
    u8 crc;
    buff[p++] = cmd;
    buff[p++] = (arg >> 24);
    buff[p++] = (arg >> 16);
    buff[p++] = (arg >> 8);
    buff[p++] = (arg >> 0);
    crc = diskCrc7(buff, 5) | 1;

    evd_SDcmdWriteMode(0);

    mem_spi(0xff);
    mem_spi(cmd);
    mem_spi(arg >> 24);
    mem_spi(arg >> 16);
    mem_spi(arg >> 8);
    mem_spi(arg);
    mem_spi(crc);
    evd_SDcmdReadMode(1);

    i = 0;
    resp = 0xff;
    while ((resp & 192) != 0) {
        resp = mem_spi(resp);
        if (i++ == WAIT)return SD_CMD_TIMEOUT;
    }
    evd_SDcmdReadMode(0);


    sd_resp_buff[0] = resp;

    for (i = 1; i < resp_len; i++) {
        sd_resp_buff[i] = mem_spi(0xff);
    }

    if (resp_type != R3) {



        if (resp_type == R2) {
            crc = diskCrc7(sd_resp_buff + 1, resp_len - 2) | 1;
        } else {
            crc = diskCrc7(sd_resp_buff, resp_len - 1) | 1;
        }
        if (crc != sd_resp_buff[resp_len - 1])return SD_CMD_CRC_ERROR;
    }

    return 0;
}

u8 diskInitSD() {

    u16 i;
    volatile u8 resp = 0;
    u32 rca;
    u32 wait_len = WAIT;

    card_type = 0;
    evd_enableSDMode();
    memSpiSSOff();
    memSpiSetSpeed(SPI_SPEED_INIT);

    evd_SDcmdWriteMode(0);

    for (i = 0; i < 40; i++)mem_spi(0xff);
    resp = diskCmdSD(CMD0, 0x1aa);
    for (i = 0; i < 40; i++)mem_spi(0xff);


    resp = diskCmdSD(CMD8, 0x1aa);
    if (resp != 0 && resp != SD_CMD_TIMEOUT) {
        return SD_INIT_ERROR + 0;
    }
    if (resp == 0)card_type |= SD_V2;


    if (card_type == SD_V2) {

        for (i = 0; i < wait_len; i++) {

            resp = diskCmdSD(CMD55, 0);
            if (resp)return SD_INIT_ERROR + 1;
            if ((sd_resp_buff[3] & 1) != 1)continue;
            resp = diskCmdSD(CMD41, 0x40300000);
            if ((sd_resp_buff[1] & 128) == 0)continue;

            break;
        }
    } else {

        i = 0;
        do {
            resp = diskCmdSD(CMD55, 0);
            if (resp)return SD_INIT_ERROR + 2;
            resp = diskCmdSD(CMD41, 0x40300000);
            if (resp)return SD_INIT_ERROR + 3;

        } while (sd_resp_buff[1] < 1 && i++ < wait_len);

    }

    if (i == wait_len)return SD_INIT_ERROR + 4;

    if ((sd_resp_buff[1] & 64) && card_type != 0)card_type |= SD_HC;


    resp = diskCmdSD(CMD2, 0);
    if (resp)return SD_INIT_ERROR + 5;

    resp = diskCmdSD(CMD3, 0);
    if (resp)return SD_INIT_ERROR + 6;

    resp = diskCmdSD(CMD7, 0);
    //if (resp)return resp;

    rca = (sd_resp_buff[1] << 24) | (sd_resp_buff[2] << 16) | (sd_resp_buff[3] << 8) | (sd_resp_buff[4] << 0);




    resp = diskCmdSD(CMD9, rca); //get csd
    if (resp)return SD_INIT_ERROR + 7;


    resp = diskCmdSD(CMD7, rca);
    if (resp)return SD_INIT_ERROR + 8;


    resp = diskCmdSD(CMD55, rca);
    if (resp)return SD_INIT_ERROR + 9;


    resp = diskCmdSD(CMD6, 2);
    if (resp)return SD_INIT_ERROR + 10;


    memSpiSetSpeed(SPI_SPEED_25);

    return 0;

}

u8 diskReadSD(u32 sector, void *buff, u16 count) {

    u8 resp;


    if (!(card_type & 1))sector *= 512;
    resp = diskCmdSD(CMD18, sector);
    if (resp)return DISK_ERR_RD1;


    resp = memSpiRead(buff, count);
    if (resp)return resp;

    //console_printf("drd: %0X\n", sector);
    resp = diskStopRwSD();

    return resp;
}

u8 diskStopRwSD() {

    u8 resp;
    u16 i;
    resp = diskCmdSD(CMD12, 0);
    if (resp)return DISK_ERR_CLOSE_RW1;
    evd_SDdatReadMode(0);
    mem_spi(0xff);

    i = 65535;
    while (i--) {
        if (mem_spi(0xff) == 0xff)break;
        if (mem_spi(0xff) == 0xff)break;
        if (mem_spi(0xff) == 0xff)break;
        if (mem_spi(0xff) == 0xff)break;
    }
    if (i == 0)return DISK_ERR_CLOSE_RW2;

    return 0;
}

u8 diskWriteSD(u32 sector, const u8 *buff, u16 count) {

    u8 resp;
    u16 crc16[5];
    u16 i;
    u16 u;
    u8 ram_buff[512];
    const u8 *buff_ptr;


    if (!(card_type & 1))sector *= 512;
    resp = diskCmdSD(CMD25, sector);
    if (resp)return DISK_ERR_WR1;

    evd_SDdatWriteMode(0);


    while (count--) {

        if ((u32) buff >= ROM_ADDR && (u32) buff < ROM_END_ADDR) {
            dma_read_s(ram_buff, (u32) buff, 512);
            buff_ptr = ram_buff;
        } else {
            buff_ptr = buff;
        }

        diskCrc16SD(buff_ptr, crc16, 512);
        evd_SDdatWriteMode(0);


        mem_spi(0xff);
        mem_spi(0xf0);


        memSpiWrite(buff_ptr);

        for (i = 0; i < 4; i++) {
            mem_spi(crc16[i] >> 8);
            mem_spi(crc16[i] & 0xff);
        }

        buff += 512;

        evd_SDdatWriteMode(1);
        mem_spi(0xff);
        evd_SDdatReadMode(1);

        i = 1024;
        while ((mem_spi(0xff) & 1) != 0 && i-- != 0);
        if (i == 0)return DISK_WR_SB_TOUT;
        resp = 0;

        for (i = 0; i < 3; i++) {
            resp <<= 1;
            u = mem_spi(0xff);
            resp |= u & 1;
        }
        resp &= 7;

        if (resp != 0x02) {
            //console_printf("error blia: %0X\n", resp);
            // joyWait();
            if (resp == 5)return DISK_ERR_WR_CRC;
            return DISK_ERR_WRX;
        }

        evd_SDdatReadMode(0);
        mem_spi(0xff);
        i = 65535;
        while (i--) {

            if (mem_spi(0xff) == 0xff)break;
            if (mem_spi(0xff) == 0xff)break;
        }
        if (i == 0)return DISK_ERR_WR2;
    }

    resp = diskStopRwSD();
    if (resp)return resp;

    return 0;
}

u8 diskCmdSPI(u8 cmd, u32 arg) {


    u8 crc;
    u8 p = 0;
    u8 buff[6];
    buff[p++] = cmd;
    buff[p++] = (arg >> 24);
    buff[p++] = (arg >> 16);
    buff[p++] = (arg >> 8);
    buff[p++] = (arg >> 0);

    crc = diskCrc7(buff, 5) | 1;

    volatile u32 i = 0;
    volatile u8 resp;
    memSpiBusy();
    memSpiSSOff();
    memSpiSSOn();
    //mem_spi(0x1);
    mem_spi(0xff);
    mem_spi(cmd);
    mem_spi(arg >> 24);
    mem_spi(arg >> 16);
    mem_spi(arg >> 8);
    mem_spi(arg);
    mem_spi(crc);
    mem_spi(0xff);
    resp = mem_spi(0xff);

    // memSpiSSOn();

    while (resp == 0xff) {
        resp = mem_spi(0xff);
        if (i++ == WAIT)break;
    }

    memSpiSSOff();
    return resp;
}

u8 diskInitSPI() {

    u16 i;
    u32 u;

    volatile u8 resp = 0;
    u8 cmd;
    u32 wait_len = WAIT;

    card_type = 0;

    evd_enableSPIMode();
    //return evd_mmcInit();

    memSpiSetSpeed(SPI_SPEED_INIT);

    for (u = 0; u < 4; u++) {
        for (i = 0; i < 40; i++)mem_spi(0xff);
        resp = diskCmdSPI(CMD0, 0);
        if (resp == 1)break;
    }

    if (resp != 1) return DISK_ERR_INIT + 0;
    //mmcCmdCrc(0x7b, 0, 0x91);
    resp = diskCmdSPI(CMD8, 0x1aa);
    for (i = 0; i < 5; i++)mem_spi(0xff);
    if (resp == 0xff)return DISK_ERR_INIT + 1;
    if (resp != 5)card_type |= SD_V2;

    if (card_type == 2) {

        for (i = 0; i < wait_len; i++) {


            resp = diskCmdSPI(CMD55, 0xffff);
            if (resp == 0xff)return DISK_ERR_INIT + 2;
            if (resp != 1)continue;

            resp = diskCmdSPI(CMD41, 0x40300000);
            if (resp == 0xff)return DISK_ERR_INIT + 3;
            if (resp != 0)continue;

            break;
        }
        if (i == wait_len)return DISK_ERR_INIT + 4;

        resp = diskCmdSPI(CMD58, 0);
        if (resp == 0xff)return DISK_ERR_INIT + 5;
        memSpiSSOn();
        resp = mem_spi(0xff);
        for (i = 0; i < 3; i++)mem_spi(0xff);
        if ((resp & 0x40))card_type |= 1;
    } else {


        i = 0;

        resp = diskCmdSPI(CMD55, 0);
        if (resp == 0xff)return DISK_ERR_INIT + 6;
        resp = diskCmdSPI(CMD41, 0);
        if (resp == 0xff)return DISK_ERR_INIT + 7;
        cmd = resp;

        for (i = 0; i < wait_len; i++) {
            if (resp < 1) {

                resp = diskCmdSPI(CMD55, 0);
                if (resp == 0xff)return DISK_ERR_INIT + 8;
                if (resp != 1)continue;

                resp = diskCmdSPI(CMD41, 0);
                if (resp == 0xff)return DISK_ERR_INIT + 9;
                if (resp != 0)continue;

            } else {

                resp = diskCmdSPI(CMD1, 0);
                if (resp != 0)continue;
            }

            break;

        }

        if (i == wait_len)return DISK_ERR_INIT + 10;

    }

    memSpiSetSpeed(SPI_SPEED_25);

    return 0;
}

u8 diskReadSPI(u32 sector, void *buff, u16 count) {


    u8 resp;

    if (!(card_type & 1))sector *= 512;
    resp = diskCmdSPI(CMD18, sector);
    if (resp != 0)return DISK_ERR_RD1;
    memSpiSSOn();

    resp = memSpiRead(buff, count);

    memSpiSSOff();
    diskCmdSPI(CMD12, 0);

    return resp;
}

u8 diskWriteSPI(u32 sector, const u8 *buff, u16 count) {

    u8 resp;
    u16 i;

    if (!(card_type & 1))sector *= 512;
    resp = diskCmdSPI(CMD25, sector);
    if (resp != 0)return DISK_ERR_WR1;
    memSpiSSOn();


    while (count--) {

        mem_spi(0xff);
        mem_spi(0xff);
        mem_spi(0xfc);


        memSpiWrite(buff);

        mem_spi(0xff);
        mem_spi(0xff);
        resp = mem_spi(0xff);
        //resp = mem_spi(0xff);

        buff += 512;



        if ((resp & 0x1f) != 0x05) {
            memSpiSSOff();
            return DISK_ERR_WRX;
        }


        for (i = 0; i < 65535; i++) {

            if (mem_spi(0xff) == 0xff)break;
            if (mem_spi(0xff) == 0xff)break;
            if (mem_spi(0xff) == 0xff)break;
            if (mem_spi(0xff) == 0xff)break;
        }


        if (i == 65535) {
            memSpiSSOff();
            return DISK_ERR_WR2;
        }

    }

    mem_spi(0xfd);
    mem_spi(0xff);

    for (i = 0; i < 65535; i++) {
        if ((mem_spi(0xff) & 1) != 0)break;
        if ((mem_spi(0xff) & 1) != 0)break;
        if ((mem_spi(0xff) & 1) != 0)break;
        if ((mem_spi(0xff) & 1) != 0)break;
    }
    memSpiSSOff();
    if (i == 65535) return DISK_ERR_WR3;

    return 0;
}
