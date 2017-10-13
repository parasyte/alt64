//
// Copyright (c) 2017 The Altra64 project contributors
// Portions (c) 2011 KRIK
// See LICENSE file in the project root for full license information.
//

#ifndef _FAT_H
#define	_FAT_H

#include "types.h"



#define FAT_MAX_DIR_SIZE 1024
#define FAT_MAX_NAME_LEN 128
#define FAT_MAX_NAME_LEN_S 256

#define FAT_TYPE_16 1
#define FAT_TYPE_32 2

typedef struct {
    u8 name[FAT_MAX_NAME_LEN];
    u32 entry_cluster;
    u32 size;
    u32 hdr_sector;
    u16 hdr_idx;
    u16 fav_idx;
    u8 is_dir;
} FatRecord;

typedef struct {
    u8 name[8];
    u8 ext[3];
    u8 attrib;
    u8 x[8];
    u16 cluster_hi;
    u16 time;
    u16 date;
    u16 cluster_lo;
    u32 size;
} FatRecordHdr;

typedef struct {
    FatRecord * rec[FAT_MAX_DIR_SIZE];
    FatRecord * s_records[FAT_MAX_DIR_SIZE];
    u8 *path;
    u16 size;
    u32 entry_cluster;
    u32 entry_sector;
    u32 in_cluster_ptr;
    u32 current_cluster;
    u8 is_root;
} Dir;

typedef struct {
    u32 pbr_entry;
    u32 root_entry;
    u32 fat_entry;
    u32 data_entry;
    u8 type;
    u8 cluster_size;
    u32 loaded_sector;
    u32 cluster_byte_size;
    u32 sectors_per_fat;
} Fat;

typedef struct {
    u8 *table_buff;
    u8 *table_sector;
    u8 *data_sector;
    u32 data_sec_idx;
    u32 table_sec_idx;
} FatCache;

typedef struct {
    u32 sector;
    u32 cluster;
    u32 sec_available;
    u16 in_cluster_ptr;
    FatRecord rec;
    u8 mode;
} File;



extern Dir *dir;
//extern Dir dir_o;
extern File file;
extern FatCache *fat_cache;
//extern FatRecord rec_tmp;
#define ED_ROOT "/ED64"
#define ED_SAVE "/ED64/SDSAVE"
#define ED_ROM_CFG "/ED64/SDSAVE/LAST.CRT"
//#define ED_TEST_CFG "/MISC/test.dat"
//#define ED_OPTIONS "/ED64/options.dat"
//#define ED_FAVOR "/ED64/favor.dat"

//#define ED_SAVE_DMP "/EDMD/sav-dmp.bin"
#define FILE_MODE_RD 1
#define FILE_MODE_WR 2

u8 fatInit();
u8 fatLoadDir(u32 cluster);
u8 fatLoadDirByName(u8 *name);

u8 fatWriteFile(void *src, u32 sectors);
u8 fatReadFile(void *dst, u32 sectors);
u8 fatReadPartialFile(void *dst, u32 sectors, int b_offset);
u8 fatCreateRecord(u8 *name, u8 is_dir, u8 check_exist);
u8 fatOpenFileByName(u8 *name, u32 wr_sectors);
u8 fatFindRecord(u8 *name, FatRecord *rec, u8 is_dir);
u8 fatOpenFile(FatRecord *rec, u32 wr_sectors);

u8 fatSkipSectors(u32 sectors);
u8 fatCreateRecIfNotExist(u8 *name, u8 is_dir);
u8 fatGetFullName(u8 *name, u32 dir_entry, u32 rec_entry);
u8 fatOpenDirByName(u8 *name);
void fatInitRam();
void fatSortRecords();


#endif	/* _FAT_H */
