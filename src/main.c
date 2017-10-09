//
// Copyright (c) 2017 The Altra64 project contributors
// Portions (c) 2013 saturnu (Alt64) based on libdragon, Neo64Menu, ED64IO, libn64-hkz, libmikmod
// See LICENSE file in the project root for full license information.
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

//libdragon n64 lib
#include <libdragon.h>

//everdrive system libs
#include "types.h"
#include "sys.h"
#include "everdrive.h"
#include "everdrive.h"

//filesystem
#include "disk.h"
#include "fat.h"

//utils
#include "utils.h"

//config file on sd
#include "ini.h"
#include "strlib.h"

//main header
#include "main.h"

//sound
#include "sound.h"
#include "mp3.h"

//debug
#include "debug.h"

// YAML parser
#include <yaml.h>

#ifdef USE_TRUETYPE
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct glyph
{
    int xoff;
    int yoff;
    int adv;
    int lsb;
    float scale;
    unsigned int color;
    unsigned char *alpha;
    sprite_t sprite;
};
typedef struct glyph glyph_t;

glyph_t sans[96];
glyph_t serif[96];
#endif

typedef struct
{
    uint32_t type;
    uint32_t color;
    char filename[MAX_FILENAME_LEN + 1];
} direntry_t;

//ini file
typedef struct
{
    int version;
    const char *name;
    char *background_image;
    char *border_color_1;
    char *border_color_2;
    char *box_color;
    char *selection_color;
    char *list_font_color;
    char *list_dir_font_color;
    char *selection_font_color;
    char *mempak_path;
    char *save_path;
    int sound_on;
    int page_display;
    int tv_mode;
    int quick_boot;
    int enable_colored_list;
    int ext_type;
    int cd_behaviour;
    int scroll_behaviour;
    int text_offset;
    int hide_sysfolder;
    int sd_speed;

} configuration;

volatile u32 *romaddr_ptr = (u32 *)ROM_ADDR;
unsigned int gBootCic = CIC_6102;

static uint8_t mempak_data[128 * 256];
static void *bg_buffer;
void *__safe_bg_buffer;

#define __get_buffer(x) __safe_buffer[(x)-1]
extern uint32_t __bitdepth;
extern uint32_t __width;
extern uint32_t __height;
extern void *__safe_buffer[];

char firmware_str[64];
int firm_found = 0;
char rom_config[10];

display_context_t lockVideo(int wait);

int select_mode = 0;
int toplist_reload = 1;
int toplist_cursor = 0;
char toplist15[15][256];

int fat_initialized = 0;
int exit_ok = 0;
int boot_cic = 0;
int boot_save = 0;

int cursor_line = 0;
int cursor_lastline = 0;
u16 cursor_history[32];
u16 cursor_history_pos = 0;

u8 empty = 0;
u8 playing = 0;
u8 gb_load_y = 0;

//start with filebrowser menu key settings
enum InputMap
{
    none,
    file_manager,
    mempak_menu,
    char_input,
    rom_loaded,
    mpk_format,
    mpk_restore,
    rom_config_box,
    toplist,
    mpk_choice,
    mpk_quick_backup,
    mp3,
    abort_screen,
};
enum InputMap input_mapping = file_manager;

//holds the string of the character input screen result
//int text_input_on = 0;

//char input vars
int x, y, position, set;

unsigned char input_text[32];
uint32_t chr_forecolor;
uint32_t chr_backcolor;

//save type still set - save to do after reboot
int save_after_reboot = 0;

//cart id from the rom header
unsigned char cartID[4];
char curr_dirname[64];
char pwd[64];
char rom_filename[128];

u32 rom_buff[128]; //rom buffer
u8 *rom_buff8;     //rom buffer

u8 *firmware;
u8 gbload = 0;

int cheats_on = 0;
int checksum_fix_on = 0;
short int gCheats = 0; /* 0 = off, 1 = select, 2 = all */
short int force_tv = 0;
short int boot_country = 0;

static resolution_t res = RESOLUTION_320x240;

//background sprites
sprite_t *loadPng(u8 *png_filename);
sprite_t *background;   //background
sprite_t *splashscreen; //splash screen

//config file theme settings
u32 border_color_1 = 0xFFFFFFFF; //hex 0xRRGGBBAA AA=transparenxy
u32 border_color_2 = 0x3F3F3FFF;
u32 box_color = 0x00000080;
u32 selection_color = 0x6495ED60;
u32 selection_font_color = 0xFFFFFFFF;
u32 list_font_color = 0xFFFFFFFF;
u32 list_dir_font_color = 0xFFFFFFFF;

char *border_color_1_s;
char *border_color_2_s;
char *box_color_s;
char *selection_color_s;
char *selection_font_color_s;
char *list_font_color_s;
char *list_dir_font_color_s;

char *mempak_path;
char *save_path;

u8 sound_on = 0;
u8 page_display = 0;
int text_offset = 0;
u8 tv_mode = 0; // 1=ntsc 2=pal 3=mpal 0=default automatic
u8 quick_boot = 0;
u8 enable_colored_list = 0;
u8 cd_behaviour = 0;     //0=first entry 1=last entry
u8 scroll_behaviour = 0; //1=classic 0=new page-system
u8 ext_type = 0;         //0=classic 1=org os
u8 sd_speed = 1;         // 1=25Mhz 2=50Mhz
u8 hide_sysfolder = 0;
char *background_image;

//mp3
int buf_size;
char *buf_ptr;

//toplist helper
int list_pos_backup[3];
char list_pwd_backup[256];

char dirz[512] = "rom://";

short int gCursorX;
short int gCursorY;

int count = 0;
int page = 0;
int cursor = 0;
direntry_t *list;

int filesize(FILE *pFile)
{
    fseek(pFile, 0, SEEK_END);
    int lSize = ftell(pFile);
    rewind(pFile);

    return lSize;
}

sprite_t *read_sprite(const char *const spritename)
{
    FILE *fp = fopen(spritename, "r");

    if (fp)
    {
        sprite_t *sp = malloc(filesize(fp));
        fread(sp, 1, filesize(fp), fp);
        fclose(fp);

        return sp;
    }
    else
    {
        return 0;
    }
}

void drawSelection(display_context_t disp, int p)
{
    int s_x = 23 + (text_offset * 8);

    if (scroll_behaviour)
    {
        if (select_mode)
        {
            if (cursor_lastline > cursor && cursor_line > 0)
            {
                cursor_line--;
            }

            if (cursor_lastline < cursor && cursor_line < 19)
            {
                cursor_line++;
            }

            p = cursor_line;
            graphics_draw_box_trans(disp, s_x, (((p + 3) * 8) + 24), 272, 8, selection_color); //(p+3) diff
            cursor_lastline = cursor;
        }
    }
    else
    { //new page-system
        //accept p
        graphics_draw_box_trans(disp, s_x, (((p + 3) * 8) + 24), 272, 8, selection_color);
    }
}

void drawConfigSelection(display_context_t disp, int l)
{
    int s_x = 62 + (text_offset * 8);

    l = l + 5;
    graphics_draw_box_trans(disp, s_x, (((l + 3) * 8) + 24), 193, 8, 0x00000080); //(p+3) diff
}

void drawToplistSelection(display_context_t disp, int l)
{
    int s_x = 30 + (text_offset * 8);

    l = l + 2;
    graphics_draw_box_trans(disp, s_x, (((l + 3) * 8) + 24), 256, 8, 0xFFFFFF70); //(p+3) diff
}

void chdir(const char *const dirent)
{
    /* Ghetto implementation */
    if (strcmp(dirent, "..") == 0)
    {
        /* Go up one */
        int len = strlen(dirz) - 1;

        /* Stop going past the min */
        if (dirz[len] == '/' && dirz[len - 1] == '/' && dirz[len - 2] == ':')
        {
            //return if ://
            return;
        }

        if (dirz[len] == '/')
        {
            dirz[len] = 0;
            len--;
        }

        while (dirz[len] != '/')
        {
            dirz[len] = 0;
            len--;
        }
    }
    else
    {
        /* Add to end */
        strcat(dirz, dirent);
        strcat(dirz, "/");
    }
}

int compare(const void *a, const void *b)
{
    direntry_t *first = (direntry_t *)a;
    direntry_t *second = (direntry_t *)b;

    if (first->type == DT_DIR && second->type != DT_DIR)
    {
        /* First should be first */
        return -1;
    }

    if (first->type != DT_DIR && second->type == DT_DIR)
    {
        /* First should be second */
        return 1;
    }

    return strcmp(first->filename, second->filename);
}

int compare_int(const void *a, const void *b)
{
    const int *ia = (const int *)a; // casting pointer types
    const int *ib = (const int *)b;
    return *ia - *ib;
}

int compare_int_reverse(const void *a, const void *b)
{
    const int *ia = (const int *)a; // casting pointer types
    const int *ib = (const int *)b;
    return *ib - *ia;
}

void new_scroll_pos(int *cursor, int *page, int max, int count)
{
    /* Make sure windows too small can be calculated right */
    if (max > count)
    {
        max = count;
    }

    /* Bounds checking */
    if (*cursor >= count)
    {
        *cursor = count - 1;
    }

    if (*cursor < 0)
    {
        *cursor = 0;
    }

    /* Scrolled up? */
    if (*cursor < *page)
    {
        *page = *cursor;
        return;
    }

    /* Scrolled down/ */
    if (*cursor >= (*page + max))
    {
        *page = (*cursor - max) + 1;
        return;
    }

    /* Nothing here, should be good! */
}

void display_dir(direntry_t *list, int cursor, int page, int max, int count, display_context_t disp)
{
    //system color
    uint32_t forecolor = 0;
    uint32_t forecolor_menu = 0;
    uint32_t backcolor;
    backcolor = graphics_make_color(0x00, 0x00, 0x00, 0x00);      //bg
    forecolor_menu = graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF); //fg

    graphics_set_color(list_font_color, backcolor);

    u8 c_pos[MAX_LIST + 1];
    int c_pos_counter = 0;

    c_pos[c_pos_counter++] = 0;

    u8 c_dirname[64];

    if (page_display)
    {

        u8 pi = page / 20;
        u8 ci = 0;

        if (count % 20 == 0)
            ci = (count - 1) / 20;
        else
            ci = count / 20;
        sprintf(c_dirname, "%i/%i SD:/%s", pi + 1, ci + 1, pwd);
    }
    else
    {
        sprintf(c_dirname, "SD:/%s", pwd);
    }
    char sel_str[128];

    printText(c_dirname, 3, 4, disp);

    int firstrun = 1;
    /* Page bounds checking */
    if (max > count)
    { //count = directories starting at 1
        max = count;
    }

    /* Cursor bounds checking */
    if (cursor >= (page + max))
    {
        cursor = (page + max) - 1;
    }

    if (cursor < page)
    {
        cursor = page;
    }

    if (max == 0)
    {
        printText("dir empty...", 3, 6, disp);
        sprintf(sel_str, "dir empty...");
        empty = 1;
    }
    else
    {
        empty = 0;
    }

    //last page anti ghosting entries
    if (page == (count / 20) * 20)
        max = count % 20;

    for (int i = page; i < (page + max); i++)
    { //from page to page + max
        if (list[i].type == DT_DIR)
        {
            char tmpdir[(CONSOLE_WIDTH - 5) + 1];
            strncpy(tmpdir, list[i].filename, CONSOLE_WIDTH - 5);

            tmpdir[CONSOLE_WIDTH - 5] = 0;

            char *dir_str;
            dir_str = malloc(slen(tmpdir) + 3);

            if (i == cursor)
            {
                sprintf(dir_str, " [%s]", tmpdir);
                sprintf(sel_str, " [%s]", tmpdir);

                if (scroll_behaviour)
                    drawSelection(disp, i);

                c_pos[c_pos_counter++] = 1;
            }
            else
            {
                sprintf(dir_str, "[%s]", tmpdir);
                c_pos[c_pos_counter++] = 0;
            }
            graphics_set_color(list_dir_font_color, backcolor);
            if (firstrun)
            {
                printText(dir_str, 3, 6, disp);
                firstrun = 0;
            }
            else
            {
                printText(dir_str, 3, -1, disp);
            }
            free(dir_str);
        }
        else
        { //if(list[i].type == DT_REG)
            int fcolor = list[i].color;

            if (fcolor != 0)
            {
                switch (fcolor)
                {
                case 1:
                    forecolor = graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF); //common (white)
                    break;
                case 2:
                    forecolor = graphics_make_color(0x00, 0xFF, 0x00, 0xCF); //uncommon (green)
                    break;
                case 3:
                    forecolor = graphics_make_color(0x1E, 0x90, 0xFF, 0xFF); //rare (blue)
                    break;
                case 4:
                    forecolor = graphics_make_color(0x9B, 0x30, 0xFF, 0xFF); //epic (purple)
                    break;
                case 5:
                    forecolor = graphics_make_color(0xFF, 0xA5, 0x00, 0xFF); //legendary (orange)
                    break;
                default:
                    break;
                }
            }
            else
                forecolor = list_font_color;

            char tmpdir[(CONSOLE_WIDTH - 3) + 1];
            strncpy(tmpdir, list[i].filename, CONSOLE_WIDTH - 3);
            tmpdir[CONSOLE_WIDTH - 3] = 0;

            char *dir_str;
            dir_str = malloc(slen(tmpdir) + 1);

            if (i == cursor)
            {
                sprintf(dir_str, " %s", tmpdir);
                sprintf(sel_str, " %s", tmpdir);

                if (scroll_behaviour)
                    drawSelection(disp, i);

                c_pos[c_pos_counter++] = 1;
            }
            else
            {
                sprintf(dir_str, "%s", tmpdir);
                c_pos[c_pos_counter++] = 0;
            }

            graphics_set_color(list_font_color, backcolor);

            if (firstrun)
            {
                if (enable_colored_list)
                    graphics_set_color(forecolor, backcolor);

                printText(dir_str, 3, 6, disp); //3,6
                firstrun = 0;
            }
            else
            {
                if (enable_colored_list)
                    graphics_set_color(forecolor, backcolor);

                printText(dir_str, 3, -1, disp); //3,1
            }
            free(dir_str);
        }
    }

    //for page-wise scrolling
    if (scroll_behaviour == 0)
    {
        int c = 0;
        for (c = 0; c < max + 1; c++)
        {
            if (c_pos[c] == 1)
            {
                drawSelection(disp, c - 1);
                //todo: set selection color
                graphics_set_color(selection_font_color, backcolor);
                printText(sel_str, 3, 6 + c - 1, disp);
                graphics_set_color(forecolor, backcolor);
            }
        }
    }
    graphics_set_color(forecolor_menu, backcolor);
}

void printText(char *msg, int x, int y, display_context_t dcon)
{
    x = x + text_offset;

    if (x != -1)
        gCursorX = x;
    if (y != -1)
        gCursorY = y;

    if (dcon)
        graphics_draw_text(dcon, gCursorX * 8, gCursorY * 8, msg);

    gCursorY++;
    if (gCursorY > 29)
    {
        gCursorY = 0;
        gCursorX++;
    }
}

