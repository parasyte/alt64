//
// Copyright (c) 2017 The Altra64 project contributors
// Portions (c) 2013 saturnu (Alt64) based on libdragon, Neo64Menu, ED64IO, libn64-hkz, libmikmod
// See LICENSE file in the project root for full license information.
//

#include <malloc.h>
#include <libdragon.h>

#include "fat.h"
#include "disk.h"
#include "mem.h"
#include "everdrive.h"
#include "strlib.h"
#include "errors.h"









//FatRecordHdr rec_hdr;




u8 fatLoadLfnPart(u8 *name_ptr, u8 *lfn_buff);
u8 fatCacheLoadTable(u32 sector, u8 save_before_load);
u8 fatCacheLoadData(u32 sector, u8 save_befor_load);

u8 fatGetNextCluster(u32 *cluster);
u8 fatGetTableRecord(u32 cluster, u32 *val);
u8 fatSetTableRecord(u32 cluster, u32 next_cluster);
u8 fatCacheApplyTable();
u8 fatGetFreeRecord(FatRecord *dir_rec, u8 len);
void fatMakeLfnBlock(u8 *name, u8 *lfn_block, u8 block_idx, u8 crc);
u32 fatSectorToCluster(u32 sector);
u8 fatClearClusters(u32 cluster, u8 len);

u8 fatReadCluster(void *dst);
u8 fatOccupyClusters(u32 *base_cluster, u16 len);
void fatOpenDir(u32 cluster);
u8 fatReadDir(FatRecord *rec, u16 max_name_len);
u32 fatClusterToSector(u32 cluster);
u8 fatInitRoot();
u8 fatSkipCluster();
//u8 fatReadMultiClustes(void *dst, u32 max_sectors, u32 *readrd_sectors);
u8 fatMultiClustRW(void *buff, u32 max_sectors, u32 *readrd_sectors, u16 write);
u8 fatResizeFile(FatRecord *rec, u32 req_sec_size);

Dir *dir;
Fat current_fat;
File file;
FatCache *fat_cache;
u16 kot;
u32 *occupy_buff;
vu16 table_changed;

//FatRecord *rec_tmp;
//#define CACHE_SIZE 16

void fatInitRam() {

    u16 i;
    //rec_tmp = malloc()
    dir = malloc(sizeof (Dir));

    for (i = 0; i < FAT_MAX_DIR_SIZE; i++) {
        dir->rec[i] = malloc(sizeof (FatRecord));
    }

    dir->path = malloc(1024);

    fat_cache = malloc(sizeof (FatCache));

    fat_cache->data_sector = malloc(512);
    fat_cache->table_sector = malloc(512);

    occupy_buff = malloc(16384);

    // current_cache->table_frame = current_cache->table_buff;

    //cache = &ch;
}


u8 fatInitRoot() {

   // u8 resp = 0;
    
  //  resp = fatCreateRecIfNotExist(ED_ROOT, 1);
  //  if (resp)return resp;
    
//	resp = fatCreateRecIfNotExist(ED_SAVE, 1);
 //   if (resp)return resp;
    
  //  resp = fatCreateRecIfNotExist(ED_ROM_CFG, 0);
   // if (resp)return resp;
    
    return 0;
}

/*
u8 fatInitRoot() {

    u8 resp = 0;

    resp = fatCreateRecIfNotExist(ED_ROOT, 1);
    if (resp)return resp;
    resp = fatCreateRecIfNotExist(ED_SAVE, 1);
    if (resp)return resp;
    resp = fatCreateRecIfNotExist(ED_ROM_CFG, 0);
    if (resp)return resp;
    resp = fatCreateRecIfNotExist(ED_OPTIONS, 0);
    if (resp)return resp;
 
    return 0;
}*/

u8 fatGetFullName(u8 *name, u32 dir_entry, u32 rec_entry) {

    u16 i;
    u8 resp;
    FatRecord rec_tmp;
    fatOpenDir(dir_entry);
    for (;;) {

        resp = fatReadDir(&rec_tmp, FAT_MAX_NAME_LEN_S);
        if (resp)return resp;
        if (rec_tmp.name[0] == 0)return FAT_ERR_NOT_EXIST;
        if (rec_entry == rec_tmp.entry_cluster) {
            for (i = 0; i < FAT_MAX_NAME_LEN_S; i++) {
                *name++ = rec_tmp.name[i];
                if (rec_tmp.name[i] == 0)return 0;
            }
        }

    }

    return 0;
}

u8 fatCreateRecIfNotExist(u8 *name, u8 is_dir) {

    u8 resp;
    FatRecord rec_tmp;
    resp = fatFindRecord(name, &rec_tmp, is_dir);
    if (resp == FAT_ERR_NOT_EXIST) {
        resp = fatCreateRecord(name, is_dir, 0);
        if (resp)return resp;
    } else
        if (resp)return resp;

    return 0;
}

u8 fatInit() {


    u8 resp;

    u32 reserved_sectors;
    resp = diskInit();
    if (resp)return resp;
    fat_cache->data_sec_idx = 0xffffffff;
    fat_cache->table_sec_idx = 0xffffffff;
    current_fat.pbr_entry = 0;
    table_changed = 0;
    //rec_tmp.name = rec_tmp_name;

    //for (i = 0; i < FAT_MAX_DIR_SIZE; i++)dir.rec[i].name = (u8 *) & rec_names[i * (FAT_MAX_NAME_LEN + 4) / 4];
    //for (i = 0; i < sizeof (rec_names); i++)((u8 *) rec_names)[i] = 0;
    resp = fatCacheLoadData(current_fat.pbr_entry, 0);
    if (resp)return resp;
    fatCacheLoadData(0, 0);


    if (!streql("FAT16", &fat_cache->data_sector[0x36], 5) && !streql("FAT32", &fat_cache->data_sector[0x52], 5)) {

        current_fat.pbr_entry = (u32) fat_cache->data_sector[0x1c6] | (u32) fat_cache->data_sector[0x1c7] << 8 | (u32) fat_cache->data_sector[0x1c8] << 16 | (u32) fat_cache->data_sector[0x1c9] << 24;
        resp = fatCacheLoadData(current_fat.pbr_entry, 0);
        //console_printf("dat: %s\n", &cache->buff[0x36]);
        if (resp)return resp;
        if (!streql("FAT16", &fat_cache->data_sector[0x36], 5) && !streql("FAT32", &fat_cache->data_sector[0x52], 5)) {
            return FAT_ERR_INIT;
        }
    }


    if (streql("FAT16", &fat_cache->data_sector[0x36], 5)) {
        current_fat.type = FAT_TYPE_16;
        current_fat.sectors_per_fat = (u32) fat_cache->data_sector[0x16] | (u32) fat_cache->data_sector[0x17] << 8;
    } else {
        current_fat.type = FAT_TYPE_32;
        current_fat.sectors_per_fat = (u32) fat_cache->data_sector[0x24] | (u32) fat_cache->data_sector[0x25] << 8 | (u32) fat_cache->data_sector[0x26] << 16 | (u32) fat_cache->data_sector[0x27] << 24;
    }
    reserved_sectors = (u32) fat_cache->data_sector[0x0e] | (u32) fat_cache->data_sector[0x0f] << 8;

    current_fat.cluster_size = fat_cache->data_sector[0x0d];
    current_fat.root_entry = current_fat.sectors_per_fat * 2 + current_fat.pbr_entry + reserved_sectors;
    current_fat.fat_entry = current_fat.pbr_entry + reserved_sectors;
    current_fat.data_entry = current_fat.root_entry;
    if (current_fat.type == FAT_TYPE_16) {
        current_fat.data_entry += 16384 / 512;
    }
    current_fat.cluster_byte_size = current_fat.cluster_size * 512;

    //fatCacheLoadBuff(fat.root_entry, 0);
    //VDP_drawText(APLAN, &cache->buff[0], 0, 2, 20);

    //resp = fatCacheLoadTable(0, 0);
    //if (resp)return resp;
    
    //debug
    resp = fatInitRoot();
    if (resp)return resp;

    return 0;
}

