//
// Copyright (c) 2017 The Altra64 project contributors
// Portions (c) 2011 KRIK
// See LICENSE file in the project root for full license information.
//

#ifndef _ERRORS_H
#define	_ERRORS_H


#define EVD_ERROR_FIFO_TIMEOUT 90; 
#define EVD_ERROR_MMC_TIMEOUT 91;

#define BOOT_UPD_ERR_WRONG_SIZE 95
#define BOOT_UPD_ERR_HDR 96
#define BOOT_UPD_ERR_CMP 97
#define BOOT_UPD_ERR_CIC_DTCT 98

#define FAT_ERR_NOT_EXIST 100
#define FAT_ERR_EXIST 101
#define FAT_ERR_NAME 102
#define FAT_ERR_OUT_OF_FILE 103
#define FAT_ERR_BAD_BASE_CLUSTER 104;
#define FAT_ERR_NO_FRE_SPACE 105
#define FAT_ERR_NOT_FILE 106
#define FAT_ERR_FILE_MODE 107
#define FAT_ERR_ROT_OVERFLOW 108
#define FAT_ERR_OUT_OF_TABLE 109
#define FAT_ERR_INIT 110
#define FAT_LFN_BUFF_OVERFLOW 111
#define FAT_DISK_NOT_READY 112
#define FAT_ERR_SIZE 113
#define FAT_ERR_RESIZE 114

#define ERR_FILE8_TOO_BIG 140
#define ERR_FILE16_TOO_BIG 141
#define ERR_WRON_OS_SIZE 142
#define ERR_OS_VERIFY 143
#define ERR_OS_VERIFY2 144
#define ERR_EMU_NOT_FOUND 145
#define ERR_SAVE_FORMAT 146
#define ERR_EEPROM 147
#define ERR_NO_FAV_SPACE 150



#define DISK_ERR_INIT 50

#define DISK_ERR_RD1 62
#define DISK_ERR_RD2 63

#define DISK_ERR_WR1 64
#define DISK_ERR_WR2 65
#define DISK_ERR_WR3 66
#define DISK_ERR_WRX 67
#define DISK_WR_SB_TOUT 68
#define DISK_ERR_WR_CRC 69

#define DISK_RD_FE_TOUT 70
#define DISK_ERR_CLOSE_RW1 71
#define DISK_ERR_CLOSE_RW2 72

#define SD_CMD_TIMEOUT 75
#define SD_CMD_CRC_ERROR 76

#define SD_INIT_ERROR 80


#endif	/* _ERRORS_H */