//background sprite
void drawBg(display_context_t disp)
{
    graphics_draw_sprite(disp, 0, 0, background);
}

void drawBox(short x, short y, short width, short height, display_context_t disp)
{
    x = x + (text_offset * 8);

    /*
     *    |Y
     *    |  x0/y0
     *    |
     *    |
     *    |          x1/y1
     *    |_______________X
     *
     */

    //                          X0 Y0  X1      Y1
    graphics_draw_line(disp, x, y, width + x, y, border_color_1);                   //A top left tp bottom right ok
    graphics_draw_line(disp, width + x, y, width + x, height + y, border_color_1);  //B top right to bottom right
    graphics_draw_line(disp, x, y, x, height + y, border_color_2);                  //C  //152-20
    graphics_draw_line(disp, x, height + y, width + x, height + y, border_color_2); //D
    graphics_draw_box_trans(disp, x + 1, y + 1, width - 1, height - 1, box_color);  //red light transparent
}

void drawBoxNumber(display_context_t disp, int box)
{
    int old_color = box_color;
    //backup color

    switch (box)
    {
    case 1:
        drawBox(20, 24, 277, 193, disp);
        break; //filebrowser
    case 2:
        drawBox(60, 56, 200, 128, disp);
        break; //info screen
    case 3:
        box_color = graphics_make_color(0x00, 0x00, 0x60, 0xC9);
        drawBox(79, 29, 161, 180, disp);
        break; //cover
    case 4:
        box_color = graphics_make_color(0x30, 0x00, 0x00, 0xA3);
        drawBox(79, 29, 161, 180, disp);
        break; //mempak content
    case 5:
        box_color = graphics_make_color(0x60, 0x00, 0x00, 0xD3);
        drawBox(60, 64, 197, 114, disp); //red confirm screen
        break;                           //mempak content
    case 6:
        box_color = graphics_make_color(0x60, 0x60, 0x00, 0xC3);
        drawBox(60, 64, 197, 125, disp); //yellow screen
        break;                           //rom config box
    case 7:
        box_color = graphics_make_color(0x00, 0x00, 0x60, 0xC3);
        drawBox(60, 105, 197, 20, disp); //blue info screen
        break;                           //info screen
    case 8:
        box_color = graphics_make_color(0x60, 0x00, 0x00, 0xD3);
        drawBox(60, 105, 197, 20, disp); //red error screen
        break;                           //info screen
    case 9:
        box_color = graphics_make_color(0x00, 0x00, 0x00, 0xB6);
        drawBox(28, 49, 260, 150, disp);
        break; //yellow toplist
    case 10:
        box_color = graphics_make_color(0x00, 0x60, 0x00, 0xC3);
        drawBox(60, 105, 197, 20, disp); //green info screen
        break;                           //info screen
    case 11:
        box_color = graphics_make_color(0x00, 0x60, 0x00, 0xC3);
        drawBox(28, 105, 260, 30, disp);
        break; //green full filename

    default:
        break;
    }

    //restore color
    box_color = old_color;
}

//is setting the config file vars into the pconfig-structure
static int configHandler(void *user, const char *section, const char *name, const char *value)
{
    configuration *pconfig = (configuration *)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("ed64", "border_color_1"))
    {
        pconfig->border_color_1 = strdup(value);
    }
    else if (MATCH("ed64", "border_color_2"))
    {
        pconfig->border_color_2 = strdup(value);
    }
    else if (MATCH("ed64", "box_color"))
    {
        pconfig->box_color = strdup(value);
    }
    else if (MATCH("ed64", "selection_color"))
    {
        pconfig->selection_color = strdup(value);
    }
    else if (MATCH("ed64", "list_font_color"))
    {
        pconfig->list_font_color = strdup(value);
    }
    else if (MATCH("ed64", "list_dir_font_color"))
    {
        pconfig->list_dir_font_color = strdup(value);
    }
    else if (MATCH("ed64", "selection_font_color"))
    {
        pconfig->selection_font_color = strdup(value);
    }
    else if (MATCH("ed64", "mempak_path"))
    {
        pconfig->mempak_path = strdup(value);
    }
    else if (MATCH("ed64", "save_path"))
    {
        pconfig->save_path = strdup(value);
    }
    else if (MATCH("ed64", "sound_on"))
    {
        pconfig->sound_on = atoi(value);
    }
    else if (MATCH("ed64", "page_display"))
    {
        pconfig->page_display = atoi(value);
    }
    else if (MATCH("ed64", "tv_mode"))
    {
        pconfig->tv_mode = atoi(value);
    }
    else if (MATCH("ed64", "quick_boot"))
    {
        pconfig->quick_boot = atoi(value);
    }
    else if (MATCH("ed64", "enable_colored_list"))
    {
        pconfig->enable_colored_list = atoi(value);
    }
    else if (MATCH("ed64", "ext_type"))
    {
        pconfig->ext_type = atoi(value);
    }
    else if (MATCH("ed64", "cd_behaviour"))
    {
        pconfig->cd_behaviour = atoi(value);
    }
    else if (MATCH("ed64", "scroll_behaviour"))
    {
        pconfig->scroll_behaviour = atoi(value);
    }
    else if (MATCH("ed64", "text_offset"))
    {
        pconfig->text_offset = atoi(value);
    }
    else if (MATCH("ed64", "sd_speed"))
    {
        pconfig->sd_speed = atoi(value);
    }
    else if (MATCH("ed64", "hide_sysfolder"))
    {
        pconfig->hide_sysfolder = atoi(value);
    }
    else if (MATCH("ed64", "background_image"))
    {
        pconfig->background_image = strdup(value);
    }
    else if (MATCH("user", "name"))
    {
        pconfig->name = strdup(value);
    }
    else
    {
        return 0; /* unknown section/name, error */
    }

    return 1;
}

void updateFirmware(char *filename)
{ //check that firmware exists on the disk? mainly because it has to be ripped from the official image and may not have been.
    FatRecord rec_tmpf;
    if (fatFindRecord(filename, &rec_tmpf, 0) == 0)
    {
        int fpf = dfs_open(filename);
        firmware = malloc(dfs_size(fpf));
        dfs_read(firmware, 1, dfs_size(fpf), fpf);
        dfs_close(fpf);
        bi_load_firmware(firmware);
    }
}

//everdrive init functions
void configure()
{
    u16 tv;
    u16 msg = 0;
    u16 sd_mode = 0;
    u8 buff[16];
    u16 firm;

    evd_init();
    //REG_MAX_MSG
    evd_setCfgBit(ED_CFG_SDRAM_ON, 0);
    dma_read_s(buff, ROM_ADDR + 0x20, 16);
    asm_date = memRomRead32(0x38); //TODO: this should be displayed somewhere...
    evd_setCfgBit(ED_CFG_SDRAM_ON, 1);

    firm = evd_readReg(REG_VER); //TODO: why not just use evd_getFirmVersion()

    if (streql("ED64 SD boot", buff, 12) && firm >= 0x0116)
    {
        sd_mode = 1;
    }

    if (firm >= 0x0200)
    {
        sleep(1);
        evd_setCfgBit(ED_CFG_SDRAM_ON, 0);
        sleep(1);

        msg = evd_readReg(REG_MAX_MSG);

        if (!(msg & (1 << 14)))
        {
            msg |= 1 << 14;
            evd_writeReg(REG_MAX_MSG, msg);
            if (firm == 0x0214) //need to take into account different default firmware versions for each ED64 version
            {//TODO: use a case statement instead
                updateFirmware("/firmware/firmware_v2.bin");
            }
            else if (firm == 0x0250)
            {
                updateFirmware("/firmware/firmware_v2_5.bin");
            }
            else if (firm == 0x0300)
            {
                updateFirmware("/firmware/firmware_v3.bin");
            }

            sleep(1);
            evd_init();
        }
        sleep(1);

        evd_setCfgBit(ED_CFG_SDRAM_ON, 1);
    }

    if (sd_mode) //TODO: can this be moved before the firmware is loaded?
    {
        diskSetInterface(DISK_IFACE_SD);
    }
    else
    {
        diskSetInterface(DISK_IFACE_SPI);
    }
    memSpiSetDma(0);
}

//rewrites the background and the box to clear the screen
void clearScreen(display_context_t disp)
{
    drawBg(disp);           //background
    drawBoxNumber(disp, 1); //filebrowser box
}

void romInfoScreen(display_context_t disp, u8 *buff, int silent)
{
    if (silent != 1)
        sleep(10);

    u8 tmp[32];
    u8 filename[64];
    u8 ok = 0;

    sprintf(filename, "%s", buff);
    int swapped = 0;

    FatRecord rec_tmpf;
    //not needed any longer :>
    //file IS there, it's selected at this point
    ok = fatFindRecord(filename, &rec_tmpf, 0);

    u8 resp = 0;

    resp = fatOpenFileByName(filename, 0); //err if not found ^^

    int mb = file.sec_available / 2048;
    int block_offset = 0;
    u32 cart_buff_offset = 0;
    u32 begin_sector = file.sector;

    //filesize -> readfile / 512
    int fsize = 512;                 //rom-headersize 4096 but the bootcode is not needed
    unsigned char headerdata[fsize]; //1*512

    resp = fatReadFile(&headerdata, fsize / 512); //1 cluster

    int sw_type = is_valid_rom(headerdata);

    if (sw_type != 0)
    {
        swapped = 1;
        swap_header(headerdata, 512);
    }

    //char 32-51 name
    unsigned char rom_name[32];

    for (int u = 0; u < 19; u++)
    {
        if (u != 0)
            sprintf(rom_name, "%s%c", rom_name, headerdata[32 + u]);
        else
            sprintf(rom_name, "%c", headerdata[32 + u]);
    }
    //trim right spaces
    //romname=trimmed rom name for filename
    sprintf(rom_name, "%s", trim(rom_name));

    if (silent != 1)
        printText(rom_name, 11, 19, disp);

    sprintf(rom_name, "Size: %iMb", mb);

    if (silent != 1)
        printText(rom_name, 11, -1, disp);

    //unique cart id for gametype
    unsigned char cartID_str[12];
    sprintf(cartID_str, "ID: %c%c%c%c", headerdata[0x3B], headerdata[0x3C], headerdata[0x3D], headerdata[0x3E]);

    if (silent != 1)
        printText(cartID_str, 11, -1, disp);

    int cic, save;

    cic = get_cic(&headerdata[0x40]);

    unsigned char cartID_short[4];
    sprintf(cartID_short, "%c%c\0", headerdata[0x3C], headerdata[0x3D]);

    if (get_cic_save(cartID_short, &cic, &save))
    {
        if (silent != 1)
            printText("found in db", 11, -1, disp);
        unsigned char save_type_str[12];
        sprintf(save_type_str, "Save: %s", saveTypeToExtension(save, ext_type));
        if (silent != 1)
            printText(save_type_str, 11, -1, disp);

        unsigned char cic_type_str[12];
        sprintf(cic_type_str, "CIC: CIC-610%i", cic);
        if (silent != 1)
            printText(cic_type_str, 11, -1, disp);

        //thanks for the db :>
        //cart was found, use CIC and SaveRAM type
    }

    if (silent != 1)
    {
        char box_path[32];

        sprite_t *n64cover;

        sprintf(box_path, "/ED64/boxart/lowres/%c%c.png", headerdata[0x3C], headerdata[0x3D]);

        if (fatFindRecord(box_path, &rec_tmpf, 0) != 0)
        {
            //not found
            sprintf(box_path, "/ED64/boxart/lowres/00.png");
        }

        n64cover = loadPng(box_path);
        graphics_draw_sprite(disp, 81, 32, n64cover);
        display_show(disp);
    }
    else
    {
        rom_config[1] = cic - 1;
        rom_config[2] = save;
        rom_config[3] = 0; //tv force off
        rom_config[4] = 0; //cheat off
        rom_config[5] = 0; //chk_sum off
        rom_config[6] = 0; //rating
        rom_config[7] = 0; //country
        rom_config[8] = 0; //reserved
        rom_config[9] = 0; //reserved
    }
}

sprite_t *loadPng(u8 *png_filename)
{
    u8 *filename;
    u8 ok = 0;

    filename = (u8 *)malloc(slen(png_filename));
    //config filename

    sprintf(filename, "%s", png_filename);
    FatRecord rec_tmpf;
    ok = fatFindRecord(filename, &rec_tmpf, 0);

    u8 resp = 0;
    resp = fatOpenFileByName(filename, 0);

    //filesize of the opend file -> is the readfile / 512
    int fsize = file.sec_available * 512;

    u8 png_rawdata[fsize];

    resp = fatReadFile(&png_rawdata, fsize / 512); //1 cluster

    return loadImage32(png_rawdata, fsize);

    free(filename);
}

void loadgbrom(display_context_t disp, u8 *buff)
{
    FatRecord rec_tmpf;

    if (fatFindRecord("/ED64/gblite.z64", &rec_tmpf, 0) == 0) //filename already exists?
    {
        u8 gb_sram_file[64];

        u8 resp = 0;

        sprintf(gb_sram_file, "/ED64/%s/gblite.SRM", save_path);

        resp = fatFindRecord(gb_sram_file, &rec_tmpf, 0); //filename already exists
        resp = fatCreateRecIfNotExist(gb_sram_file, 0);
        resp = fatOpenFileByName(gb_sram_file, 32768 / 512);

        static uint8_t sram_buffer[36928];

        for (int i = 0; i < 36928; i++)
            sram_buffer[i] = 0;

        sprintf(sram_buffer, buff);

        resp = fatWriteFile(&sram_buffer, 32768 / 512);

        while (!(disp = display_lock()))
            ;

        sprintf(rom_filename, "gblite");
        gbload = 1;

        loadrom(disp, "/ED64/gblite.z64", 1);
    }
}

void loadmsx2rom(display_context_t disp, u8 *rom_path)
{
    //max 128kb rom
    int max_ok = fatOpenFileByName(rom_path, 0);
    int fsize = file.sec_available * 512; //fsize in bytes

    if (fsize > 128 * 1024)
    {
        //error

        drawShortInfoBox(disp, "  error: rom > 128kB", 1);
        input_mapping = abort_screen;

        return;
    }
    else
    {
        drawShortInfoBox(disp, " loading please wait", 0);
        input_mapping = none; //disable all
    }

    FatRecord rec_tmpf;
    if (fatFindRecord("/ED64/ultraMSX2.z64", &rec_tmpf, 0) == 0) //file exists?
    {
        u8 resp = 0;
        //load nes emulator
        resp = fatOpenFileByName("/ED64/ultraMSX2.z64", 0); //err if not found ^^

        int fsize = 1024 * 1024;
        u8 buffer[fsize];

        //injecting in buffer... slow but working :/
        resp = fatReadFile(buffer, file.sec_available);
        resp = fatOpenFileByName(rom_path, 0); //err if not found ^^
        resp = fatReadFile(buffer + 0x2df48, file.sec_available);
        dma_write_s(buffer, 0xb0000000, fsize);

        boot_cic = 2;  //cic 6102
        boot_save = 0; //save off/cpak
        force_tv = 0;  //no force
        cheats_on = 0; //cheats off
        checksum_fix_on = 0;

        checksum_sdram();

        bootRom(disp, 1);
    }
}