u8 fatOpenDirByName(u8 *name) {

    u8 resp;
    FatRecord rec_tmp;
    resp = fatFindRecord(name, &rec_tmp, 1);
    if (resp)return resp;
    fatOpenDir(rec_tmp.entry_cluster);
    return 0;
}

void fatOpenDir(u32 cluster) {


    if (cluster == 0) {
        dir->entry_sector = current_fat.root_entry;
        dir->is_root = 1;
        if (current_fat.type == FAT_TYPE_32)cluster = 2;
    } else {
        dir->entry_sector = fatClusterToSector(cluster);
        dir->is_root = 0;
    }

    dir->entry_cluster = cluster;
    dir->current_cluster = cluster;
    dir->in_cluster_ptr = 0;

}

u8 fatLoadDirByName(u8 *name) {

    u8 resp;

    if (streq(name, "/")) {
        fatOpenDir(0);
        resp = fatLoadDir(dir->entry_cluster);
        if (resp)return resp;
    } else {
        resp = fatOpenDirByName(name);
        if (resp)return resp;
        resp = fatLoadDir(dir->entry_cluster);
        if (resp)return resp;
    }



    return 0;
}

u8 fatLoadDir(u32 cluster) {

    u8 resp;

    fatOpenDir(cluster);

    for (dir->size = 0; dir->size < FAT_MAX_DIR_SIZE; dir->size++) {

        resp = fatReadDir(dir->rec[dir->size], FAT_MAX_NAME_LEN);
        if (resp)return resp;
        if (dir->rec[dir->size]->name[0] == 0)break;
    }

    fatSortRecords();
    return 0;
}

void fatSortRecords() {


    FatRecord *rec_buff;
    s32 i;
    u8 flag0;
    u8 flag1;
    u64 name0;
    u64 name1;
    u32 u;

    // VDP_drawText(APLAN, "1", 0, 2, cy++);
    for (i = 0; i < dir->size; i++) {
        dir->s_records[i] = dir->rec[i];
    }

    s32 stop, swap, limit;
    s32 x = (s32) (dir->size / 2) - 1;
    while (x > 0) {
        stop = 0;
        limit = dir->size - x;
        while (stop == 0) {
            swap = 0;
            for (i = 0; i < limit; i++) {

                name0 = 0;
                name1 = 0;
                for (u = 0; u < 8; u++) {
                    name0 <<= 8;
                    name1 <<= 8;
                    name0 |= dir->s_records[i]->name[u];
                    name1 |= dir->s_records[i + x]->name[u];
                }

                name0 |= 0x2020202020202020;
                name1 |= 0x2020202020202020;
                if (name0 > name1) {

                    rec_buff = dir->s_records[i];
                    dir->s_records[i] = dir->s_records[i + x];
                    dir->s_records[i + x] = rec_buff;
                    swap = i;

                }
            }
            limit = swap - x;
            if (swap == 0)
                stop = 1;
        }
        x = (int) (x / 2);
    }

    //VDP_drawText(APLAN, "2", 0, 2, cy++);
    /*
        for (i = 1; i < dir->size;) {

            name0 = 0;
            name1 = 0;
            for (u = 0; u < 8; u++) {
                name0 <<= 8;
                name1 <<= 8;
                name0 |= dir->s_records[i - 1]->name[u];
                name1 |= dir->s_records[i]->name[u];
            }
            // name0 = *((u64 *) dir->s_records[i - 1]->name);
            //VDP_drawText(APLAN, "--", 0, 2, cy++);
            //name1 = *((u64 *) dir->s_records[i]->name);

            if ((name1 | 0x2020202020202020) < (name0 | 0x2020202020202020)) {

                rec_buff = dir->s_records[i];
                dir->s_records[i] = dir->s_records[i - 1];
                dir->s_records[i - 1] = rec_buff;
                if (i > 1)i--;

            } else {
                i++;
            }
        }
     */

    // VDP_drawText(APLAN, "3", 0, 2, cy++);

    for (i = 1; i < dir->size;) {

        flag0 = dir->s_records[i - 1]->is_dir;
        flag1 = dir->s_records[i]->is_dir;

        if (flag1 && !flag0) {
            rec_buff = dir->s_records[i];
            dir->s_records[i] = dir->s_records[i - 1];
            dir->s_records[i - 1] = rec_buff;
            if (i > 1)i--;
        } else {
            i++;
        }
    }
    //VDP_drawText(APLAN, "4", 0, 2, cy++);

}

u8 fatReadDir(FatRecord *rec, u16 max_name_len) {

    u16 lfn_len = 0;
    u16 in_sector_ptr = 0;
    u32 current_sector = 0;
    u16 i = 0;
    u8 *ptr = 0;
    u8 resp;
    u32 base_sector; // = dir.entry_sector == dir.is_root ? fat.root_entry : fatClusterToSector(dir.current_cluster);
    u8 lfn_buff[FAT_MAX_NAME_LEN_S];

    //u8 attr;
    // u8 attrib;

    //cache->buff_sector = ~0;
    rec->is_dir = 0;
    rec->entry_cluster = 0;
    rec->size = 0;
    rec->name[0] = 0;

    if (current_fat.type == FAT_TYPE_16 && dir->is_root) {
        base_sector = current_fat.root_entry;
    } else {
        base_sector = fatClusterToSector(dir->current_cluster);
    }

    for (;;) {

        current_sector = base_sector + dir->in_cluster_ptr / 512;

        if (dir->is_root && current_fat.type == FAT_TYPE_16 && dir->in_cluster_ptr >= 16384) {

            return 0;
        }

        if (dir->in_cluster_ptr >= current_fat.cluster_byte_size && (!dir->is_root || current_fat.type == FAT_TYPE_32)) {

            //return 0;
            //if(dir->current_cluster == 0)dir->current_cluster = 2;
            resp = fatGetNextCluster(&dir->current_cluster);
            if (resp)return resp;
            if (dir->current_cluster == 0) return 0;
            base_sector = fatClusterToSector(dir->current_cluster);
            current_sector = base_sector; // + dir->in_cluster_ptr / 512;
            dir->in_cluster_ptr = 0;
        }

        in_sector_ptr = dir->in_cluster_ptr & 511;
        resp = fatCacheLoadData(current_sector, 0);
        if (resp)return resp;

        if (fat_cache->data_sector[in_sector_ptr] == 0) {

            rec->is_dir = 0;
            rec->entry_cluster = 0;
            rec->size = 0;
            rec->name[0] = 0;
            return 0;
            //dir->in_cluster_ptr += 32;
            // continue;
        }
        if (fat_cache->data_sector[in_sector_ptr] == 0xe5 || fat_cache->data_sector[in_sector_ptr] == 0x2e || fat_cache->data_sector[in_sector_ptr] == '.') {
            dir->in_cluster_ptr += 32;
            continue;
        }

        if (fat_cache->data_sector[in_sector_ptr + 0x0b] == 0x0f) {

            i = (fat_cache->data_sector[in_sector_ptr] & 0x0f);

            if (i * 13 >= sizeof (lfn_buff) || i == 0) {
                dir->in_cluster_ptr += 32; //i * 32; //cache.buff[in_sector_ptr] * 32;
                lfn_len = 0;
                //VDP_drawText(APLAN, "xxx", 0, 20, 8);
            } else {

                //while (i--) {

                resp = fatLoadLfnPart(&fat_cache->data_sector[in_sector_ptr], lfn_buff);
                if (resp)return resp;

                dir->in_cluster_ptr += 32;
                in_sector_ptr = dir->in_cluster_ptr & 511;
                current_sector = base_sector + dir->in_cluster_ptr / 512;
                resp = fatCacheLoadData(current_sector, 0);
                if (resp)return resp;
                lfn_len += 13;
                //}

            }

        } else {

            if (lfn_len == 0) {
                ptr = &fat_cache->data_sector[in_sector_ptr];
                for (i = 0; i < 8; i++)rec->name[i] = *ptr++;

                for (i = 7; rec->name[i] == ' '; i--) {
                    rec->name[i] = 0;
                    if (i == 0)break;
                }
                i++;

                rec->name[i] = 0;

                if (ptr[0] != ' ' || ptr[1] != ' ' || ptr[2] != ' ') {
                    rec->name[i++] = '.';
                    rec->name[i++] = *ptr++;
                    rec->name[i++] = *ptr++;
                    rec->name[i++] = *ptr++;
                    rec->name[i] = 0;
                }

            } else {

                if (lfn_buff[0] == '.') {
                    lfn_buff[0] = 0;
                    dir->in_cluster_ptr += 32; //i * 32; //cache.buff[in_sector_ptr] * 32;
                    lfn_len = 0;
                    continue;
                }

                ptr = lfn_buff;
                if (lfn_len > max_name_len)lfn_len = max_name_len;
                for (i = 0; i < lfn_len; i++)rec->name[i] = *ptr++;
                rec->name[i] = 0;
            }

            lfn_len = 0;
            ptr = &fat_cache->data_sector[in_sector_ptr];
            rec->is_dir = (ptr[0x0b] & 0x10) == 0x10 ? 1 : 0;
            rec->entry_cluster = (u32) ptr[0x1a] | (u32) ptr[0x1b] << 8 | (u32) ptr[0x14] << 16 | (u32) ptr[0x15] << 24;
            rec->size = (u32) ptr[0x1c] | (u32) ptr[0x1d] << 8 | (u32) ptr[0x1e] << 16 | (u32) ptr[0x1f] << 24;
            rec->hdr_idx = (dir->in_cluster_ptr & 511) / 32;
            rec->hdr_sector = current_sector;

            dir->in_cluster_ptr += 32;

            return 0;
            /*
                        } else {

                            dir->in_cluster_ptr += 32;
                        }*/
        }
    }


    return 1;
}

