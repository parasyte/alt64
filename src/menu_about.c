
#include <libdragon.h>
#include <stdio.h>
#include "types.h"
#include "menu.h"
#include "version.h"
#include "main.h"
#include "everdrive.h"


void menu_about(display_context_t disp)
{
    char version_str[32];
    char firmware_str[32];

    sprintf(version_str, "Altra64: v%s", Altra64_GetVersionString());
    printText(version_str, 9, 8, disp);
    sprintf(firmware_str, "ED64 firmware: v%03x", evd_getFirmVersion());
    printText(firmware_str, 9, -1, disp);
    printText("by JonesAlmighty", 9, -1, disp);
    printText(" ", 9, -1, disp);
    printText("Based on ALT64", 9, -1, disp);
    printText("By Saturnu", 9, -1, disp);
    printText(" ", 9, -1, disp);
    printText("credits to:", 9, -1, disp);
    printText("Jay Oster", 9, -1, disp);
    printText("Krikzz", 9, -1, disp);
    printText("Richard Weick", 9, -1, disp);
    printText("ChillyWilly", 9, -1, disp);
    printText("ShaunTaylor", 9, -1, disp);
    printText("Conle", 9, -1, disp);
}