void loadggrom(display_context_t disp, u8 *rom_path)
{
    //max 512kb rom
    int max_ok = fatOpenFileByName(rom_path, 0);
    int fsize = file.sec_available * 512; //fsize in bytes

    if (fsize > 512 * 1024)
    {
        //error
        drawShortInfoBox(disp, "  error: rom > 512kB", 1);
        input_mapping = abort_screen;

        return;
    }
    else
    {
        drawShortInfoBox(disp, " loading please wait", 0);
        input_mapping = none; //disable all
    }

    FatRecord rec_tmpf;
    if (fatFindRecord("/ED64/UltraSMS.z64", &rec_tmpf, 0) == 0) //file exists?
    {
        u8 resp = 0;
        //load nes emulator
        resp = fatOpenFileByName("/ED64/UltraSMS.z64", 0); //err if not found ^^

        int fsize = 1024 * 1024;
        u8 buffer[fsize];

        //injecting in buffer... slow but working :/
        resp = fatReadFile(buffer, file.sec_available);
        resp = fatOpenFileByName(rom_path, 0); //err if not found ^^
        resp = fatReadFile(buffer + 0x1b410, file.sec_available);
        dma_write_s(buffer, 0xb0000000, fsize);

        boot_cic = 2;  //cic 6102
        boot_save = 0; //save off/cpak
        force_tv = 0;  //no force
        cheats_on = 0; //cheats off
        checksum_fix_on = 0;

        checksum_sdram();

        bootRom(disp, 1);
    }
}

void rom_load_y(void)
{
    FatRecord rec_tmpf;

    u8 gb_sram_file[64];
    u8 gb_sram_file2[64];
    sprintf(gb_sram_file, "%c%c%c%c%c%c%c", 'O', 'S', '6', '4', 'P', '/', 'O');
    sprintf(gb_sram_file2, "%s%c%c%c%c%c%c%c%c", gb_sram_file, 'S', '6', '4', 'P', '.', 'v', '6', '4');

    if (fatFindRecord(gb_sram_file2, &rec_tmpf, 0) == 0) //filename already exists?
    {
        gb_load_y = 1;
    }
}

void loadnesrom(display_context_t disp, u8 *rom_path)
{
    FatRecord rec_tmpf;

    if (fatFindRecord("/ED64/neon64bu.rom", &rec_tmpf, 0) == 0) //filename already exists?
    {
        u8 resp = 0;
        //load nes emulator
        resp = fatOpenFileByName("/ED64/neon64bu.rom", 0); //err if not found ^^
        resp = diskRead(file.sector, (void *)0xb0000000, file.sec_available);

        //load nes rom
        resp = fatOpenFileByName(rom_path, 0); //err if not found ^^
        resp = diskRead(file.sector, (void *)0xb0200000, file.sec_available);

        boot_cic = 2;  //cic 6102
        boot_save = 0; //save off/cpak
        force_tv = 0;  //no force
        cheats_on = 0; //cheats off
        checksum_fix_on = 0;

        bootRom(disp, 1);
    }
}

//load a z64/v64/n64 rom to the sdram
void loadrom(display_context_t disp, u8 *buff, int fast)
{
    clearScreen(disp);
    display_show(disp);

    if (!fast)
        printText("Restoring:", 3, 4, disp);

    //sleep(1000); //needless waiting :>

    TRACE(disp, "timing done");

    u8 tmp[32];
    u8 filename[64];
    u8 ok = 0;

    sprintf(filename, "%s", buff);
    int swapped = 0;

    TRACE(disp, buff);

    FatRecord rec_tmpf;
    //not needed any longer :>
    //file IS there, it's selected at this point
    ok = fatFindRecord(filename, &rec_tmpf, 0);

    TRACE(disp, "found");

    u8 resp = 0;

    resp = fatOpenFileByName(filename, 0); //err if not found ^^

    TRACE(disp, "opened");

    int mb = file.sec_available / 2048;
    int file_sectors = file.sec_available;
    int block_offset = 0;
    u32 cart_buff_offset = 0;
    u32 begin_sector = file.sector;

    //filesize -> readfile / 512
    int fsize = 512;                 //rom-headersize 4096 but the bootcode is not needed
    unsigned char headerdata[fsize]; //1*512

    resp = fatReadFile(&headerdata, fsize / 512); //1 cluster

    int sw_type = is_valid_rom(headerdata);

    if (sw_type != 0)
    {
        if (!fast)
            printText("byteswapped file", 3, -1, disp);

        swapped = 1;
        swap_header(headerdata, 512);
    }

    //char 32-51 name
    unsigned char rom_name[32];

    for (int u = 0; u < 19; u++)
    {
        if (u != 0)
            sprintf(rom_name, "%s%c", rom_name, headerdata[32 + u]);
        else
            sprintf(rom_name, "%c", headerdata[32 + u]);
    }
    //trim right spaces
    //romname=trimmed rom name for filename
    sprintf(rom_name, "%s", trim(rom_name));

    if (!fast)
        printText(rom_name, 3, -1, disp);

    //unique cart id for gametype

    sprintf(cartID, "%c%c%c%c", headerdata[0x3B], headerdata[0x3C], headerdata[0x3D], headerdata[0x3E]);

    int cic, save;

    cic = get_cic(&headerdata[0x40]);

    unsigned char cartID_short[4];
    sprintf(cartID_short, "%c%c\0", headerdata[0x3C], headerdata[0x3D]);

    if (get_cic_save(cartID_short, &cic, &save))
    {
        if (!fast)
            printText("found in db", 3, -1, disp);
        //thanks for the db :>
        // cart was found, use CIC and SaveRAM type
    }

    TRACEF(disp, "Info: cic=%i save=%i", cic, save);

    //new rom_config
    boot_cic = rom_config[1] + 1;
    boot_save = rom_config[2];
    force_tv = rom_config[3];
    cheats_on = rom_config[4];
    checksum_fix_on = rom_config[5];
    boot_country = rom_config[7]; //boot_block

    if (gbload == 1)
        boot_save = 1;

    else if (resp)
    {
        sprintf(tmp, "Response: %i", resp);
        printText(tmp, 3, -1, disp);
    }

    TRACE(disp, "Checking SD mode");

    resp = evd_isSDMode();

    TRACEF(disp, "SD mode: %i", resp);
    TRACEF(disp, "Size: %i", file.sec_available);
    TRACEF(disp, "File sector: %i", file.sector);
    TRACE(disp, "Loading:");

    sleep(10);

    if (swapped == 1)
    {
        while (evd_isDmaBusy())
            ;
        sleep(400);
        evd_mmcSetDmaSwap(1);

        TRACE(disp, "swapping on");

        sleep(10);
    }

    if (!fast)
        printText("loading please wait...", 3, -1, disp);
    else
        printText("loading please wait...", 3, 4, disp);

    sleep(10);

    int lower_half = 2048 * 32;

    if (mb <= 32)
    {
        resp = diskRead(begin_sector, (void *)0xb0000000, file_sectors); //2048 cluster 1Mb
    }
    else
    {
        resp = diskRead(begin_sector, (void *)0xb0000000, lower_half);
        resp = diskRead(begin_sector + lower_half, (void *)0xb2000000, file_sectors - lower_half);
    }

    if (resp)
    {
        TRACEF(disp, "mmcToCart: %i", resp);
    }

    //if (debug) {
    for (int i = 0; i < 4; i++)
    {
        u8 buff[16];
        dma_read_s(buff, 0xb0000000 + 0x00100000 * i, 1);
        TRACEF(disp, "probe: %hhx", buff[0]);
    }
    //}

    if (!fast)
    {
        sleep(200);

        printText(" ", 3, -1, disp);
        printText("(C-UP to activate cheats)", 3, -1, disp);
        printText("(C-RIGHT to force menu tv mode)", 3, -1, disp);
        printText("done: PRESS START", 3, -1, disp);
    }
    else
    {
        bootRom(disp, 1);
    }
}

int backupSaveData(display_context_t disp)
{
    //backup cart-save on sd after reboot
    u8 config_file_path[32];
    int save_format;

    sprintf(config_file_path, "/ED64/%s/LAST.CRT", save_path);
    uint8_t cfg_data[512]; //TODO: this should be a strut

    FatRecord rec_tmpf;

    printText("Saving last game session...", 3, 4, disp);

    if (fatFindRecord(config_file_path, &rec_tmpf, 0) == 0) //file exists?
    {
        //file to cfg_data buffer
        u8 resp = 0;
        fatOpenFileByName(config_file_path, 0);
        fatReadFile(&cfg_data, 1);

        //split in save type and cart-id
        save_format = cfg_data[0];
        int last_cic = cfg_data[1];
        scopy(cfg_data + 2, rom_filename); //string copy

        //set savetype to 0 disable for next boot
        if (save_format != 0)
        {
            //set savetype to off
            cfg_data[0] = 0;

            u8 tmp[32];

            resp = fatOpenFileByName(config_file_path, 1); //if sector is set filemode=WR writeable

            TRACEF(disp, "FAT_OpenFileByName returned: %i", resp);

            resp = fatWriteFile(&cfg_data, 1); //filemode must be wr

            TRACE(disp, "Disabling save for subsequent system reboots");
            TRACEF(disp, "FAT_WriteFile returned: %i", resp);

            volatile u8 save_config_state = 0;
            evd_readReg(0);
            save_config_state = evd_readReg(REG_SAV_CFG);

            if (save_config_state != 0 || evd_getFirmVersion() >= 0x0300)
            { //save register set or the firmware is V3
                if (save_config_state == 0)
                {                                 //we are V3 and have had a hard reboot
                    evd_writeReg(REG_SAV_CFG, 1); //so we need to tell the save register it still has data.
                }
                save_after_reboot = 1;
            }
        }
        else
        {
            TRACE(disp, "Save not required.");
            printText("...ready", 3, -1, disp);

            sleep(200);

            return 1;
        }
    }
    else
    {
        TRACE(disp, "No previous ROM loaded - the file 'last.crt' was not found!");
        printText("...ready", 3, -1, disp);

        return 0;
    }

    //if (debug) {
    //    sleep(5000);
    //}

    //reset with save request
    if (save_after_reboot)
    {
        printText("Copying RAM to SD card...", 3, -1, disp);
        if (saveTypeToSd(disp, rom_filename, save_format))
        {
            printText("Operation completed sucessfully...", 3, -1, disp);
        }
        else
        {
            TRACE(disp, "ERROR: the RAM was not successfully saved!");
        }
    }
    else
    {
        TRACE(disp, "no reset - save request");
        printText("...ready", 3, -1, disp);

        sleep(300);
    }

    return 1;
}

//old method to write a file to the mempak at controller 1
void file_to_mpk(display_context_t disp, u8 *filename)
{
    u8 tmp[32];
    u8 buff[64];
    u8 ok = 0;

    printText(filename, 9, -1, disp);
    FatRecord rec_tmpf;
    ok = fatFindRecord(filename, &rec_tmpf, 0);

    u8 resp = 0;
    resp = fatOpenFileByName(filename, 0);

    u8 *pch;
    pch = strrchr(filename, '.');
    sprintf(buff, "%s", (pch + 2));

    if (strcmp(buff, "64") == 0)
    {
        printText("Dexdrive format", 9, -1, disp);
        printText("skip header", 9, -1, disp);

        static uint8_t mempak_data_buff[36928];
        resp = fatReadFile(&mempak_data_buff, 36928 / 512);

        memcpy(&mempak_data, mempak_data_buff + 4160, 32768);
    }
    else
    {
        printText("Z64 format", 9, -1, disp);
        resp = fatReadFile(&mempak_data, 32768 / 512);
    }

    int err = 0;
    for (int j = 0; j < 128; j++)
    {
        err |= write_mempak_sector(0, j, &mempak_data[j * MEMPAK_BLOCK_SIZE]);
    }
}

char TranslateNotes(char *bNote, char *Text)
{
#pragma warning(disable : 4305 4309)
    char cReturn = 0x00;
    const char aSpecial[] = {0x21, 0x22, 0x23, 0x60, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x3A, 0x3D, 0x3F, 0x40, 0x74, 0xA9, 0xAE};
//    { '!' , '\"', '#' , '`' , '*' , '+' , ',' , '-' , '.' , '/' , ':' , '=' , '?' , '>' , 'tm', '(r)','(c)' };
#pragma warning(default : 4305 4309)
    int i = 16;
    do
    {
        char b = bNote[i];
        if ((b > 0) && i < 32)
        {
            if (b <= 0x0F) // translate Icons as Spaces
                *Text = 0x20;
            else if (b <= 0x19) // Numbers
                *Text = 0x20 + b;
            else if (b <= 0x33) // Characters
                *Text = 0x47 + b;
            else if (b <= 0x44) // special Symbols
                *Text = aSpecial[b - 0x34];
            else if (b <= 0x94) // Japan
                *Text = 0xC0 + (b % 40);
            else // unknown
                *Text = (char)0xA4;

            ++i;
            ++Text;
        }
        else
        {
            *Text = '\0';
            if (b)
            {
                i = 12;
                Text = &cReturn;
            }
            else
                i = 13;
        }
    } while (i != 13);

    return cReturn;
}

char CountBlocks(char *bMemPakBinary, char *aNoteSizes)
{
    int wRemainingBlocks = 123;
    char bNextIndex;
    int i = 0;
    while (i < 16 && wRemainingBlocks <= 123)
    {
        aNoteSizes[i] = 0;
        bNextIndex = bMemPakBinary[0x307 + (i * 0x20)];
        while ((bNextIndex >= 5) && (aNoteSizes[i] < wRemainingBlocks))
        {
            aNoteSizes[i]++;
            bNextIndex = bMemPakBinary[0x101 + (bNextIndex * 2)];
        }

        if (aNoteSizes[i] > wRemainingBlocks)
            wRemainingBlocks = 0xFF;
        else
            wRemainingBlocks -= aNoteSizes[i];

        i++;
    }
    return wRemainingBlocks;
}