u8 fatLoadLfnPart(u8 *buff_ptr, u8 *lfn_buff) {

    u8 i;
    u8 offset = ((*buff_ptr & 0x0f) - 1) * 13;
    u8 *lfn_ptr = &lfn_buff[offset];

    if (offset + 13 >= FAT_MAX_NAME_LEN_S)return FAT_LFN_BUFF_OVERFLOW;

    buff_ptr++;
    for (i = 0; i < 5; i++) {
        *lfn_ptr++ = *buff_ptr++;
        buff_ptr++;
    }

    buff_ptr += 3;


    for (i = 0; i < 6; i++) {

        *lfn_ptr++ = *buff_ptr++;
        buff_ptr++;
    }

    buff_ptr += 2;

    *lfn_ptr++ = *buff_ptr++;
    buff_ptr++;
    *lfn_ptr++ = *buff_ptr++;

    return 0;
}

u8 fatCacheLoadData(u32 sector, u8 save_befor_load) {

    u8 resp;
    //if(cache->grobovik != 333)console_printf("b: %u\n", cache->grobovik);
    //console_printf("a: %u\n", cache->buff_sector);
    if (fat_cache->data_sec_idx == sector)return 0;

    if (save_befor_load) {
        resp = diskWrite(fat_cache->data_sec_idx, fat_cache->data_sector, 1);
    }
    fat_cache->data_sec_idx = sector;

					//addr=addr*512, buff, lenght 1
    resp = diskRead(fat_cache->data_sec_idx, fat_cache->data_sector, 1);

    //console_printf("c: %u\n", cache->buff_sector);
    //joyWait();

    return resp;

}

u8 fatCacheSaveData() {

    return diskWrite(fat_cache->data_sec_idx, fat_cache->data_sector, 1);
}

u8 fatCacheLoadTable(u32 sector, u8 save_before_load) {


    u8 resp;
    sector += current_fat.fat_entry;
    if (fat_cache->table_sec_idx == sector)return 0;
    if (save_before_load) {
        resp = fatCacheApplyTable();
        if (resp)return resp;
    }

    fat_cache->table_sec_idx = sector;

    resp = diskRead(fat_cache->table_sec_idx, fat_cache->table_sector, 1);

    return resp;

}

u8 fatCacheApplyTable() {

    u8 resp;

    resp = diskWrite(fat_cache->table_sec_idx, fat_cache->table_sector, 1);
    if (resp)return resp;
    resp = diskWrite(fat_cache->table_sec_idx + current_fat.sectors_per_fat, fat_cache->table_sector, 1);

    table_changed = 0;
    return resp;
}

u32 fatClusterToSector(u32 cluster) {

    return (cluster - 2) * current_fat.cluster_size + current_fat.data_entry;
}

u32 fatSectorToCluster(u32 sector) {

    return current_fat.data_entry + sector / current_fat.cluster_size + 2;
}

u8 fatGetNextCluster(u32 *cluster) {

    u8 resp;


    resp = fatGetTableRecord(*cluster, cluster);
    if (resp)return resp;

    if (current_fat.type == FAT_TYPE_16) {
        if (*cluster == 0xffff) {
            *cluster = 0;
        }
    } else {
        if (*cluster == 0x0FFFFFFF) {

            *cluster = 0;
        }
    }
    return 0;

}

u8 fatOpenFileByName(u8 *name, u32 wr_sectors) {

    //u16 i;
    u8 resp;
    FatRecord rec;
    //rec.name = fat_name;


    resp = fatFindRecord(name, &rec, 0);
    if (resp)return resp;
    if (rec.is_dir)return FAT_ERR_NOT_FILE;
    resp = fatOpenFile(&rec, wr_sectors);
    if (resp)return resp;

    return 0;
}

u8 fatFindRecord(u8 *name, FatRecord *rec, u8 is_dir) { //TODO: why does this return 0 for TRUE????

    u8 resp;
    u8 *sub_name_ptr;
    u8 sub_name[FAT_MAX_NAME_LEN_S];
    u8 name_len;
    //u8 cy = 2;
    //u16 i;



    rec->entry_cluster = 0;

    for (;;) {

        if (*name == '/')name++;
        sub_name_ptr = sub_name;
        name_len = 0;
        while (*name != 0 && *name != '/') {
            *sub_name_ptr++ = *name++;
            name_len++;
        }

        *sub_name_ptr = 0;

        //for (i = 0; sub_name[i] != 0; i++)if (sub_name[i] == ' ')sub_name[i] = '*';
        //VDP_drawText(APLAN, sub_name, 0, 20, cy++);
        fatOpenDir(rec->entry_cluster);
        for (;;) {


            resp = fatReadDir(rec, FAT_MAX_NAME_LEN_S);
            //VDP_drawText(APLAN, rec->name, 0, 20, cy++);
            if (resp)return resp;
            if (rec->name[0] == 0) {
                return FAT_ERR_NOT_EXIST;
            }



            if (streq(sub_name, rec->name)) {

                if (*name != 0)break;
                if (*name == 0 && ((is_dir && rec->is_dir) || (!is_dir && !rec->is_dir)))break;
            }
        }
        //& ((is_dir && rec->is_dir) || (!is_dir && !rec->is_dir))

        if (*name == 0) {
            //VDP_drawText(APLAN, "record found+++++", 0, 20, cy++);
            //VDP_drawText(APLAN, sub_name, 0, 20, cy++);

            return 0;
        }

    }

    return 0;

}

//FatRecordHdr hdr;

/*
void fatGetNameExt(u8 *filename, u8 *name, u8 *ext){

}
 */


void fatSetClustVal(FatRecordHdr *hdr, u32 clust_val) {

    ((u8 *) & hdr->cluster_lo)[0] = (u32) clust_val & 0xff;
    ((u8 *) & hdr->cluster_lo)[1] = (u32) clust_val >> 8;
    //hdr.cluster_lo = cluster & 0xffff;
    if (current_fat.type == FAT_TYPE_32) {
        ((u8 *) & hdr->cluster_hi)[0] = (u32) clust_val >> 16;
        ((u8 *) & hdr->cluster_hi)[1] = (u32) clust_val >> 24;
    }

}


#define fatSlen slen
#define fatStrHiCase strhicase

u8 fatCreateRecord(u8 *name, u8 is_dir, u8 check_exist) {

    //u8 cy = 10;
    u16 i;
    FatRecordHdr hdr;
    FatRecord rec;
    u8 patch[256];
    u8 *file_name;
    u8 *ptr;
    u8 lfn_blocks = 0;
    u8 resp;
    u8 sum = 0;
    u8 nam_len;
    u8 ext_len;
    u32 cluster;
    u8 lfn_block[32];
    u32 sub_dir_cluster = 0;

    //rec.name = rec_name;
    fat_cache->data_sec_idx = ~0;

    if (*name != '/')return FAT_ERR_NAME;
    if (fatSlen(name) < 2)return FAT_ERR_NAME;
    i = fatSlen(name) - 1;
    if (name[i] == '.' || name[i] == ' ')return FAT_ERR_NAME;


    if (check_exist) {
        resp = fatFindRecord(name, &rec, is_dir);
        if (resp != FAT_ERR_NOT_EXIST) {
            if (resp)return resp;
            return FAT_ERR_EXIST;
        }
    }



    memfill(&hdr, 0, 32);


    ptr = &name[fatSlen(name)];

    i = 0;
    while (*ptr != '/')ptr--;

    i = 0;
    while (name != ptr)patch[i++] = *name++;
    patch[i] = 0;

    ptr++;
    file_name = ptr;
    //i = 0;
    //while (*ptr != 0)file_name[i++] = *ptr++;
    //file_name[i] = 0;



    if (fatSlen(file_name) < 1)return FAT_ERR_NAME;


    i = fatSlen(file_name);
    while (i != 0 && file_name[i] != '.')i--;
    if (i == 0)i = fatSlen(file_name);
    nam_len = i;
    ext_len = fatSlen(file_name);
    if (ext_len == nam_len) {
        ext_len = 0;
    } else {
        ext_len -= (nam_len + 1);
    }



    if (nam_len > 8 || ext_len > 3) {

        lfn_blocks = fatSlen(file_name);
        if (lfn_blocks % 13 != 0)lfn_blocks += 13;
        lfn_blocks /= 13;
        for (i = 0; i < 8; i++)hdr.name[i] = file_name[i];
    } else {
        for (i = 0; i < 8 && file_name[i] != 0 && file_name[i] != '.'; i++)hdr.name[i] = file_name[i];
        for (; i < 8; i++)hdr.name[i] = ' ';
    }
    //VDP_drawText(APLAN, file_name, 0, 2, cy++);

    ptr = 0;
    if (ext_len != 0)ptr = &file_name[nam_len + 1];


    if (ptr != 0) {
        hdr.ext[0] = *ptr == 0 ? 0x20 : *ptr++;
        hdr.ext[1] = *ptr == 0 ? 0x20 : *ptr++;
        hdr.ext[2] = *ptr == 0 ? 0x20 : *ptr++;
    } else {
        for (i = 0; i < 3; i++)hdr.ext[i] = ' ';
    }
    fatStrHiCase(hdr.name, 8);
    fatStrHiCase(hdr.ext, 3);
    hdr.attrib = is_dir ? 0x10 : 0x20;




    if (patch[0] != 0) {
        resp = fatFindRecord(patch, &rec, 1);
        sub_dir_cluster = rec.entry_cluster;
        //if (resp == FAT_ERR_NOT_EXIST)VDP_drawText(APLAN, "cant find patch", 0, 2, cy++);
        if (resp)return resp;
    } else {
        rec.entry_cluster = 0;
    }


    resp = fatGetFreeRecord(&rec, lfn_blocks + 1);
    if (resp)return resp;

    //rec.hdr_sector = fat->root_entry;
    //rec.hdr_idx


    resp = fatCacheLoadData(rec.hdr_sector,0);
    if (resp)return resp;

    if (lfn_blocks != 0) {

        hdr.name[7] = '1';
        hdr.name[6] = '~';
        hdr.name[5] = (rec.hdr_idx + rec.hdr_sector / 32) & 15;
        hdr.name[4] = ((rec.hdr_idx + rec.hdr_sector / 32) >> 4) & 15;
        hdr.name[5] += hdr.name[5] <= 10 ? 48 : 55;
        hdr.name[4] += hdr.name[4] <= 10 ? 48 : 55;
        for (sum = i = 0; i < 11; i++) {
            if (i < 8) {
                sum = (((sum & 1) << 7) | ((sum & 0xfe) >> 1)) + hdr.name[i];
            } else {
                sum = (((sum & 1) << 7) | ((sum & 0xfe) >> 1)) + hdr.ext[i - 8];
            }
        }
    }

    for (i = 0; i < lfn_blocks; i++) {

        fatMakeLfnBlock(file_name, lfn_block, lfn_blocks - i - 1, sum);
        if (i == 0)lfn_block[0] |= 0x40;
        memcopy(lfn_block, &fat_cache->data_sector[rec.hdr_idx * 32], 32);
        rec.hdr_idx++;
        if (rec.hdr_idx == 16) {
            rec.hdr_sector++;
            rec.hdr_idx = 0;
            resp = fatCacheSaveData();
            if (resp)return resp;
            //resp = fatCacheLoadData(rec.hdr_sector);
            resp = fatCacheLoadData(rec.hdr_sector, 0);
            if (resp)return resp;
        }
    }
    /*
        if (lfn_blocks != 0) {
            resp = diskWriteSector(cache->buff_sector, cache->buff);
            if (resp)return resp;
        }
     */

    if (is_dir) {

        //ocuppy function may destroy cache buff data
        if (lfn_blocks != 0) {
            resp = diskWrite(fat_cache->data_sec_idx, fat_cache->data_sector, 1);
            if (resp)return resp;
        }
        cluster = 0;
        resp = fatOccupyClusters(&cluster, 1);
        if (resp)return resp;

        fatSetClustVal(&hdr, cluster);

        fat_cache->data_sec_idx = ~0;
        resp = fatCacheLoadData(rec.hdr_sector, 0);
        if (resp)return resp;
    }

    hdr.date = 0x5d11;
    hdr.time = 0;

    memcopy(&hdr, &fat_cache->data_sector[rec.hdr_idx * 32], 32);

    //resp = diskWriteSector(current_cache->data_sec_idx, current_cache->data_sector);
    resp = fatCacheSaveData();
    if (resp)return resp;



    if (is_dir) {

        u32 sector = fatClusterToSector(cluster);
        resp = fatClearClusters(cluster, 1);
        if (resp)return resp;

        resp = fatCacheLoadData(sector, 0);
        if (resp)return resp;

        memfill(&hdr, 0, 32);
        //memcopy((void *)rawData, &hdr, 32);
        hdr.attrib = 0x10;
        ptr = (u8 *) & hdr;
        for (i = 0; i < 11; i++)ptr[i] = 0x20;
        ptr[0] = 0x2e;

        fatSetClustVal(&hdr, cluster);

        memcopy(&hdr, fat_cache->data_sector, 32);

        fatSetClustVal(&hdr, sub_dir_cluster);
        ptr[1] = 0x2e;

        memcopy(&hdr, fat_cache->data_sector + 32, 32);

        fatCacheSaveData();
        //fatCacheLoadData()
    }

    return 0;
}