void view_mpk_file(display_context_t disp, char *mpk_filename)
{
    u8 tmp[32];
    u8 buff[64];
    u8 ok = 0;

    FatRecord rec_tmpf;
    ok = fatFindRecord(mpk_filename, &rec_tmpf, 0);

    u8 resp = 0;
    resp = fatOpenFileByName(mpk_filename, 0);

    u8 *pch;
    pch = strrchr(mpk_filename, '.');
    sprintf(buff, "%s", (pch + 2));

    if (strcmp(buff, "64") == 0)
    {
        static uint8_t mempak_data_buff[36928];
        resp = fatReadFile(&mempak_data_buff, 36928 / 512);
        memcpy(&mempak_data, mempak_data_buff + 4160, 32768);
    }
    else
    {
        resp = fatReadFile(&mempak_data, 32768 / 512);
    }

    printText("File content:", 11, 5, disp);
    printText("   ", 11, -1, disp);

    int notes_c = 0;

    char szBuffer[40],
        cAppendix;
    int bFirstChar;

    int i = 0,
        nNotes = 0,
        iSum = 0,
        iRemainingBlocks;

    char aNoteSizes[16];

    for (i = 0x10A; i < 0x200; i++)
        iSum += mempak_data[i];

    if (((iSum % 256) == mempak_data[0x101]))
    {
        iRemainingBlocks = CountBlocks(mempak_data, aNoteSizes);

        if (iRemainingBlocks <= 123)
        {
            for (notes_c = 0; notes_c < 16; notes_c++)
            {
                if (mempak_data[0x300 + (notes_c * 32)] ||
                    mempak_data[0x301 + (notes_c * 32)] ||
                    mempak_data[0x302 + (notes_c * 32)])
                {
                    cAppendix = TranslateNotes(&mempak_data[0x300 + (notes_c * 32)], szBuffer);

                    if (cAppendix != '\0')
                        sprintf(szBuffer, "%s. %c", szBuffer, cAppendix);

                    bFirstChar = 1;
                    for (i = 0; i < (int)strlen(szBuffer); i++)
                    {
                        if (szBuffer[i] == ' ')
                            bFirstChar = 1;
                        else
                        {
                            if (bFirstChar && (szBuffer[i] >= 'a') && (szBuffer[i] <= 'z'))
                            {
                                bFirstChar = 0;
                                szBuffer[i] -= 0x20;
                            }
                        }
                    }
                    printText(szBuffer, 11, -1, disp);

                    switch (mempak_data[0x303 + (notes_c * 32)])
                    {
                    case 0x00:
                        sprintf(szBuffer, "None");
                        break;
                    case 0x37:
                        sprintf(szBuffer, "Beta");
                        break;
                    case 0x41:
                        sprintf(szBuffer, "NTSC");
                        break;
                    case 0x44:
                        sprintf(szBuffer, "Germany");
                        break;
                    case 0x45:
                        sprintf(szBuffer, "USA");
                        break;
                    case 0x46:
                        sprintf(szBuffer, "France");
                        break;
                    case 0x49:
                        sprintf(szBuffer, "Italy");
                        break;
                    case 0x4A:
                        sprintf(szBuffer, "Japan");
                        break;
                    case 0x50:
                        sprintf(szBuffer, "Europe");
                        break;
                    case 0x53:
                        sprintf(szBuffer, "Spain");
                        break;
                    case 0x55:
                        sprintf(szBuffer, "Australia");
                        break;
                    case 0x58:
                    case 0x59:
                        sprintf(szBuffer, "PAL");
                        break;
                    default:
                        sprintf(szBuffer, "Unknown(%02X)", mempak_data[0x303 + (notes_c * 32)]);
                    }

                    sprintf(szBuffer, "%i", aNoteSizes[notes_c]);
                    nNotes++;
                }
            }
        }

        int free_c = 0;
        for (free_c = nNotes; free_c < 16; free_c++)
            printText("[free]", 11, -1, disp);

        char buff[512];
        printText("   ", 11, -1, disp);
        printText("Free space:", 11, -1, disp);
        sprintf(buff, "%i blocks", iRemainingBlocks);
        printText(buff, 11, -1, disp);
    }
    else
    {
        printText("empty", 11, -1, disp);
    }
}

void view_mpk(display_context_t disp)
{
    int err;

    printText("Mempak content:", 11, 5, disp);
    get_accessories_present();

    /* Make sure they don't have a rumble pak inserted instead */
    switch (identify_accessory(0))
    {
    case ACCESSORY_NONE:

        printText(" ", 11, -1, disp);
        printText("no Mempak", 11, -1, disp);
        break;

    case ACCESSORY_MEMPAK:
        if ((err = validate_mempak(0)))
        {
            if (err == -3)
            {
                printText(" ", 11, -1, disp);

                printText("not formatted", 11, -1, disp);
            }
            else
            {
                printText(" ", 11, -1, disp);
                printText("read error", 11, -1, disp);
            }
        }
        else
        {
            printText("   ", 11, -1, disp);
            for (int j = 0; j < 16; j++)
            {
                entry_structure_t entry;

                get_mempak_entry(0, j, &entry);

                if (entry.valid)
                {
                    char tmp[512];
                    sprintf(tmp, "%s", entry.name);
                    printText(tmp, 11, -1, disp);
                }
                else
                {
                    printText("[free]", 11, -1, disp);
                }
            }

            char buff[512];
            printText("   ", 11, -1, disp);
            printText("Free space:", 11, -1, disp);
            sprintf(buff, "%d blocks", get_mempak_free_space(0));
            printText(buff, 11, -1, disp);
        }
        break;

    case ACCESSORY_RUMBLEPAK:
        printText("RumblePak inserted", 11, -1, disp);
        break;

    default:
        break;
    }
}

//old function to dump a mempak to a file
void mpk_to_file(display_context_t disp, char *mpk_filename, int quick)
{
    u8 buff[64];
    u8 v = 0;
    u8 ok = 0;

    if (quick)
        sprintf(buff, "%s%s", mempak_path, mpk_filename);
    else
        sprintf(buff, "%s%s.MPK", mempak_path, mpk_filename);

    FatRecord rec_tmpf;

    if (!fatFindRecord(buff, &rec_tmpf, 0))
    { //filename already exists
        printText("File exists", 9, -1, disp);
        if (quick)
            printText("override", 9, -1, disp);
        else
            while (ok == 0)
            {
                sprintf(buff, "%s%s%i.MPK", mempak_path, mpk_filename, v);

                ok = fatFindRecord(buff, &rec_tmpf, 0);
                if (ok == 0)
                    v++;
                else
                    break;
            }
    }

    u8 resp = 0;
    resp = fatCreateRecIfNotExist(buff, 0);
    resp = fatOpenFileByName(buff, 32768 / 512);

    controller_init();

    int err = 0;
    for (int j = 0; j < 128; j++)
    {
        err |= read_mempak_sector(0, j, &mempak_data[j * 256]);
    }

    fatWriteFile(&mempak_data, 32768 / 512);

    sleep(500);

    sprintf(buff, "File: %s%i.MPK", mpk_filename, v);

    printText(buff, 9, -1, disp);
    printText("backup done...", 9, -1, disp);
}

//before boot_simulation
//writes a cart-save from a file to the fpga/cart
int saveTypeFromSd(display_context_t disp, char *rom_name, int stype)
{
    rom_load_y();

    u8 tmp[32];
    u8 fname[128];
    u8 found = 0;

    int size;
    size = saveTypeToSize(stype); // int byte

    sprintf(fname, "/ED64/%s/%s.%s", save_path, rom_name, saveTypeToExtension(stype, ext_type));

    uint8_t cartsave_data[size];

    //if (debug) {
    TRACE(disp, fname);
    //sleep(2000);
    //}

    FatRecord rec_tmpf;
    found = fatFindRecord(fname, &rec_tmpf, 0);

    TRACEF(disp, "fatFindRecord returned: %i", found);

    if (found == 0)
    {
        u8 resp = 0;
        resp = fatOpenFileByName(fname, 0);

        TRACEF(disp, "fatOpenFileByName returned: %i", resp);

        resp = fatReadFile(cartsave_data, size / 512);

        TRACEF(disp, "fatReadFile returned: %i", resp);
    }
    else
    {
        printText("no savegame found", 3, -1, disp);
        //todo clear memory area

        return 0;
    }

    if (gb_load_y != 1)
    {
        if (pushSaveToCart(stype, cartsave_data))
        {

            printText("save upload done...", 3, -1, disp);
        }
        else
        {

            printText("pushSaveToCart error", 3, -1, disp);
        }
    }

    return 1;
}

int saveTypeToSd(display_context_t disp, char *rom_name, int stype)
{
    rom_load_y();

    //after reset create new savefile
    u8 tmp[32];
    u8 fname[128]; //filename buffer to small :D
    u8 found = 0;

    int size;
    size = saveTypeToSize(stype); // int byte

    TRACEF(disp, "size for save=%i", size);

    sprintf(fname, "/ED64/%s/%s.%s", save_path, rom_name, saveTypeToExtension(stype, ext_type));

    FatRecord rec_tmpf;
    found = fatFindRecord(fname, &rec_tmpf, 0);

    TRACEF(disp, "found=%i", found);

    u8 resp = 0;

    //FAT_ERR_NOT_EXIST 100
    if (found != 0)
    {
        //create before save
        printText("try fatCreateRecIfNotExist", 3, -1, disp);
        resp = fatCreateRecIfNotExist(fname, 0);

        TRACEF(disp, "fatCreateRecIfNotExist returned: %i", resp); //0 means try to create
    }

    //open file with stype size
    resp = fatOpenFileByName(fname, size / 512);

    TRACEF(disp, "fatOpenFileByName returned: %i", resp); //100 not exist

    //for savegame
    uint8_t cartsave_data[size];

    for (int zero = 0; zero < size; zero++)
        cartsave_data[zero] = 0;

    TRACEF(disp, "cartsave_data=%p", &cartsave_data);

    //universal dumpfunction
    //returns data from fpga/cart to save on sd

    if (getSaveFromCart(stype, cartsave_data))
    {
        printText("got save from fpga", 3, -1, disp);
        //write to file

        if (gb_load_y != 1)
            fatWriteFile(&cartsave_data, size / 512);

        printText("reset-save done...", 3, -1, disp);
        sleep(3000);
        return 1;
    }
    else
    {
        printText("getSaveFromCart error", 3, -1, disp);
        sleep(3000);
        return 0;
    }
}

//check out the userfriendly ini file for config-information

int readConfigFile(void)
{
    //var file readin
    u8 tmp[32];
    u8 filename[32];
    u8 ok = 0;

    //config filename
    sprintf(filename, "/ED64/ALT64.INI");
    FatRecord rec_tmpf;
    ok = fatFindRecord(filename, &rec_tmpf, 0);

    u8 resp = 0;
    resp = fatOpenFileByName(filename, 0);

    //filesize of the opend file -> is the readfile / 512
    int fsize = file.sec_available * 512;
    char config_rawdata[fsize];

    resp = fatReadFile(&config_rawdata, fsize / 512); //1 cluster
    configuration config;

    if (ini_parse_str(config_rawdata, configHandler, &config) < 0)
    {
        return 0;
    }
    else
    {
        border_color_1_s = config.border_color_1;
        border_color_2_s = config.border_color_2;
        box_color_s = config.box_color;
        selection_color_s = config.selection_color;
        selection_font_color_s = config.selection_font_color;
        list_font_color_s = config.list_font_color;
        list_dir_font_color_s = config.list_dir_font_color;

        mempak_path = config.mempak_path;
        save_path = config.save_path;
        sound_on = config.sound_on;
        page_display = config.page_display;
        tv_mode = config.tv_mode;
        quick_boot = config.quick_boot;
        enable_colored_list = config.enable_colored_list;
        ext_type = config.ext_type;
        cd_behaviour = config.cd_behaviour;
        scroll_behaviour = config.scroll_behaviour;
        text_offset = config.text_offset;
        hide_sysfolder = config.hide_sysfolder;
        sd_speed = config.sd_speed;
        background_image = config.background_image;

        return 1;
    }
}

int str2int(char data)
{
    data -= '0';
    if (data > 9)
        data -= 7;

    return data;
}

uint32_t translate_color(char *hexstring)
{
    int r = str2int(hexstring[0]) * 16 + str2int(hexstring[1]);
    int g = str2int(hexstring[2]) * 16 + str2int(hexstring[3]);
    int b = str2int(hexstring[4]) * 16 + str2int(hexstring[5]);
    int a = str2int(hexstring[6]) * 16 + str2int(hexstring[7]);

    return graphics_make_color(r, g, b, a);
}

//init fat filesystem after everdrive init and before sdcard access
void initFilesystem(void)
{
    sleep(1000);
    evd_ulockRegs();
    sleep(1000);

    fatInitRam();
    fatInit();
    fat_initialized = 1;
}

//prints the sdcard-filesystem content
void readSDcard(display_context_t disp, char *directory)
{
    //todo: check out the minimal sleeping needs
    //color test

    FatRecord *frec;
    u8 cresp = 0;
    count = 1;

    //load the directory-entry
    cresp = fatLoadDirByName("/ED64/CFG");

    int dsize = dir->size;
    char colorlist[dsize][256];

    if (enable_colored_list)
    {
        for (int i = 0; i < dir->size; i++)
        {
            frec = dir->rec[i];
            u8 rom_cfg_file[128];

            //set rom_cfg
            sprintf(rom_cfg_file, "/ED64/CFG/%s", frec->name);

            static uint8_t cfg_file_data[512] = {0};
            cresp = fatOpenFileByName(rom_cfg_file, 0); //512 bytes fix one cluster
            cresp = fatReadFile(&cfg_file_data, 1);

            colorlist[i][0] = (char)cfg_file_data[5];     //color
            strcpy(colorlist[i] + 1, cfg_file_data + 32); //fullpath
        }
    }

    clearScreen(disp);
    printText("SD-Card loading...", 3, 4, disp); //very short display time maybe comment out

    u8 buff[32];

    //some trash buffer
    FatRecord *rec;
    u8 resp = 0;

    count = 1;
    dir_t buf;

    //load the directory-entry
    resp = fatLoadDirByName(directory);

    if (resp != 0)
    {
        char error_msg[32];
        sprintf(error_msg, "CHDIR ERROR: %i", resp);
        printText(error_msg, 3, -1, disp);
        sleep(3000);
    }

    //clear screen and print the directory name
    clearScreen(disp);

    //creates string list of files and directories
    for (int i = 0; i < dir->size; i++)
    {
        char name_tmpl[32];
        rec = dir->rec[i];

        if (strcmp(rec->name, "ED64") == 0 && hide_sysfolder == 1)
        {
            //don't add
        }
        else
        {
            if (rec->is_dir)
            {
                list[count - 1].type = DT_DIR; //2 is dir +1
            }
            else
            {
                list[count - 1].type = DT_REG; //1 is file +1
            }

            strcpy(list[count - 1].filename, rec->name); //+1

            //new color test
            list[count - 1].color = 0;

            if (enable_colored_list)
            {
                for (int c = 0; c < dsize; c++)
                {

                    u8 short_name[256];

                    sprintf(short_name, "%s", colorlist[c] + 1);

                    u8 *pch_s; // point-offset
                    pch_s = strrchr(short_name, '/');

                    if (strcmp(list[count - 1].filename, pch_s + 1) == 0)
                    {

                        list[count - 1].color = colorlist[c][0];
                    }
                }
                //new color test end
            }

            count++;
            list = realloc(list, sizeof(direntry_t) * count);
        }
    }
    count--;

    page = 0;
    cursor = 0;
    select_mode = 1;

    if (count > 0)
    {
        /* Should sort! */
        qsort(list, count, sizeof(direntry_t), compare);
    }

    //print directory
    display_dir(list, cursor, page, MAX_LIST, count, disp);
}

/*
 * Returns two cheat lists:
 * - One for the "at boot" cheats
 * - Another for the "in-game" cheats
 */