u8 fatCreateRecord2(u8 *name, u8 is_dir, u8 check_exist) {

    //u8 cy = 10;
    u16 i;
    FatRecordHdr hdr;
    FatRecord rec;
    u8 patch[FAT_MAX_NAME_LEN_S];
    u8 *file_name; //[FAT_MAX_NAME_LEN_S];
    u8 *ptr;
    u8 lfn_blocks = 0;
    u8 resp;
    u8 sum = 0;
    u8 nam_len;
    u8 ext_len;
    u32 cluster;
    u8 lfn_block[32];
    u32 sub_dir_cluster = 0;

    //rec.name = rec_name;
    fat_cache->data_sec_idx = ~0;

    if (*name != '/')return FAT_ERR_NAME;
    if (slen(name) < 2)return FAT_ERR_NAME;
    i = slen(name) - 1;
    if (name[i] == '.' || name[i] == ' ')return FAT_ERR_NAME;


    if (check_exist) {
        resp = fatFindRecord(name, &rec, is_dir);
        if (resp != FAT_ERR_NOT_EXIST) {
            if (resp)return resp;
            return FAT_ERR_EXIST;
        }
    }


    memfill(&hdr, 0, 32);


    ptr = &name[slen(name)];

    i = 0;
    while (*ptr != '/')ptr--;

    i = 0;
    while (name != ptr)patch[i++] = *name++;
    patch[i] = 0;

    ptr++;
    file_name = ptr;
    //i = 0;
    //while (*ptr != 0)file_name[i++] = *ptr++;
    //file_name[i] = 0;



    if (slen(file_name) < 1)return FAT_ERR_NAME;


    i = slen(file_name);
    while (i != 0 && file_name[i] != '.')i--;
    if (i == 0)i = slen(file_name);
    nam_len = i;
    ext_len = slen(file_name);
    if (ext_len == nam_len) {
        ext_len = 0;
    } else {
        ext_len -= (nam_len + 1);
    }



    if (nam_len > 8 || ext_len > 3) {

        lfn_blocks = slen(file_name);
        if (lfn_blocks % 13 != 0)lfn_blocks += 13;
        lfn_blocks /= 13;
        for (i = 0; i < 8; i++)hdr.name[i] = file_name[i];
    } else {
        for (i = 0; i < 8 && file_name[i] != 0 && file_name[i] != '.'; i++)hdr.name[i] = file_name[i];
        for (; i < 8; i++)hdr.name[i] = ' ';
    }
    //VDP_drawText(APLAN, file_name, 0, 2, cy++);

    ptr = 0;
    if (ext_len != 0)ptr = &file_name[nam_len + 1];


    if (ptr != 0) {
        hdr.ext[0] = *ptr == 0 ? 0x20 : *ptr++;
        hdr.ext[1] = *ptr == 0 ? 0x20 : *ptr++;
        hdr.ext[2] = *ptr == 0 ? 0x20 : *ptr++;
    } else {
        for (i = 0; i < 3; i++)hdr.ext[i] = ' ';
    }
    strhicase(hdr.name, 8);
    strhicase(hdr.ext, 3);
    hdr.attrib = is_dir ? 0x10 : 0x20;




    if (patch[0] != 0) {
        resp = fatFindRecord(patch, &rec, 1);
        sub_dir_cluster = rec.entry_cluster;
        //if (resp == FAT_ERR_NOT_EXIST)VDP_drawText(APLAN, "cant find patch", 0, 2, cy++);
        if (resp)return resp;
    } else {
        rec.entry_cluster = 0;
    }


    resp = fatGetFreeRecord(&rec, lfn_blocks + 1);
    if (resp)return resp;

    //rec.hdr_sector = fat.root_entry;
    //rec.hdr_idx


    resp = fatCacheLoadData(rec.hdr_sector, 0);
    if (resp)return resp;

    if (lfn_blocks != 0) {

        hdr.name[7] = '1';
        hdr.name[6] = '~';
        hdr.name[5] = (rec.hdr_idx + rec.hdr_sector / 32) & 15;
        hdr.name[4] = ((rec.hdr_idx + rec.hdr_sector / 32) >> 4) & 15;
        hdr.name[5] += hdr.name[5] <= 10 ? 48 : 55;
        hdr.name[4] += hdr.name[4] <= 10 ? 48 : 55;
        for (sum = i = 0; i < 11; i++) {
            if (i < 8) {
                sum = (((sum & 1) << 7) | ((sum & 0xfe) >> 1)) + hdr.name[i];
            } else {
                sum = (((sum & 1) << 7) | ((sum & 0xfe) >> 1)) + hdr.ext[i - 8];
            }
        }
    }

    for (i = 0; i < lfn_blocks; i++) {

        fatMakeLfnBlock(file_name, lfn_block, lfn_blocks - i - 1, sum);
        if (i == 0)lfn_block[0] |= 0x40;
        memcopy(lfn_block, &fat_cache->data_sector[rec.hdr_idx * 32], 32);
        rec.hdr_idx++;
        if (rec.hdr_idx == 16) {
            rec.hdr_sector++;
            rec.hdr_idx = 0;
            resp = fatCacheLoadData(rec.hdr_sector, 1);
            if (resp)return resp;
        }
    }
    /*
        if (lfn_blocks != 0) {
            resp = diskWriteSector(cache->buff_sector, cache->buff);
            if (resp)return resp;
        }
     */

    if (is_dir) {

        //ocuppy function may destroy cache buff data
        if (lfn_blocks != 0) {
            resp = diskWrite(fat_cache->data_sec_idx, fat_cache->data_sector, 1);
            if (resp)return resp;
        }
        cluster = 0;
        resp = fatOccupyClusters(&cluster, 1);
        if (resp)return resp;
        ((u8 *) & hdr.cluster_lo)[0] = (u32) cluster & 0xff;
        ((u8 *) & hdr.cluster_lo)[1] = (u32) cluster >> 8;
        //hdr.cluster_lo = cluster & 0xffff;
        if (current_fat.type == FAT_TYPE_32) {
            ((u8 *) & hdr.cluster_hi)[0] = (u32) cluster >> 16;
            ((u8 *) & hdr.cluster_hi)[1] = (u32) cluster >> 24;
        }

        fat_cache->data_sec_idx = ~0;
        resp = fatCacheLoadData(rec.hdr_sector, 0);
        if (resp)return resp;
    }

    hdr.date = 0x5d11;
    hdr.time = 0;

    memcopy(&hdr, &fat_cache->data_sector[rec.hdr_idx * 32], 32);

    resp = diskWrite(fat_cache->data_sec_idx, fat_cache->data_sector, 1);
    if (resp)return resp;

    if (is_dir) {

        u32 sector = fatClusterToSector(cluster);
        resp = fatClearClusters(cluster, 1);
        if (resp)return resp;

        resp = fatCacheLoadData(sector, 0);
        if (resp)return resp;

        memfill(&hdr, 0, 32);
        //memcopy((void *)rawData, &hdr, 32);
        hdr.attrib = 0x10;
        ptr = (u8 *) & hdr;
        for (i = 0; i < 11; i++)ptr[i] = 0x20;
        ptr[0] = 0x2e;

        fatSetClustVal(&hdr, cluster);

        memcopy(&hdr, fat_cache->data_sector, 32);

        fatSetClustVal(&hdr, sub_dir_cluster);
        ptr[1] = 0x2e;

        memcopy(&hdr, fat_cache->data_sector + 32, 32);

        fatCacheSaveData();
        //fatCacheLoadData()
    }

    return 0;
}