int readCheatFile(char *filename, u32 *cheat_lists[2])
{
    // YAML parser
    yaml_parser_t parser;
    yaml_event_t event;

    // State for YAML parser
    int is_code = 0;
    int code_on = 1;
    int done = 0;
    u32 *list1;
    u32 *list2;
    char *next;

    int repeater = 0;
    u32 address;
    u32 value;

    yaml_parser_initialize(&parser);

    FatRecord rec_tmpf;

    if (fatFindRecord(filename, &rec_tmpf, 0) != 0)
    {
        return -1; //err file not found
    }

    u8 resp = 0;
    resp = fatOpenFileByName(filename, 0);

    //filesize of the opend file -> is the readfile / 512
    int fsize = file.sec_available * 512;
    char *cheatfile = malloc(fsize);
    if (!cheatfile)
    {
        return -2; // Out of memory
    }

    /*
     * Size of the cheat list can never be more than half the size of the YAML
     * Minimum YAML example:
     *   A:-80001234 FFFF
     * Which is exactly 16 bytes.
     * The cheat list in this case fits into exactly 8 bytes (2 words):
     *   0x80001234, 0x0000FFFF
     */
    list1 = calloc(1, fsize + 2 * sizeof(u32)); // Plus 2 words to be safe
    if (!list1)
    {
        // Free
        free(cheatfile);
        return -2; // Out of memory
    }
    list2 = &list1[fsize / sizeof(u32) / 2];
    cheat_lists[0] = list1;
    cheat_lists[1] = list2;

    resp = fatReadFile(cheatfile, fsize / 512); //1 cluster

    yaml_parser_set_input_string(&parser, cheatfile, strlen(cheatfile));

    do
    {
        if (!yaml_parser_parse(&parser, &event))
        {
            // Free
            yaml_parser_delete(&parser);
            yaml_event_delete(&event);
            free(cheatfile);
            free(cheat_lists[0]);
            cheat_lists[0] = 0;
            cheat_lists[1] = 0;

            return -3; // Parse error
        }

        // Process YAML
        switch (event.type)
        {
        case YAML_MAPPING_START_EVENT:
            // Begin code block
            is_code = 0;
            break;

        case YAML_SEQUENCE_START_EVENT:
            // Begin code lines
            is_code = 1;
            code_on = (event.data.sequence_start.tag ? !!strcasecmp(event.data.sequence_start.tag, "!off") : 1);
            break;

        case YAML_SEQUENCE_END_EVENT:
            // End code lines
            is_code = 0;
            code_on = 1;
            repeater = 0;
            break;

        case YAML_SCALAR_EVENT:
            // Code line
            if (!is_code || !code_on)
            {
                break;
            }

            address = strtoul(event.data.scalar.value, &next, 16);
            value = strtoul(next, NULL, 16);

            // Do not check code types within "repeater data"
            if (repeater)
            {
                repeater--;
                *list2++ = address;
                *list2++ = value;
                break;
            }

            // Determine destination cheat_list for the code type
            switch (address >> 24)
            {

            // Uncessary code types
            case 0x20: // Clear code list
            case 0xCC: // Exception Handler Selection
            case 0xDE: // Entry Point
                break;

            // Boot-time cheats
            case 0xEE: // Disable Expansion Pak
            case 0xF0: // 8-bit Boot-Time Write
            case 0xF1: // 16-bit Boot-Time Write
            case 0xFF: // Cheat Engine Location
                *list1++ = address;
                *list1++ = value;
                break;

            // In-game cheats
            case 0x50: // Repeater/Patch
                // Validate repeater count
                if (address & 0x0000FF00)
                {
                    repeater = 1;
                    *list2++ = address;
                    *list2++ = value;
                }
                break;

            // Everything else
            default:
                if (!address)
                {
                    // TODO: Support special code types! :)
                }
            // Fall-through!

            case 0xD0: // 8-bit Equal-To Conditional
            case 0xD1: // 16-bit Equal-To Conditional
            case 0xD2: // 8-bit Not-Equal-To Conditional
            case 0xD3: // 16-bit Not-Equal-To Conditional
                // Validate 16-bit codes
                if ((address & 0x01000001) == 0x01000001)
                {
                    break;
                }

                *list2++ = address;
                *list2++ = value;
                break;
            }
            break;

        case YAML_STREAM_END_EVENT:
            // And we're outta here!
            done = 1;
            break;

        default:
            break;
        }

        yaml_event_delete(&event);
    } while (!done);

    // Free
    yaml_parser_delete(&parser);
    free(cheatfile);

    return repeater; // Ok or repeater error
}

//TODO: UNUSED CODE, WHAT IS IS FOR?
// void timing(display_context_t disp)
// {
//     unsigned char tmp[32];

//     IO_WRITE(PI_STATUS_REG, 0x02);

//     u32 pi0 = IO_READ(PI_BSD_DOM1_LAT_REG);
//     u32 pi1 = IO_READ(PI_BSD_DOM1_PWD_REG);
//     u32 pi2 = IO_READ(PI_BSD_DOM1_PGS_REG);
//     u32 pi3 = IO_READ(PI_BSD_DOM1_RLS_REG);

//     printText("timing dom1:", 3, -1, disp);
//     sprintf(tmp, "lat=%x pwd=%x\npgs=%x rls=%x", pi0, pi1, pi2, pi3);
//     printText(tmp, 3, -1, disp);
// }

void bootRom(display_context_t disp, int silent)
{
    if (boot_cic != 0)
    {
        if (boot_save != 0)
        {
            u8 cfg_file[32];
            u8 found = 0;
            u8 resp = 0;
            u8 tmp[32];

            //set cfg file with last loaded cart info and save-type
            sprintf(cfg_file, "/ED64/%s/LAST.CRT", save_path);

            resp = fatCreateRecIfNotExist(cfg_file, 0);
            resp = fatOpenFileByName(cfg_file, 1); //512 bytes fix one cluster

            static uint8_t cfg_file_data[512] = {0};
            cfg_file_data[0] = boot_save;
            cfg_file_data[1] = boot_cic;
            scopy(rom_filename, cfg_file_data + 2);

            fatWriteFile(&cfg_file_data, 1);
            sleep(500);

            //set the fpga cart-save type
            evd_setSaveType(boot_save);

            TRACE(disp, "try to restore save from sd");

            resp = saveTypeFromSd(disp, rom_filename, boot_save);

            TRACEF(disp, "saveTypeFromSd returned: %i", resp);
        }

        TRACE(disp, "Cartridge-Savetype set");
        TRACE(disp, "information stored for reboot-save...");

        sleep(50);

        u32 cart, country;
        u32 info = *(vu32 *)0xB000003C;
        cart = info >> 16;
        country = (info >> 8) & 0xFF;

        u32 *cheat_lists[2] = {NULL, NULL};
        if (cheats_on)
        {
            gCheats = 1;
            printText("try to load cheat-file...", 3, -1, disp);

            char cheat_filename[64];
            sprintf(cheat_filename, "/ED64/CHEATS/%s.yml", rom_filename);

            int ok = readCheatFile(cheat_filename, cheat_lists);
            if (ok == 0)
            {
                printText("cheats found...", 3, -1, disp);
                sleep(600);
            }
            else
            {
                printText("cheats not found...", 3, -1, disp);
                sleep(2000);
                gCheats = 0;
            }
        }
        else
        {
            gCheats = 0;
        }

        disable_interrupts();
        int bios_cic = getCicType(1);

        if (checksum_fix_on)
        {
            checksum_sdram();
        }

        evd_lockRegs();
        sleep(1000);

        while (!(disp = display_lock()))
            ;
        //blank screen to avoid glitches

        graphics_fill_screen(disp, 0x000000FF);
        display_show(disp);

        simulate_boot(boot_cic, bios_cic, cheat_lists); // boot_cic
    }
}

void playSound(int snd)
{
    //no thread support in libdragon yet, sounds pause the menu for a time :/

    if (snd == 1)
        sndPlaySFX("rom://sounds/ed64_mono.wav");

    if (snd == 2)
        sndPlaySFX("rom://sounds/bamboo.wav");

    if (snd == 3)
        sndPlaySFX("rom://sounds/warning.wav");

    if (snd == 4)
        sndPlaySFX("rom://sounds/done.wav");
}

//draws the next char at the text input screen
void drawInputAdd(display_context_t disp, char *msg)
{
    graphics_draw_box_trans(disp, 23, 5, 272, 18, 0x00000090);
    position++;
    sprintf(input_text, "%s%s", input_text, msg);
    drawTextInput(disp, input_text);
}

//del the last char at the text input screen
void drawInputDel(display_context_t disp)
{
    graphics_draw_box_trans(disp, 23, 5, 272, 18, 0x00000090);
    if (position)
    {
        input_text[position - 1] = '\0';
        drawTextInput(disp, input_text);

        position--;
    }
}

void drawTextInput(display_context_t disp, char *msg)
{
    graphics_draw_text(disp, 40, 15, msg);
}

void drawConfirmBox(display_context_t disp)
{
    drawBoxNumber(disp, 5);
    display_show(disp);

    if (sound_on)
        playSound(3);

    printText(" ", 9, 9, disp);
    printText("Confirmation required:", 9, -1, disp);
    printText(" ", 9, -1, disp);
    printText(" ", 9, -1, disp);
    printText("    Are you sure?", 9, -1, disp);
    printText(" ", 9, -1, disp);
    printText("    C-UP Continue ", 9, -1, disp); //set mapping 3
    printText(" ", 9, -1, disp);
    printText("      B Cancel", 9, -1, disp);

    sleep(500);
}

void drawShortInfoBox(display_context_t disp, char *text, u8 mode)
{
    if (mode == 0)
        drawBoxNumber(disp, 7);
    else if (mode == 1)
        drawBoxNumber(disp, 8);
    else if (mode == 2)
        drawBoxNumber(disp, 10);

    printText(text, 9, 14, disp);

    display_show(disp);

    if (sound_on)
    {
        if (mode == 0)
            playSound(4);
        else if (mode == 1)
            playSound(3);
        else if (mode == 2)
            playSound(4);
    }
    sleep(300);
}

void readRomConfig(display_context_t disp, char *short_filename, char *full_filename)
{
    char cfg_filename[128];
    sprintf(rom_filename, "%s", short_filename);
    rom_filename[strlen(rom_filename) - 4] = '\0'; // cut extension
    sprintf(cfg_filename, "/ED64/CFG/%s.CFG", rom_filename);

    uint8_t rom_cfg_data[512];

    FatRecord rec_tmpf;

    if (fatFindRecord(cfg_filename, &rec_tmpf, 0) == 0)
    {
        //read rom-config
        u8 resp = 0;
        resp = fatOpenFileByName(cfg_filename, 0);
        resp = fatReadFile(&rom_cfg_data, 1);

        rom_config[1] = rom_cfg_data[0];
        rom_config[2] = rom_cfg_data[1];
        rom_config[3] = rom_cfg_data[2];
        rom_config[4] = rom_cfg_data[3];
        rom_config[5] = rom_cfg_data[4];
        rom_config[6] = rom_cfg_data[5];
        rom_config[7] = rom_cfg_data[6];
        rom_config[8] = rom_cfg_data[7];
        rom_config[9] = rom_cfg_data[8];
    }
    else
    {
        //preload with header data

        romInfoScreen(disp, full_filename, 1); //silent info screen with readout
    }

    //preload cursor position 1 cic
    rom_config[0] = 1;
}

void alterRomConfig(int type, int mode)
{
    //mode 1 = increae mode 2 = decrease

    //cic
    u8 min_cic = 0;
    u8 max_cic = 5;

    //save
    u8 min_save = 0;
    u8 max_save = 5;

    //tv-type
    u8 min_tv = 0;
    u8 max_tv = 3;

    //cheat
    u8 min_cheat = 0;
    u8 max_cheat = 1;

    //chk fix
    u8 min_chk_sum = 0;
    u8 max_chk_sum = 1;

    //quality
    u8 min_quality = 0;
    u8 max_quality = 5;

    //country
    u8 min_country = 0;
    u8 max_country = 2;

    switch (type)
    {
    case 1:
        //start cic
        if (mode == 1)
        {
            //down
            if (rom_config[1] < max_cic)
                rom_config[1]++;
            if (rom_config[1] == 3)
                rom_config[1]++;
        }
        else if (mode == 2)
        {
            //up
            if (rom_config[1] > min_cic)
                rom_config[1]--;
            if (rom_config[1] == 3)
                rom_config[1]--;
        }
        //end cic
        break;
    case 2:
        //start save
        if (mode == 1)
        {
            //down
            if (rom_config[2] < max_save)
                rom_config[2]++;
        }
        else if (mode == 2)
        {
            //up
            if (rom_config[2] > min_save)
                rom_config[2]--;
        }
        //end save
        break;
    case 3:
        //start tv
        if (mode == 1)
        {
            //down
            if (rom_config[3] < max_tv)
                rom_config[3]++;
        }
        else if (mode == 2)
        {
            //up
            if (rom_config[3] > min_tv)
                rom_config[3]--;
        }
        //end tv
        break;
    case 4:
        //start cheat
        if (mode == 1)
        {
            //down
            if (rom_config[4] < max_cheat)
                rom_config[4]++;
        }
        else if (mode == 2)
        {
            //up
            if (rom_config[4] > min_cheat)
                rom_config[4]--;
        }
        //end cheat
        break;
    case 5:
        //start chk sum
        if (mode == 1)
        {
            //down
            if (rom_config[5] < max_chk_sum)
                rom_config[5]++;
        }
        else if (mode == 2)
        {
            //up
            if (rom_config[5] > min_chk_sum)
                rom_config[5]--;
        }
        //end chk sum
        break;
    case 6:
        //start quality
        if (mode == 1)
        {
            //down
            if (rom_config[6] < max_quality)
                rom_config[6]++;
        }
        else if (mode == 2)
        {
            //up
            if (rom_config[6] > min_quality)
                rom_config[6]--;
        }
        break;
    case 7:
        //start country
        if (mode == 1)
        {
            //down
            if (rom_config[7] < max_country)
                rom_config[7]++;
        }
        else if (mode == 2)
        {
            //up
            if (rom_config[7] > min_country)
                rom_config[7]--;
        }
        break;

    default:
        break;
    }
}

void drawToplistBox(display_context_t disp, int line)
{
    list_pos_backup[0] = cursor;
    list_pos_backup[1] = page;

    u8 list_size = 0;

    if (line == 0)
    {
        FatRecord *rec;
        u8 resp = 0;

        count = 1;
        dir_t buf;

        //load the directory-entry
        resp = fatLoadDirByName("/ED64/CFG");

        int dsize = dir->size;

        char toplist[dsize][256];

        for (int i = 0; i < dir->size; i++)
        {
            rec = dir->rec[i];
            u8 rom_cfg_file[128];

            //set rom_cfg
            sprintf(rom_cfg_file, "/ED64/CFG/%s", rec->name);

            static uint8_t cfg_file_data[512] = {0};

            resp = fatOpenFileByName(rom_cfg_file, 0); //512 bytes fix one cluster
            resp = fatReadFile(&cfg_file_data, 1);

            toplist[i][0] = (char)cfg_file_data[5];     //quality
            strcpy(toplist[i] + 1, cfg_file_data + 32); //fullpath
        }

        qsort(toplist, dsize, 256, compare_int_reverse);

        if (dsize > 15)
            list_size = 15;
        else
            list_size = dsize;

        for (int c = 0; c < list_size; c++)
            strcpy(toplist15[c], toplist[c]);

        list_pos_backup[2] = list_size;
    }

    list_size = list_pos_backup[2];

    u8 min = 1;
    u8 max = 15;
    u8 real_max = 0;

    for (int c = 0; c < list_size; c++)
        if (toplist15[c][0] != 0)
            real_max++;

    max = real_max;

    //cursor line
    if (line != 0)
    {
        if (line == 1)
        {
            //down
            if (toplist_cursor < max)
                toplist_cursor++;
        }
        else if (line == 2)
        {
            //up
            if (toplist_cursor > min)
                toplist_cursor--;
        }
    }

    drawBoxNumber(disp, 9); //toplist

    printText("          -Toplist 15-", 4, 7, disp);
    printText(" ", 4, -1, disp);

    uint32_t forecolor;
    uint32_t backcolor;
    backcolor = graphics_make_color(0x00, 0x00, 0x00, 0x00); //bg
    forecolor = graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF); //fg

    for (int t = 0; t < 15; t++)
    {
        int quality_level = toplist15[t][0]; //quality
        if (quality_level != 0)
        {
            switch (quality_level)
            {
            case 1:
                forecolor = graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF); //common (white)
                break;
            case 2:
                forecolor = graphics_make_color(0x00, 0xFF, 0x00, 0xCF); //uncommon (green)
                break;
            case 3:
                forecolor = graphics_make_color(0x1E, 0x90, 0xFF, 0xFF); //rare (blue)
                break;
            case 4:
                forecolor = graphics_make_color(0x9B, 0x30, 0xFF, 0xFF); //epic (purple)
                break;
            case 5:
                forecolor = graphics_make_color(0xFF, 0xA5, 0x00, 0xFF); //legendary (orange)
                break;
            default:
                break;
            }

            graphics_set_color(forecolor, backcolor);
            //max 32 chr

            u8 short_name[256];

            sprintf(short_name, "%s", toplist15[t] + 1);

            u8 *pch_s; // point-offset
            pch_s = strrchr(short_name, '/');

            u8 *pch; // point-offset
            pch = strrchr(pch_s, '.');
            pch_s[30] = '\0'; //was 31
            pch_s[pch - pch_s] = '\0';

            if (t + 1 == toplist_cursor)
                printText(pch_s + 1, 5, -1, disp);
            else
                printText(pch_s + 1, 4, -1, disp);

            //restore color
            graphics_set_color(graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF), graphics_make_color(0x00, 0x00, 0x00, 0x00));
        }
    }
}

void drawRomConfigBox(display_context_t disp, int line)
{
    u8 min = 1;
    u8 max = 7;

    //cursor line
    if (line != 0)
    {
        if (line == 1)
        {
            //down
            if (rom_config[0] < max)
                rom_config[0]++;
        }
        else if (line == 2)
        {
            //up
            if (rom_config[0] > min)
                rom_config[0]--;
        }
    }

    drawBoxNumber(disp, 6);

    drawConfigSelection(disp, rom_config[0]);

    printText(" ", 9, 9, disp);
    printText("Rom configuration:", 9, -1, disp);
    printText(" ", 9, -1, disp);

    switch (rom_config[1])
    {
    case 0:
        printText("     CIC: 6101", 9, -1, disp);
        break;
    case 1:
        printText("     CIC: 6102", 9, -1, disp);
        break;
    case 2:
        printText("     CIC: 6103", 9, -1, disp);
        break;
    case 3:
        printText("     CIC: 6104", 9, -1, disp);
        break; //do i really need 6104 in that list? :D
    case 4:
        printText("     CIC: 6105", 9, -1, disp);
        break;
    case 5:
        printText("     CIC: 6106", 9, -1, disp);
        break;
    default:
        break;
    }

    switch (rom_config[2])
    {
    case 0:
        printText("    Save: Off/Mempak", 9, -1, disp);
        break;
    case 1:
        printText("    Save: Sram 32", 9, -1, disp);
        break;
    case 2:
        printText("    Save: Sram 128", 9, -1, disp);
        break;
    case 3:
        printText("    Save: Eeprom 4k", 9, -1, disp);
        break;
    case 4:
        printText("    Save: Eeprom 16k", 9, -1, disp);
        break;
    case 5:
        printText("    Save: Flashram", 9, -1, disp);
        break;
    default:
        break;
    }

    switch (rom_config[3])
    {
    case 0:
        printText("      Tv: Force off", 9, -1, disp);
        break;
    case 1:
        printText("      Tv: NTSC", 9, -1, disp);
        break;
    case 2:
        printText("      Tv: PAL", 9, -1, disp);
        break;
    case 3:
        printText("      Tv: M-PAL", 9, -1, disp);
        break;
    default:
        break;
    }

    switch (rom_config[4])
    {
    case 0:
        printText("   Cheat: off", 9, -1, disp);
        break;
    case 1:
        printText("   Cheat: on", 9, -1, disp);
        break;
    default:
        break;
    }

    switch (rom_config[5])
    {
    case 0:
        printText("Checksum: disable fix", 9, -1, disp);
        break;
    case 1:
        printText("Checksum: enable fix", 9, -1, disp);
        break;
    default:
        break;
    }

    switch (rom_config[6])
    {
    case 0:
        printText("  Rating: off", 9, -1, disp);
        break;
    case 1:
        printText("  Rating: common", 9, -1, disp);
        break;
    case 2:
        printText("  Rating: uncommon", 9, -1, disp);
        break;
    case 3:
        printText("  Rating: rare", 9, -1, disp);
        break;
    case 4:
        printText("  Rating: epic", 9, -1, disp);
        break;
    case 5:
        printText("  Rating: legendary", 9, -1, disp);
        break;

    default:
        break;
    }

    switch (rom_config[7])
    {
    case 0:
        printText(" Country: default", 9, -1, disp);
        break;
    case 1:
        printText(" Country: NTSC", 9, -1, disp);
        break;
    case 2:
        printText(" Country: PAL", 9, -1, disp);
        break;
    default:
        break;
    }

    printText(" ", 9, -1, disp);
    printText("B Cancel", 9, -1, disp);
    printText("A Save config", 9, -1, disp);
}

//draws the charset for the textinputscreen
void drawSet1(display_context_t disp)
{
    set = 1;
    uint32_t forecolor;
    uint32_t backcolor;
    backcolor = graphics_make_color(0x00, 0x00, 0x00, 0xFF);
    forecolor = graphics_make_color(0xFF, 0xFF, 0x00, 0xFF); //yellow

    graphics_draw_text(disp, 80, 40, "<"); //L
    graphics_set_color(forecolor, backcolor);
    graphics_draw_text(disp, 233, 40, "A");  //R
    graphics_draw_text(disp, 223, 62, "B");  //G up
    graphics_draw_text(disp, 210, 74, "C");  //G left
    graphics_draw_text(disp, 235, 74, "D");  //G right
    graphics_draw_text(disp, 193, 86, "E");  //B
    graphics_draw_text(disp, 223, 86, "F");  //G down
    graphics_draw_text(disp, 209, 100, "G"); //A
}

void drawSet2(display_context_t disp)
{
    set = 2;
    uint32_t forecolor;
    uint32_t backcolor;
    backcolor = graphics_make_color(0x00, 0x00, 0x00, 0xFF);
    forecolor = graphics_make_color(0xFF, 0xFF, 0x00, 0xFF);

    graphics_draw_text(disp, 80, 40, "<");
    graphics_set_color(forecolor, backcolor);
    graphics_draw_text(disp, 233, 40, "H");
    graphics_draw_text(disp, 223, 62, "I");
    graphics_draw_text(disp, 210, 74, "J");
    graphics_draw_text(disp, 235, 74, "K");
    graphics_draw_text(disp, 193, 86, "L");
    graphics_draw_text(disp, 223, 86, "M");
    graphics_draw_text(disp, 209, 100, "N");
}

void drawSet3(display_context_t disp)
{
    set = 3;
    uint32_t forecolor;
    uint32_t backcolor;
    backcolor = graphics_make_color(0x00, 0x00, 0x00, 0xFF);
    forecolor = graphics_make_color(0xFF, 0xFF, 0x00, 0xFF);

    graphics_draw_text(disp, 80, 40, "<");
    graphics_set_color(forecolor, backcolor);
    graphics_draw_text(disp, 233, 40, "O");
    graphics_draw_text(disp, 223, 62, "P");
    graphics_draw_text(disp, 210, 74, "Q");
    graphics_draw_text(disp, 235, 74, "R");
    graphics_draw_text(disp, 193, 86, "S");
    graphics_draw_text(disp, 223, 86, "T");
    graphics_draw_text(disp, 209, 100, "U");
}

display_context_t lockVideo(int wait)
{
    display_context_t dc;

    if (wait)
        while (!(dc = display_lock()))
            ;
    else
        dc = display_lock();
    return dc;
}

void drawSet4(display_context_t disp)
{
    set = 4;
    uint32_t forecolor;
    uint32_t backcolor;
    backcolor = graphics_make_color(0x00, 0x00, 0x00, 0xFF);
    forecolor = graphics_make_color(0xFF, 0xFF, 0x00, 0xFF);

    graphics_draw_text(disp, 80, 40, "<");
    graphics_set_color(forecolor, backcolor);
    graphics_draw_text(disp, 233, 40, "V");
    graphics_draw_text(disp, 223, 62, "W");
    graphics_draw_text(disp, 210, 74, "X");
    graphics_draw_text(disp, 235, 74, "Y");
    graphics_draw_text(disp, 193, 86, "Z");
    graphics_draw_text(disp, 223, 86, "-");
    graphics_draw_text(disp, 209, 100, "_");
}

void showAboutScreen(display_context_t disp)
{
    drawBoxNumber(disp, 2);
    display_show(disp);

    if (sound_on)
        playSound(2);

    printText("Altra64: v0.1.8.6.1.2", 9, 8, disp);
    sprintf(firmware_str, "ED64 firmware: v%03x", evd_getFirmVersion());
    printText(firmware_str, 9, -1, disp);
    printText("by Saturnu", 9, -1, disp);
    printText("& JonesAlmighty", 9, -1, disp);
    printText(" ", 9, -1, disp);
    printText("Code engine by:", 9, -1, disp);
    printText("Jay Oster", 9, -1, disp);
    printText(" ", 9, -1, disp);
    printText("thanks to:", 9, -1, disp);
    printText("Krikzz", 9, -1, disp);
    printText("Richard Weick", 9, -1, disp);
    printText("ChillyWilly", 9, -1, disp);
    printText("ShaunTaylor", 9, -1, disp);
    printText("Conle", 9, -1, disp);
}

void loadFile(display_context_t disp)
{
    char name_file[256];

    if (strcmp(pwd, "/") == 0)
        sprintf(name_file, "/%s", list[cursor].filename);
    else
        sprintf(name_file, "%s/%s", pwd, list[cursor].filename);

    int ft = 0;
    char _upper_name_file[256];

    strcpy(_upper_name_file, name_file);

    strhicase(_upper_name_file, strlen(_upper_name_file));
    sprintf(_upper_name_file, "%s", _upper_name_file);

    u8 extension[4];
    u8 *pch;
    pch = strrchr(_upper_name_file, '.'); //asd.n64

    sprintf(extension, "%s", (pch + 1)); //0123456

    if (!strcmp(extension, "GB"))
        ft = 5;
    if (!strcmp(extension, "GBC"))
        ft = 6;
    if (!strcmp(extension, "NES"))
        ft = 7;
    if (!strcmp(extension, "GG"))
        ft = 8;
    if (!strcmp(extension, "MSX"))
        ft = 9;
    if (!strcmp(extension, "MP3"))
        ft = 10;
    if (!strcmp(extension, "MPK"))
        ft = 2;
    if (!strcmp(extension, "Z64") || !strcmp(extension, "V64") || !strcmp(extension, "N64"))
        ft = 1;

    if (ft != 10 || ft != 2)
    {
        while (!(disp = display_lock()))
            ;

        clearScreen(disp);
        u16 msg = 0;
        sleep(300);
        evd_ulockRegs();
        sleep(300);
        sprintf(rom_filename, "%s", list[cursor].filename);
        display_show(disp);
        select_mode = 9;
    }
    switch (ft)
    {
    case 1:
        if (quick_boot) //write to the file
        {
            u8 resp = 0;
            resp = fatCreateRecIfNotExist("/ED64/LASTROM.CFG", 0);
            resp = fatOpenFileByName("/ED64/LASTROM.CFG", 1); //512 bytes fix one cluster
            static uint8_t lastrom_file_data[512] = {0};
            scopy(name_file, lastrom_file_data);
            fatWriteFile(&lastrom_file_data, 1);
        }

        //read rom_config data
        readRomConfig(disp, rom_filename, name_file);

        loadrom(disp, name_file, 1);
        display_show(disp);

        //rom loaded mapping
        input_mapping = rom_loaded;
        break;
    case 2:
        while (!(disp = display_lock()))
            ;
        clearScreen(disp); //part clear?
        display_dir(list, cursor, page, MAX_LIST, count, disp);
        display_show(disp);
        drawShortInfoBox(disp, " L=Restore  R=Backup", 2);
        input_mapping = mpk_choice;
        sprintf(rom_filename, "%s", name_file);
        break;
    case 5:
    case 6:
        loadgbrom(disp, name_file);
        display_show(disp);
        break;
    case 7:
        loadnesrom(disp, name_file);
        display_show(disp);
        break;
    case 8:
        loadggrom(disp, name_file);
        display_show(disp);
        break;
    case 9:
        loadmsx2rom(disp, name_file);
        display_show(disp);
        break;
    case 10:
        buf_size = audio_get_buffer_length() * 4;
        buf_ptr = malloc(buf_size);

        while (!(disp = display_lock()))
            ;
        clearScreen(disp);
        drawShortInfoBox(disp, "      playback", 0);

        long long start = 0, end = 0, curr, pause = 0, samples;
        int rate = 44100, last_rate = 44100, channels = 2;

        audio_init(44100, 2);

        mp3_Start(name_file, &samples, &rate, &channels);
        playing = 1;
        select_mode = 9;

        input_mapping = mp3; //mp3 stop

        display_show(disp);
        break;
    default:
        break;
    }
}