void fatMakeLfnBlock(u8 *name, u8 *lfn_block, u8 block_idx, u8 crc) {

    u8 i;
    name += block_idx * 13;
    for (i = 0; i < 13; i++) {
        if (name[i] == 0)break;
    }
    i++;
    for (; i < 13; i++) {
        name[i] = 0xff;
    }

    *lfn_block++ = block_idx + 1;

    for (i = 0; i < 5; i++) {
        *lfn_block++ = *name == 0xff ? 0xff : *name;
        *lfn_block++ = *name == 0xff ? 0xff : 0;
        name++;
    }
    *lfn_block++ = 0x0f;
    *lfn_block++ = 0;
    *lfn_block++ = crc;

    for (i = 0; i < 6; i++) {

        *lfn_block++ = *name == 0xff ? 0xff : *name;
        *lfn_block++ = *name == 0xff ? 0xff : 0;
        name++;
    }

    *lfn_block++ = 0;
    *lfn_block++ = 0;

    *lfn_block++ = *name == 0xff ? 0xff : *name;
    *lfn_block++ = *name == 0xff ? 0xff : 0;
    name++;
    *lfn_block++ = *name == 0xff ? 0xff : *name++;
    *lfn_block++ = *name == 0xff ? 0xff : 0;
    name++;

}

u8 fatGetFreeRecord(FatRecord *dir_rec, u8 len) {

    u8 len_ctr = 0;
    u32 sector;
    u8 resp;
    u32 current_cluster = dir_rec->entry_cluster;
    u32 next_cluster;
    u16 in_cluster_ptr = 0;
    u16 i;
    u8 is_root = 0;
    fat_cache->data_sec_idx = ~0;

    if (current_cluster == 0 && current_fat.type == FAT_TYPE_32)current_cluster = 2;

    if (current_cluster == 0) {
        //current_cluster = fatSectorToCluster(fat->root_entry);
        sector = current_fat.root_entry;
        is_root = 1;
    } else {
        sector = fatClusterToSector(current_cluster);
    }

    //VDP_drawText(APLAN, "get rec", 0, 15, cy++);

    while (len_ctr != len) {



        resp = fatCacheLoadData(sector,0);
        if (resp)return resp;

        for (i = 0; i < 512; i += 32) {

            if (fat_cache->data_sector[i] == 0 || fat_cache->data_sector[i] == 0xe5) {
                if (len_ctr == 0) {
                    dir_rec->hdr_sector = sector;
                    dir_rec->hdr_idx = i / 32;
                }
                len_ctr++;
            } else {
                len_ctr = 0;
                dir_rec->hdr_sector = 0;
            }
            if (len_ctr == len)return 0;
        }

        sector++;
        in_cluster_ptr++;

        if (is_root && current_fat.type == FAT_TYPE_16 && in_cluster_ptr >= 16384 / 512) {
            //VDP_drawText(APLAN, "root overflow", 0, 15, cy++);
            return FAT_ERR_ROT_OVERFLOW;
        }

        if (current_fat.type == FAT_TYPE_16 && is_root)continue;

        if (in_cluster_ptr == current_fat.cluster_size) {

            //VDP_drawText(APLAN, "cluster end", 0, 15, cy++);
            next_cluster = current_cluster;
            resp = fatGetNextCluster(&next_cluster);
            if (resp)return resp;
            if (next_cluster == 0) {
                //VDP_drawText(APLAN, "need to ocupy cluster for dir", 0, 15, cy++);
                resp = fatOccupyClusters(&current_cluster, 1);
                if (resp)return resp;
                resp = fatClearClusters(current_cluster, 1);
                if (resp)return resp;
                //VDP_drawText(APLAN, "ocupied!!!!!", TILE_ATTR(cy % 3, 0, 0, 0), 15, cy++);
                //cy %= 27;
            } else {
                current_cluster = next_cluster;
            }
            sector = fatClusterToSector(current_cluster);
            in_cluster_ptr = 0;
        }
    }

    return 0;
}

u8 fatClearClusters(u32 cluster, u8 len) {

    u32 i;
    u8 resp;
    memfill(fat_cache->data_sector, 0x00, 512);

    while (len--) {
        fat_cache->data_sec_idx = fatClusterToSector(cluster++);
        for (i = 0; i < current_fat.cluster_size; i++) {
            resp = diskWrite(fat_cache->data_sec_idx++, fat_cache->data_sector, 1);

            if (resp)return resp;
        }
    }
    return 0;
}

u8 fatOccupyClusters(u32 *base_cluster, u16 len) {


    if ((current_fat.type == FAT_TYPE_16 && *base_cluster == 0xffff) || (current_fat.type == FAT_TYPE_32 && *base_cluster == 0x0FFFFFFF))return FAT_ERR_BAD_BASE_CLUSTER;
    u32 *ptr32;
    u16 *ptr16;
    u16 i;
    u32 fat_table_sector = 0;
    u16 len_ctr = 0;
    u32 first_free_cluster = 0;
    u32 cluster_ctr = 0;
    u16 fat_page_len = current_fat.type == FAT_TYPE_16 ? 8192 : 4096;
    u8 resp = 0;


    kot = 0;
    //gDrawString("okupy 1", 32, 32);

    while (len_ctr < len) {


        //memcopy(current_cache->table_frame, occupy_buff, 512);
        resp = diskRead(current_fat.fat_entry + fat_table_sector, (void *) ROM_ADDR, 32);
        //resp = diskRead(current_fat.fat_entry + fat_table_sector, occupy_buff, 1);
        if (resp)return resp;
        dma_read(occupy_buff, ROM_ADDR, 32 * 512);


        if (current_fat.type == FAT_TYPE_16) {

            ptr16 = (u16 *) occupy_buff;


            for (i = 0; i < fat_page_len; i++) {

                if (*ptr16++ == 0) {
                    if (first_free_cluster == 0)first_free_cluster = cluster_ctr;
                    len_ctr++;
                    if (len_ctr == len)break;
                } else if (len_ctr != 0) {
                    len_ctr = 0;
                    first_free_cluster = 0;
                }
                cluster_ctr++;

            }

            if (cluster_ctr == 0x10000)return FAT_ERR_NO_FRE_SPACE;

        } else {

            ptr32 = (u32 *) occupy_buff;
            for (i = 0; i < fat_page_len; i++) {


                if (*ptr32++ == 0) {
                    if (first_free_cluster == 0)first_free_cluster = cluster_ctr;
                    len_ctr++;
                    if (len_ctr == len)break;
                } else if (len_ctr != 0) {
                    len_ctr = 0;
                    first_free_cluster = 0;
                }
                cluster_ctr++;

            }
        }



        fat_table_sector += 32;
    }


    /*
    gConsDrawString("oc: ");
    gAppendNum(first_free_cluster);
     */

    //cache current fact sector to avoid dummy rewrite
    /*
    cluster = *base_cluster != 0 ? *base_cluster : first_free_cluster;
    table_sector = current_fat.type == FAT_TYPE_16 ? cluster / 256 : cluster / 128;
    resp = fatCacheLoadTable(table_sector, 0);
    if (resp)return resp;*/

    if (*base_cluster != 0) {
        resp = fatSetTableRecord(*base_cluster, first_free_cluster);
        if (resp)return resp;
    }

    *base_cluster = first_free_cluster;

    while (len--) {

        if (len == 0) {
            resp = fatSetTableRecord(first_free_cluster, 0x0FFFFFFF);
        } else {
            resp = fatSetTableRecord(first_free_cluster, first_free_cluster + 1);
        }
        if (resp)return resp;
        first_free_cluster++;
    }

    fatCacheApplyTable();
    if (resp)return resp;

    return 0;
}