void handleInput(display_context_t disp, sprite_t *contr)
{
    //request controller
    controller_scan();
    struct controller_data keys = get_keys_down();
    struct controller_data keys_held = get_keys_held();

    if (keys.c[0].up || keys_held.c[0].up || keys_held.c[0].y > +25)
    {
        switch (input_mapping)
        {
        case file_manager:
            if (select_mode)
            {
                if (count != 0)
                {
                    if (scroll_behaviour == 1)
                        cursor--;
                    else if (cursor != 0)
                    {
                        if ((cursor + 0) % 20 == 0)
                        {
                            cursor--;
                            page -= 20;
                        }
                        else
                        {
                            cursor--;
                        }
                    }

                    //end
                    while (!(disp = display_lock()))
                        ;

                    new_scroll_pos(&cursor, &page, MAX_LIST, count);

                    clearScreen(disp); //part clear?
                    display_dir(list, cursor, page, MAX_LIST, count, disp);

                    display_show(disp);
                }
            }
            break;
        case mempak_menu:
            break;
        case char_input:
            //chr input screen
            set = 1;
            break;
        case rom_config_box:
            while (!(disp = display_lock()))
                ;

            new_scroll_pos(&cursor, &page, MAX_LIST, count);
            clearScreen(disp); //part clear?
            display_dir(list, cursor, page, MAX_LIST, count, disp);

            drawRomConfigBox(disp, 2);
            display_show(disp);
            input_mapping = rom_config_box;
            sleep(80);
            break;
        case toplist:
            while (!(disp = display_lock()))
                ;

            drawBg(disp); //background
            drawToplistBox(disp, 2);

            display_show(disp);
            input_mapping = toplist;
            sleep(80);
            break;

        default:
            break;
        }
    }

    if (keys.c[0].down || keys_held.c[0].down || keys_held.c[0].y < -25)
    {
        switch (input_mapping)
        {
        case file_manager:
            if (select_mode)
            {
                if (count != 0)
                {
                    if (scroll_behaviour == 1)
                        cursor++;
                    else if (cursor + 1 != count)
                    {
                        if ((cursor + 1) % 20 == 0)
                        {
                            cursor++;
                            page += 20;
                        }
                        else
                        {
                            cursor++;
                        }
                    }

                    while (!(disp = display_lock()))
                        ;

                    new_scroll_pos(&cursor, &page, MAX_LIST, count);
                    clearScreen(disp); //part clear?
                    display_dir(list, cursor, page, MAX_LIST, count, disp);

                    display_show(disp);
                }
            }
            break;
        case mempak_menu:
            break;
        case char_input:
            //chr input screen
            set = 3;
            break;
        case rom_config_box:
            while (!(disp = display_lock()))
                ;

            new_scroll_pos(&cursor, &page, MAX_LIST, count);
            clearScreen(disp); //part clear?

            display_dir(list, cursor, page, MAX_LIST, count, disp);

            drawRomConfigBox(disp, 1);

            display_show(disp);
            input_mapping = rom_config_box;
            sleep(80);
            break;
        case toplist:
            while (!(disp = display_lock()))
                ;

            drawBg(disp);
            drawToplistBox(disp, 1);

            display_show(disp);
            input_mapping = toplist;
            sleep(80);
            break;

        default:
            break;
        }
    }
    else if (keys.c[0].left || keys_held.c[0].left || keys_held.c[0].x < -25)
    {
        switch (input_mapping)
        {
        case file_manager:
            if (select_mode)
            {
                if (count != 0 && scroll_behaviour == 0 && cursor - 20 >= 0)
                {
                    page -= 20;
                    cursor = page;
                }

                while (!(disp = display_lock()))
                    ;

                new_scroll_pos(&cursor, &page, MAX_LIST, count);
                clearScreen(disp); //part clear?
                display_dir(list, cursor, page, MAX_LIST, count, disp);

                display_show(disp);
            }
            break;
        case mempak_menu:
            break;
        case char_input:
            //chr input screen
            set = 4;
            break;
        case rom_config_box:
            while (!(disp = display_lock()))
                ;

            new_scroll_pos(&cursor, &page, MAX_LIST, count);
            clearScreen(disp); //part clear?
            display_dir(list, cursor, page, MAX_LIST, count, disp);

            alterRomConfig(rom_config[0], 2);

            drawRomConfigBox(disp, 0);
            display_show(disp);
            input_mapping = rom_config_box;
            sleep(80);
            break;

        default:
            break;
        }
    }
    else if (keys.c[0].right || keys_held.c[0].right || keys_held.c[0].x > +25)
    {
        switch (input_mapping)
        {
        case file_manager:

            if (select_mode)
            {
                if ((count != 0) &&
                    (scroll_behaviour == 0) &&
                    (page + 20 != count) &&
                    (cursor + 20 <= ((count / 20) * 20 + 19)))
                {
                    page += 20;
                    cursor = page;
                }

                while (!(disp = display_lock()))
                    ;

                new_scroll_pos(&cursor, &page, MAX_LIST, count);
                clearScreen(disp); //part clear?
                display_dir(list, cursor, page, MAX_LIST, count, disp);

                display_show(disp);
            }
            break;

        case mempak_menu:
            break;

        case char_input:

            //chr input screen
            set = 2;
            break;

        case rom_config_box:

            while (!(disp = display_lock()))
                ;

            new_scroll_pos(&cursor, &page, MAX_LIST, count);
            clearScreen(disp); //part clear?
            display_dir(list, cursor, page, MAX_LIST, count, disp);

            alterRomConfig(rom_config[0], 1);

            drawRomConfigBox(disp, 0);
            display_show(disp);
            input_mapping = rom_config_box;
            sleep(80);
            break;

        default:
            break;
        }
    }
    else if (keys.c[0].start)
    {
        switch (input_mapping)
        {
        case file_manager:

            //quick boot
            if (quick_boot)
            {
                FatRecord rec_last;
                uint8_t lastrom_cfg_data[512];

                if (fatFindRecord("/ED64/LASTROM.CFG", &rec_last, 0) == 0)
                {
                    u8 resp = 0;
                    resp = fatOpenFileByName("/ED64/LASTROM.CFG", 0);
                    resp = fatReadFile(&lastrom_cfg_data, 1);

                    u8 *short_s;
                    short_s = strrchr(lastrom_cfg_data, '/');

                    while (!(disp = display_lock()))
                        ;
                    clearScreen(disp);

                    sleep(100);
                    evd_ulockRegs();
                    sleep(100);

                    select_mode = 9;
                    //short          fullpath
                    readRomConfig(disp, short_s + 1, lastrom_cfg_data);

                    loadrom(disp, lastrom_cfg_data, 1);
                    display_show(disp);
                }
                //nothing else :>

                drawShortInfoBox(disp, "    rom not found", 0);
            }
            else if (list[cursor].type != DT_DIR && empty == 0)
            {
                loadFile(disp);
            }
            break;

        case mempak_menu:
            break;

        case char_input:

            //better config color-set
            graphics_set_color(
                graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF),
                graphics_make_color(0x00, 0x00, 0x00, 0x00));
            clearScreen(disp);
            display_dir(list, cursor, page, MAX_LIST, count, disp);

            if (input_text[0] != '\0')
            { //input_text is set - do backup
                drawBoxNumber(disp, 2);

                display_show(disp);

                printText("Mempak-Backup:", 9, 9, disp);
                printText(" ", 9, -1, disp);
                printText("search...", 9, -1, disp);
                mpk_to_file(disp, input_text, 0);
                sleep(300);

                drawShortInfoBox(disp, "         done", 0);
                sleep(2000);

                //reread filesystem
                cursor_line = 0;
                readSDcard(disp, "/");
            }

            input_mapping = file_manager;
            break;

        case rom_loaded:

            //rom start screen

            //normal boot
            //boot the loaded rom
            bootRom(disp, 0);
            //never return
            break;

        default:
            break;
        }
    }
    else if (keys.c[0].L)
    {
        switch (input_mapping)
        {
        case file_manager:

            input_mapping = mempak_menu;

            drawBoxNumber(disp, 2);

            display_show(disp);

            printText("Mempak-Subsystem:", 9, 9, disp);
            printText(" ", 9, -1, disp);
            printText(" ", 9, -1, disp);
            printText("  Z: View content", 9, -1, disp);
            printText(" ", 9, -1, disp);
            printText("  A: Backup - new", 9, -1, disp); //set mapping 3
            printText(" ", 9, -1, disp);
            printText("  R: Format", 9, -1, disp);
            printText(" ", 9, -1, disp);
            printText("  B: Abort", 9, -1, disp);
            if (sound_on)
                playSound(2);
            sleep(500);
            break;

        case mempak_menu:
            break;

        case char_input:

            //chr input screen
            drawInputDel(disp);
            break;

        case mpk_choice:

            //c-up or A
            drawConfirmBox(disp);
            //confirm restore mpk
            input_mapping = mpk_restore;
            break;

        default:
            break;
        }
    }
    else if (keys.c[0].R)
    {
        switch (input_mapping)
        {
        case file_manager:
            break;

        case mempak_menu:

            //c-up or A
            drawConfirmBox(disp);
            //confirm format mpk
            input_mapping = mpk_format;
            break;

        case char_input:

            //chr input screen
            if (set == 1)
                drawInputAdd(disp, "A"); //P X )
            if (set == 2)
                drawInputAdd(disp, "H");
            if (set == 3)
                drawInputAdd(disp, "O");
            if (set == 4)
                drawInputAdd(disp, "V");
            break;

        case mpk_choice:

            //c-up or A
            drawConfirmBox(disp);
            //confirm quick-backup
            input_mapping = mpk_quick_backup;
            break;

        default:
            break;
        }
    }
    else if (keys.c[0].C_up)
    {
        switch (input_mapping)
        {
        case file_manager:

            if (list[cursor].type != DT_DIR && empty == 0)
            {
                drawBoxNumber(disp, 11);

                char *part;

                part = malloc(slen(list[cursor].filename));
                sprintf(part, "%s", list[cursor].filename);
                part[31] = '\0';

                printText(part, 4, 14, disp);

                if (slen(list[cursor].filename) > 31)
                {
                    sprintf(part, "%s", list[cursor].filename + 31);
                    part[31] = '\0';
                    printText(part, 4, -1, disp);
                }

                free(part);
                input_mapping = abort_screen;
            }
            break;

        case mempak_menu:
            break;

        case char_input:

            //chr input screen
            if (set == 1)
                drawInputAdd(disp, "B"); //P X )
            if (set == 2)
                drawInputAdd(disp, "I");
            if (set == 3)
                drawInputAdd(disp, "P");
            if (set == 4)
                drawInputAdd(disp, "W");
            break;

        case rom_loaded:

            //rom start screen
            if (cheats_on == 0)
            {
                printText("cheat system activated...", 3, -1, disp);
                cheats_on = 1;
            }
            break;

        case mpk_format:
            // format mpk
            drawBoxNumber(disp, 2);
            display_show(disp);

            printText("Mempak-Format:", 9, 9, disp);
            printText(" ", 9, -1, disp);

            printText("formating...", 9, -1, disp);

            /* Make sure they don't have a rumble pak inserted instead */
            switch (identify_accessory(0))
            {
            case ACCESSORY_NONE:
                printText("No Mempak", 9, -1, disp);
                break;

            case ACCESSORY_MEMPAK:
                printText("Please wait...", 9, -1, disp);
                if (format_mempak(0))
                {
                    printText("Error formatting!", 9, -1, disp);
                }
                else
                {
                    drawShortInfoBox(disp, "         done", 0);
                    input_mapping = abort_screen;
                }
                break;

            case ACCESSORY_RUMBLEPAK:
                printText("Really, format a RumblePak?!", 9, -1, disp);
                break;
            }

            sleep(500);

            input_mapping = abort_screen;
            break;

        case mpk_restore:
            //restore mpk
            drawBoxNumber(disp, 2);
            display_show(disp);

            printText("Mempak-Restore:", 9, 9, disp);
            printText(" ", 9, -1, disp);

            file_to_mpk(disp, rom_filename);
            sleep(300);

            drawShortInfoBox(disp, "         done", 0);
            input_mapping = abort_screen;

            display_show(disp);
            break;

        case mpk_quick_backup:
            //quick-backup
            drawBoxNumber(disp, 2);
            display_show(disp);

            printText("Quick-Backup:", 9, 9, disp);
            printText(" ", 9, -1, disp);
            printText("search...", 9, -1, disp);

            mpk_to_file(disp, list[cursor].filename, 1); //quick
            sleep(300);

            drawShortInfoBox(disp, "         done", 0);
            sleep(500);
            input_mapping = abort_screen;
            break;

        default:
            break;
        }
    }
    else if (keys.c[0].C_right)
    {
        switch (input_mapping)
        {
        case file_manager:

            if (list[cursor].type != DT_DIR)
            {
                //show rom cfg screen

                char name_file[64];

                if (strcmp(pwd, "/") == 0)
                    sprintf(name_file, "/%s", list[cursor].filename);
                else
                    sprintf(name_file, "%s/%s", pwd, list[cursor].filename);

                /*filetype
                     * 1 rom
                     */

                //TODO: this code is very similar to that used in loadFile, we should move it to a seperate function!
                char _upper_name_file[64];

                strcpy(_upper_name_file, name_file);

                strhicase(_upper_name_file, strlen(_upper_name_file));
                sprintf(_upper_name_file, "%s", _upper_name_file);

                u8 extension[4];
                u8 *pch;
                pch = strrchr(_upper_name_file, '.'); //asd.n64

                sprintf(extension, "%s", (pch + 1)); //0123456

                if (!strcmp(extension, "Z64") || !strcmp(extension, "V64") || !strcmp(extension, "N64"))
                { //rom
                    //cfg rom
                    sprintf(rom_filename, "%s", list[cursor].filename);

                    //preload config or file header
                    readRomConfig(disp, rom_filename, name_file);

                    drawRomConfigBox(disp, 0);
                    display_show(disp);
                    input_mapping = rom_config_box;
                }
            }
            break;

        case mempak_menu:
            break;

        case char_input:

            //chr input screen
            if (set == 1)
                drawInputAdd(disp, "D"); //P X )
            if (set == 2)
                drawInputAdd(disp, "K");
            if (set == 3)
                drawInputAdd(disp, "R");
            if (set == 4)
                drawInputAdd(disp, "Y");
            break;

        case rom_loaded:

            //rom start screen

            if (force_tv != 0)
            {
                printText("force tv mode...", 3, -1, disp);
            }
            break;

        default:
            break;
        }
    }
    else if (keys.c[0].C_down)
    {
        switch (input_mapping)
        {
        case file_manager:
            scopy(pwd, list_pwd_backup);
            while (!(disp = display_lock()))
                ;

            drawBg(disp); //background

            toplist_cursor = 1;
            drawToplistBox(disp, 0); //0 = load entries
            display_show(disp);

            input_mapping = toplist;
            break;

        case mempak_menu:
            break;

        case char_input:

            //chr input screen
            if (set == 1)
                drawInputAdd(disp, "F"); //P X )
            if (set == 2)
                drawInputAdd(disp, "M");
            if (set == 3)
                drawInputAdd(disp, "T");
            if (set == 4)
                drawInputAdd(disp, "-"); //GR Set4
            break;

        default:
            break;
        }
    }
    else if (keys.c[0].C_left)
    {
        switch (input_mapping)
        {
        case file_manager:

            if (list[cursor].type != DT_DIR)
            {
                //TODO: this code is similar (if not the same) as loadFile and can be optimised!
                //open
                char name_file[64];

                if (strcmp(pwd, "/") == 0)
                    sprintf(name_file, "/%s", list[cursor].filename);
                else
                    sprintf(name_file, "%s/%s", pwd, list[cursor].filename);

                /*filetype
                         * 1 rom
                         * 2 mempak
                         * 3 background
                         * 4 mp3
                         */

                int ft = 0;
                char _upper_name_file[64];

                strcpy(_upper_name_file, name_file);
                strhicase(_upper_name_file, strlen(_upper_name_file));
                sprintf(_upper_name_file, "%s", _upper_name_file);

                u8 extension[4];
                u8 *pch;
                pch = strrchr(_upper_name_file, '.');
                sprintf(extension, "%s", (pch + 1));

                if (!strcmp(extension, "MPK"))
                    ft = 2;
                if (!strcmp(extension, "Z64") || !strcmp(extension, "V64") || !strcmp(extension, "N64"))
                    ft = 1;

                if (ft == 1)
                { //rom
                    //load rom
                    drawBoxNumber(disp, 3); //rominfo

                    u16 msg = 0;
                    sleep(300);
                    evd_ulockRegs();
                    sleep(300);
                    sprintf(rom_filename, "%s", list[cursor].filename);
                    romInfoScreen(disp, name_file, 0);

                    if (sound_on)
                        playSound(2);

                    sleep(500);
                    input_mapping = abort_screen;
                }
                if (ft == 2)
                { //mpk file
                    drawBoxNumber(disp, 4);
                    display_show(disp);

                    if (strcmp(pwd, "/") == 0)
                        sprintf(rom_filename, "/%s", list[cursor].filename);
                    else
                        sprintf(rom_filename, "%s/%s", pwd, list[cursor].filename);

                    view_mpk_file(disp, rom_filename);

                    if (sound_on)
                        playSound(2);

                    sleep(500);
                    input_mapping = abort_screen;
                }
            } //mapping and not dir
            break;

        case mempak_menu:
            break;

        case char_input:

            //chr input screen
            if (set == 1)
                drawInputAdd(disp, "C"); //P X )
            if (set == 2)
                drawInputAdd(disp, "J");
            if (set == 3)
                drawInputAdd(disp, "Q");
            if (set == 4)
                drawInputAdd(disp, "X");
            break;
        }
    }
    else if (keys.c[0].Z)
    {
        switch (input_mapping)
        {
        case file_manager:
            input_mapping = abort_screen;

            showAboutScreen(disp);
            break;

        case mempak_menu:
            input_mapping = abort_screen;
            if (sound_on)
                playSound(2);

            drawBoxNumber(disp, 4);
            display_show(disp);
            view_mpk(disp);
            break;

        default:
            break;
        }
    }
    else if (keys.c[0].A)
    {
        switch (input_mapping)
        {
        // open
        case file_manager:
        {
            while (!(disp = display_lock()))
                ;

            if (list[cursor].type == DT_DIR && empty == 0)
            {
                char name_dir[256];

                /* init pwd=/
                         * /
                         *
                         * cursor=ED64
                         * /ED64
                         *
                         * cursor=SAVE
                         * /ED64/SAVE
                         */

                if (strcmp(pwd, "/") == 0)
                    sprintf(name_dir, "/%s", list[cursor].filename);
                else
                    sprintf(name_dir, "%s/%s", pwd, list[cursor].filename);

                sprintf(curr_dirname, "%s", list[cursor].filename);
                sprintf(pwd, "%s", name_dir);

                //load dir
                cursor_lastline = 0;
                cursor_line = 0;

                //backup tree cursor postions
                if (cd_behaviour == 1)
                {
                    cursor_history[cursor_history_pos] = cursor;
                    cursor_history_pos++;
                }

                readSDcard(disp, name_dir);
                display_show(disp);
            } //mapping 1 and dir
            else if (list[cursor].type != DT_DIR && empty == 0)
            { //open
                loadFile(disp);
            }
            break; //mapping 1 end
        }
        case mempak_menu:
        {
            //open up charinput screen
            input_mapping = char_input;
            input_text[0] = '\0';
            graphics_draw_sprite(disp, 0, 0, contr);
            break;
        }
        case char_input:
        {
            //chr input screen
            if (set == 1)
                drawInputAdd(disp, "G"); //P X )
            if (set == 2)
                drawInputAdd(disp, "N");
            if (set == 3)
                drawInputAdd(disp, "U");
            if (set == 4)
                drawInputAdd(disp, "_");
            break;
        }
        case rom_config_box:
        {
            //save rom_cfg[] to
            // /ED64/CFG/Romname.cfg if not exist create
            // if exist open/write

            //print confirm msg
            char name_file[256];

            if (strcmp(pwd, "/") == 0)
                sprintf(name_file, "/%s", list[cursor].filename);
            else
                sprintf(name_file, "%s/%s", pwd, list[cursor].filename);

            u8 rom_cfg_file[128];

            u8 resp = 0;

            //set rom_cfg
            sprintf(rom_cfg_file, "/ED64/CFG/%s.CFG", rom_filename);

            resp = fatCreateRecIfNotExist(rom_cfg_file, 0);
            resp = fatOpenFileByName(rom_cfg_file, 1); //512 bytes fix one cluster

            static uint8_t cfg_file_data[512] = {0};
            cfg_file_data[0] = rom_config[1]; //cic
            cfg_file_data[1] = rom_config[2]; //save
            cfg_file_data[2] = rom_config[3]; //tv
            cfg_file_data[3] = rom_config[4]; //cheat
            cfg_file_data[4] = rom_config[5]; //chksum
            cfg_file_data[5] = rom_config[6]; //rating
            cfg_file_data[6] = rom_config[7]; //country
            cfg_file_data[7] = rom_config[8];
            cfg_file_data[8] = rom_config[9];

            //copy full rom path to offset at 32 byte - 32 bytes reversed
            scopy(name_file, cfg_file_data + 32); //filename to rom_cfg file
            fatWriteFile(&cfg_file_data, 1);
            sleep(200);

            drawShortInfoBox(disp, "         done", 0);
            toplist_reload = 1;

            input_mapping = abort_screen;
            break;
        }
        case toplist:
        {
            //run from toplist
            u8 *pch_s;
            pch_s = strrchr(toplist15[toplist_cursor - 1] + 1, '/');

            readRomConfig(disp, pch_s + 1, toplist15[toplist_cursor - 1] + 1);

            loadrom(disp, toplist15[toplist_cursor - 1] + 1, 1);

            //rom loaded mapping
            input_mapping = rom_loaded;
            break;
        }
        case abort_screen:
        {
            //rom info screen

            while (!(disp = display_lock()))
                ;

            clearScreen(disp); //part clear?
            display_dir(list, cursor, page, MAX_LIST, count, disp);

            display_show(disp);

            input_mapping = file_manager;
            break;
        }
        default:
            break;
        }
    } //key a
    else if (keys.c[0].B)
    { //go back
        switch (input_mapping)
        {
        case file_manager:

            if (!(strcmp(pwd, "/") == 0))
            {
                while (!(disp = display_lock()))
                    ;

                //replace by strstr()? :>
                int slash_pos = 0;
                int i = 0;
                while (pwd[i] != '\0')
                {
                    if (pwd[i] == '/')
                        slash_pos = i;

                    i++;
                }
                char new_pwd[64];

                int j;
                for (j = 0; j < slash_pos; j++)
                    new_pwd[j] = pwd[j];

                if (j == 0)
                    j++;

                new_pwd[j] = '\0';

                sprintf(pwd, "%s", new_pwd);

                cursor_lastline = 0;
                cursor_line = 0;

                readSDcard(disp, pwd);

                if (cd_behaviour == 1)
                {
                    cursor_history_pos--;
                    cursor = cursor_history[cursor_history_pos];

                    if (cursor_history[cursor_history_pos] > 0)
                    {
                        cursor_line = cursor_history[cursor_history_pos] - 1;
                        if (scroll_behaviour == 0)
                        {
                            int p = cursor_line / 20;
                            page = p * 20;
                        }
                        else
                        {
                            if (cursor_line > 19)
                                cursor_line = 19;
                        }
                    }
                    else
                        cursor_line = 0;
                }

                while (!(disp = display_lock()))
                    ;

                new_scroll_pos(&cursor, &page, MAX_LIST, count);

                clearScreen(disp); //part clear?
                display_dir(list, cursor, page, MAX_LIST, count, disp);

                display_show(disp);
            } //not root
            break;

        case mempak_menu:

            while (!(disp = display_lock()))
                ;

            graphics_set_color(graphics_make_color(0xFF, 0xFF, 0xFF, 0xFF), graphics_make_color(0x00, 0x00, 0x00, 0x00));
            clearScreen(disp);
            display_show(disp);

            display_dir(list, cursor, page, MAX_LIST, count, disp);
            input_mapping = file_manager;
            display_show(disp);
            break;

        case char_input:

            //chr input screen

            /* Lazy switching */
            if (set == 1)
                drawInputAdd(disp, "E"); //P X )
            if (set == 2)
                drawInputAdd(disp, "L");
            if (set == 3)
                drawInputAdd(disp, "S");
            if (set == 4)
                drawInputAdd(disp, "Z");
            break;

        case abort_screen:
        case mpk_format:
        case mpk_restore:
        case rom_config_box:
        case mpk_quick_backup:

            //rom info screen
            input_mapping = file_manager;

            while (!(disp = display_lock()))
                ;

            clearScreen(disp); //part clear?
            display_dir(list, cursor, page, MAX_LIST, count, disp);

            display_show(disp);
            break;

        case toplist:

            //leave toplist
            while (!(disp = display_lock()))
                ;

            readSDcard(disp, list_pwd_backup);

            if (scroll_behaviour == 0)
            {
                cursor = list_pos_backup[0];
                page = list_pos_backup[1];
            }
            else
            {
                cursor_line = 0;
            }

            clearScreen(disp); //part clear?
            display_dir(list, cursor, page, MAX_LIST, count, disp);

            display_show(disp);

            input_mapping = file_manager;
            break;

        case mp3:

            //stop mp3

            mp3_Stop();
            playing = 0;

            clearScreen(disp); //part clear?
            display_dir(list, cursor, page, MAX_LIST, count, disp);

            display_show(disp);

            input_mapping = file_manager;
            break;

        default:
            break;
        }
    } //key b
}