u8 fatGetTableRecord(u32 cluster, u32 *val) {

    u8 resp;
    u8 *ptr;

    u32 table_sector = current_fat.type == FAT_TYPE_16 ? cluster / 256 : cluster / 128;
    resp = fatCacheLoadTable(table_sector, 0);

    if (resp)return resp;

    if (current_fat.type == FAT_TYPE_16) {
        ptr = &fat_cache->table_sector[(cluster & 255) * 2];
        *val = (u32) ptr[0] | (u32) ptr[1] << 8;
    } else {

        ptr = &fat_cache->table_sector[(cluster & 127) * 4];
        *val = (u32) ptr[0] | (u32) ptr[1] << 8 | (u32) ptr[2] << 16 | (u32) ptr[3] << 24;
    }


    return 0;

}

u8 fatSetTableRecord(u32 cluster, u32 next_cluster) {

    u8 resp;
    u32 table_sector = current_fat.type == FAT_TYPE_16 ? cluster / 256 : cluster / 128;
    u8 *ptr;

    resp = fatCacheLoadTable(table_sector, table_changed);

    if (resp)return resp;

    if (current_fat.type == FAT_TYPE_16) {
        ptr = &fat_cache->table_sector[(cluster & 255) * 2];
        ptr[0] = (u32) next_cluster >> 0;
        ptr[1] = (u32) next_cluster >> 8;
    } else {

        ptr = &fat_cache->table_sector[(cluster & 127) * 4];
        ptr[0] = (u32) next_cluster >> 0;
        ptr[1] = (u32) next_cluster >> 8;
        ptr[2] = (u32) next_cluster >> 16;
        ptr[3] = (u32) next_cluster >> 24;
    }

    table_changed = 1;

    return 0;
}

/*
u8 fatWriteFile(void *src, u32 sectors) {

    u8 resp;
    u8 *ptr8 = (u8 *) src;

    if (file.mode != FILE_MODE_WR)return FAT_ERR_FILE_MODE;
    if (sectors > file.sec_available)return FAT_ERR_OUT_OF_FILE;
    while (sectors--) {

        resp = diskWrite(file.sector, ptr8, 1);
        if (resp)return resp;
        ptr8 += 512;
        resp = fatSkipSectors(1);
        if (resp)return resp;
    }


    if (file.sec_available == 0)file.mode = 0;

    return 0;
}*/


u8 fatWriteFile(void *src, u32 sectors) {

    u8 resp;
    u8 *ptr8 = (u8 *) src;
    u32 len;
    u32 readed;

    if (file.mode != FILE_MODE_WR)return FAT_ERR_FILE_MODE;
    if (sectors > file.sec_available)return FAT_ERR_OUT_OF_FILE;

    while (sectors) {

        if (file.in_cluster_ptr == 0 && sectors >= current_fat.cluster_size) {
            resp = fatMultiClustRW(src, sectors, &readed, 1);
            sectors -= readed;
            //console_printf("len: %d\n", readed);
            //joyWait();

        } else {

            len = sectors + file.in_cluster_ptr < current_fat.cluster_size ? sectors : 1;

            //console_printf("len: %d\n", len);
            //joyWait();
            resp = diskWrite(file.sector, ptr8, len);
            if (resp)return resp;
            ptr8 += 512 * len;
            resp = fatSkipSectors(len);
            if (resp)return resp;
            sectors -= len;
        }


    }


    if (file.sec_available == 0)file.mode = 0;

    return 0;
}

u8 fatReadFile(void *dst, u32 sectors) {

    u8 resp;
    u32 readed;
    u32 len;

    if (file.mode != FILE_MODE_RD)return FAT_ERR_FILE_MODE;
    if (sectors > file.sec_available)return FAT_ERR_OUT_OF_FILE;

    while (sectors) {

        if (sectors < current_fat.cluster_size || file.in_cluster_ptr != 0) {

            len = current_fat.cluster_size - file.in_cluster_ptr;
            if (len > sectors)len = sectors;

            resp = diskRead(file.sector, dst, len);

            if (resp)return resp;
            dst += 512 * len;
            resp = fatSkipSectors(len);
            if (resp)return resp;
            sectors = sectors >= len ? sectors - len : 0;
        } else {

            resp = fatMultiClustRW(dst, sectors, &readed, 0);
            if (resp)return resp;
            dst += readed * 512;
            sectors -= readed;
        }
    }

    if (file.sec_available == 0)file.mode = 0;

    return 0;
}

/*
u8 fatReadPartialFile(void *dst, u32 sectors, int b_offset) {

//1048576 * 8 sectors 
	
	//secotrs = datei gesamt-sektoranzahl
    u8 resp;
    u32 readed;
    u32 len;

    if (file.mode != FILE_MODE_RD)return FAT_ERR_FILE_MODE;
    if (end_sector > file.sec_available || offset_sector > file.sec_available)return FAT_ERR_OUT_OF_FILE;


//first   run b_offset = 0 -> 1Mb lesen
//also einmal tmp_sectors
//zweiter run b_offset = 1 -> 1Mb lesen
//aber ab offset 1024*1024 * 1

	tmp_sectors = 1024*1024; //1048576
	//for(int i=0; i < ;i++){ //1 mb
    while (tmp_sectors) {
		
				//gesamt < 4096 (8*512)    oder c_ptr!=0 ->  >first run?
        if (sectors < current_fat.cluster_size || file.in_cluster_ptr != 0) {

			//rest sectores
            len = current_fat.cluster_size - file.in_cluster_ptr;
            if (len > sectors)len = sectors;

            resp = diskRead(file.sector, dst, len); //len in sectors
            if (resp)return resp;
            dst += 512 * len;
            resp = fatSkipSectors(len);
            if (resp)return resp;
            sectors = sectors >= len ? sectors - len : 0;
        } else {
				//anfang liest ganze 512 cluster
				//sectors sollte offset+1Mb
            resp = fatMultiClustRW(dst, sectors, &readed, 0);
            if (resp)return resp;
            dst += readed * 512;
            sectors -= readed; //zieht großen block ab
        }
    }

    if (file.sec_available == 0)file.mode = 0;

    return 0;
} 
*/

u8 fatMultiClustRW(void *buff, u32 max_sectors, u32 *readrd_sectors, u16 write) {

    u8 resp;
    u32 clust = max_sectors / current_fat.cluster_size;
    if (max_sectors % current_fat.cluster_size != 0)clust++;
    u32 begin_sect = file.sector;
    u32 len = 0;
    u32 prev_clust = file.cluster;
    u32 max_len = 0x2000000 / current_fat.cluster_byte_size;

    while (clust-- && len < max_len) {
        len++;
        resp = fatSkipCluster();
        if (resp)return resp;
        if (prev_clust + 1 != file.cluster)break;
        prev_clust = file.cluster;
    }

    len *= current_fat.cluster_size;
    if (len > max_sectors)len = max_sectors;
    *readrd_sectors = len;

    if (write) {
        resp = diskWrite(begin_sect, buff, len);
    } else {
        resp = diskRead(begin_sect, buff, len);
    }

    return resp;

}

u8 fatSkipCluster() {

    u8 resp;

    resp = fatGetNextCluster(&file.cluster);
    if (resp)return resp;
    file.sec_available = file.sec_available < current_fat.cluster_size ? 0 : file.sec_available - current_fat.cluster_size;
    if (file.sec_available != 0 && file.cluster == 0)return FAT_ERR_OUT_OF_TABLE;
    file.sector = fatClusterToSector(file.cluster);
    file.in_cluster_ptr = 0;

    if (file.sec_available == 0)file.mode = 0;

    return 0;
}