//entry point
int main(void)
{
    int fast_boot = 0;

    //reserve memory
    list = malloc(sizeof(direntry_t));

    //dfs init for the rom-attached virtual filesystem
    if (dfs_init(DFS_DEFAULT_LOCATION) == DFS_ESUCCESS)
    {
        // everdrive initial function
        configure();

        //fast boot for backup-save data
        //int sj = evd_readReg(REG_CFG); // not sure if this is needed!
        int save_job = evd_readReg(REG_SAV_CFG); //TODO: or the firmware is V3

        if (save_job != 0)
            fast_boot = 1;

        //not gamepads more or less the n64 hardware-controllers
        controller_init();

        //filesystem on
        initFilesystem();
        sleep(200);

        readConfigFile();
        //n64 initialization

        //sd card speed settings from config
        if (sd_speed == 2)
        {
            bi_speed50();
        }
        else
        {
            bi_speed25();
        }

        if (tv_mode != 0)
        {
            *(u32 *)0x80000300 = tv_mode;
        }

        init_interrupts();

        if (sound_on)
        {
            //load soundsystem
            audio_init(44100, 2);
            sndInit();
        }

        timer_init();

        //background
        display_init(res, DEPTH_32_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE);

        //bg buffer
        static display_context_t disp;

        //Grab a render buffer
        while (!(disp = display_lock()))
            ;

        //backgrounds from ramfs/libdragonfs

        if (!fast_boot)
        {
            splashscreen = read_sprite("rom://sprites/splash.sprite");
            graphics_draw_sprite(disp, 0, 0, splashscreen); //start-picture
            display_show(disp);

            if (sound_on)
            {
                playSound(1);
                for (int s = 0; s < 200; s++) //todo: this blocks for 2 seconds (splashscreen)! is there a better way before the main loop starts!
                {
                    sndUpdate();
                    sleep(10);
                }
            }
        }

        char background_path[64];
        sprintf(background_path, "/ED64/wallpaper/%s", background_image);

        FatRecord rec_tmpf;
        if (fatFindRecord(background_path, &rec_tmpf, 0) == 0)
        {
            background = loadPng(background_path);
        }
        else
        {
            background = read_sprite("rom://sprites/background.sprite");
        }

        //todo: if bgm is enabled, we should start it...
        //sndPlayBGM("rom://sounds/bgm21.it");

        border_color_1 = translate_color(border_color_1_s);
        border_color_2 = translate_color(border_color_2_s);
        box_color = translate_color(box_color_s);
        selection_color = translate_color(selection_color_s);
        selection_font_color = translate_color(selection_font_color_s);
        list_font_color = translate_color(list_font_color_s);
        list_dir_font_color = translate_color(list_dir_font_color_s);

        while (!(disp = display_lock()))
            ;

        drawBg(disp);           //new
        drawBoxNumber(disp, 1); //new

        uint32_t *buffer = (uint32_t *)__get_buffer(disp); //fg disp = 2

        display_show(disp); //new

        backupSaveData(disp);

        while (!(disp = display_lock()))
            ;

        sprintf(pwd, "%s", "/");
        readSDcard(disp, "/");

        display_show(disp);
        //chr input coord
        x = 30;
        y = 30;

        position = 0;
        set = 1;
        sprintf(input_text, "");

        //sprite for chr input
        int fp = dfs_open("/sprites/n64controller.sprite");
        sprite_t *contr = malloc(dfs_size(fp));
        dfs_read(contr, 1, dfs_size(fp), fp);
        dfs_close(fp);

        //system main-loop with controller inputs-scan
        for ( ;; )
        {
            if (sound_on)
                sndUpdate();

            handleInput(disp, contr);

            if (playing == 1)
                playing = mp3_Update(buf_ptr, buf_size);

            if (input_mapping == file_manager)
                sleep(60);

            if (input_mapping == char_input)
            {
                while (!(disp = display_lock()))
                    ;

                graphics_draw_sprite(disp, 0, 0, contr);
                /* Set the text output color */
                graphics_set_color(0x0, 0xFFFFFFFF);

                chr_forecolor = graphics_make_color(0xFF, 0x14, 0x94, 0xFF); //pink
                graphics_set_color(chr_forecolor, chr_backcolor);

                graphics_draw_text(disp, 85, 55, "SETS");
                graphics_draw_text(disp, 94, 70, "1");  //u
                graphics_draw_text(disp, 104, 82, "2"); //r
                graphics_draw_text(disp, 94, 93, "3");  //d
                graphics_draw_text(disp, 82, 82, "4");  //l

                graphics_draw_text(disp, 208, 206, "press START");

                if (set == 1)
                    drawSet1(disp);
                if (set == 2)
                    drawSet2(disp);
                if (set == 3)
                    drawSet3(disp);
                if (set == 4)
                    drawSet4(disp);

                drawTextInput(disp, input_text);

                /* Force backbuffer flip */
                display_show(disp);
            } //mapping 2 chr input drawings

            //sleep(10);
        }
    }
    else
    {
        printf("Filesystem failed to start!\n");
        for ( ;; )
            ; //never leave!
    }
}