u8 fatReadCluster(void *dst) {

    u8 resp;
    if (file.mode != FILE_MODE_RD)return FAT_ERR_FILE_MODE;

    resp = diskRead(file.sector, dst, current_fat.cluster_size);
    if (resp)return resp;
    dst += current_fat.cluster_byte_size;
    resp = fatGetNextCluster(&file.cluster);
    //file.cluster ++;
    if (resp)return resp;
    file.sec_available = file.sec_available < current_fat.cluster_size ? 0 : file.sec_available - current_fat.cluster_size;
    if (file.sec_available != 0 && file.cluster == 0)return FAT_ERR_OUT_OF_TABLE;
    file.sector = fatClusterToSector(file.cluster);
    file.in_cluster_ptr = 0;

    if (file.sec_available == 0)file.mode = 0;

    return 0;
}

u8 fatSkipSectors(u32 sectors) {

    u8 resp;

    if (sectors == 0)return 0;

    if (sectors > file.sec_available)sectors = file.sec_available;
    while (sectors--) {

        file.sector++;
        file.sec_available--;
        file.in_cluster_ptr++;
        if (file.in_cluster_ptr == current_fat.cluster_size) {
            resp = fatGetNextCluster(&file.cluster);
            if (resp)return resp;

            if (file.cluster == 0 && sectors != 0)return FAT_ERR_OUT_OF_TABLE;
            file.sector = fatClusterToSector(file.cluster);
            file.in_cluster_ptr = 0;
        }
    }


    return 0;
}

u8 fatOpenFile(FatRecord *rec, u32 wr_sectors) {


    u8 resp;
    file.sec_available = rec->size / 512;
    if ((rec->size & 511) != 0)file.sec_available++;
    file.cluster = rec->entry_cluster;
    file.in_cluster_ptr = 0;
    file.mode = wr_sectors == 0 ? FILE_MODE_RD : FILE_MODE_WR;



    if (wr_sectors > file.sec_available) {

        resp = fatResizeFile(rec, wr_sectors);
        if (resp)return resp;
        if (file.cluster == 0)file.cluster = rec->entry_cluster;
        file.sec_available = wr_sectors;
    }

    file.sector = fatClusterToSector(file.cluster);


    return 0;

}

/*
u8 fatOpenFile(FatRecord *rec, u32 wr_sectors) {

    u8 resp;
    u32 clusters_need;
    FatRecordHdr hdr;
    u32 clust;

    u32 size;
    u32 clusters_available;
    u32 wr_clusters_req;


    current_cache->data_sector = ~0;
    memcopy(rec, &file.rec, sizeof (FatRecord));


    file.sec_available = file.rec.size / 512;
    if ((file.rec.size & 511) != 0)file.sec_available++;
    file.cluster = file.rec.entry_cluster;
    file.in_cluster_ptr = 0;
    file.mode = wr_sectors == 0 ? FILE_MODE_RD : FILE_MODE_WR;

    clusters_available = file.sec_available / current_fat.cluster_size;
    if ((file.sec_available & (current_fat.cluster_size - 1)) != 0)clusters_available++;
    wr_clusters_req = wr_sectors / current_fat.cluster_size;
    if ((wr_sectors & (current_fat.cluster_size - 1)) != 0)wr_clusters_req++;

    if (wr_sectors > file.sec_available) {


        if (clusters_available == 0 || clusters_available < wr_clusters_req) {
            //VDP_drawText(APLAN, "ned to get some clusters", 0, 2, cy++);
            clusters_need = (wr_sectors - file.sec_available);
            if ((clusters_need & (current_fat.cluster_size - 1)) != 0)clusters_need += current_fat.cluster_size;
            clusters_need /= current_fat.cluster_size;
            clust = file.cluster;
            if (clusters_available != 0)clust += clusters_available - 1;

            resp = fatOccupyClusters(&clust, clusters_need);


            if (resp)return resp;
            if (file.cluster == 0)file.cluster = clust;
        }

        file.sec_available = wr_sectors;
        current_cache->data_sector = ~0;
        fatCacheLoadBuff(file.rec.hdr_sector, 0);
        memcopy(&current_cache->data_frame[file.rec.hdr_idx * 32], &hdr, 32);
        size = file.sec_available * 512;
        //hdr.size = 0x12345;
        ((u8 *) & hdr.size)[0] = (u32) size;
        ((u8 *) & hdr.size)[1] = (u32) size >> 8;
        ((u8 *) & hdr.size)[2] = (u32) size >> 16;
        ((u8 *) & hdr.size)[3] = (u32) size >> 24;

        ((u8 *) & hdr.cluster_lo)[0] = (u32) file.cluster & 0xff;
        ((u8 *) & hdr.cluster_lo)[1] = (u32) file.cluster >> 8;
        //hdr.cluster_lo = cluster & 0xffff;
        if (current_fat.type == FAT_TYPE_32) {
            ((u8 *) & hdr.cluster_hi)[0] = (u32) file.cluster >> 16;
            ((u8 *) & hdr.cluster_hi)[1] = (u32) file.cluster >> 24;
        }


        memcopy(&hdr, &current_cache->data_frame[file.rec.hdr_idx * 32], 32);

        resp = diskWrite(current_cache->data_sector, current_cache->data_frame, 1);

        if (resp)return resp;
    }

    file.sector = fatClusterToSector(file.cluster);

    return 0;
}
 */
u8 fatResizeFile(FatRecord *rec, u32 req_sec_size) {

    u8 resp;
    u32 sec_available;
    u32 wr_clusters_req;
    u32 clusters_available;
    u32 clusters_need;
    u32 clust;
    u16 i;
    FatRecordHdr *hdr;
    u32 size;



    sec_available = rec->size / 512;
    if ((rec->size & 511) != 0)sec_available += 512;

    if (req_sec_size <= sec_available)return FAT_ERR_RESIZE;


    clusters_available = sec_available / current_fat.cluster_size;
    if ((sec_available % current_fat.cluster_size) != 0)clusters_available++;
    wr_clusters_req = req_sec_size / current_fat.cluster_size;
    if (req_sec_size % current_fat.cluster_size != 0)wr_clusters_req++;

    if (clusters_available < wr_clusters_req) {

        clusters_need = (wr_clusters_req - clusters_available);
        clust = rec->entry_cluster;
        if (clusters_available != 0) {
            for (i = 0; i < clusters_available - 1; i++) {
                fatGetNextCluster(&clust);
            }
        }

        resp = fatOccupyClusters(&clust, clusters_need);

        if (rec->entry_cluster == 0)rec->entry_cluster = clust;
        if (resp)return resp;
    }

    //resp = fatCacheLoadData(rec->hdr_sector);
    resp = fatCacheLoadData(rec->hdr_sector, 0);
    if (resp)return resp;
    hdr = (FatRecordHdr *) & fat_cache->data_sector[rec->hdr_idx * 32];

    size = req_sec_size * 512;
    ((u8 *) & hdr->size)[0] = (u32) size;
    ((u8 *) & hdr->size)[1] = (u32) size >> 8;
    ((u8 *) & hdr->size)[2] = (u32) size >> 16;
    ((u8 *) & hdr->size)[3] = (u32) size >> 24;


    ((u8 *) & hdr->cluster_lo)[0] = (u32) rec->entry_cluster & 0xff;
    ((u8 *) & hdr->cluster_lo)[1] = (u32) rec->entry_cluster >> 8;
    if (current_fat.type == FAT_TYPE_32) {
        ((u8 *) & hdr->cluster_hi)[0] = (u32) rec->entry_cluster >> 16;
        ((u8 *) & hdr->cluster_hi)[1] = (u32) rec->entry_cluster >> 24;
    }

    resp = fatCacheSaveData();
    if (resp)return resp;


    return 0;
}
