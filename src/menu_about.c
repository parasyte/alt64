
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
    printText("Conle        Z: Page 2", 9, -1, disp);
} //TODO: make scrolling text, should include libraries used.
void menu_controls(display_context_t disp)
{
    printText("          - Controls -", 4, 4, disp);
    printText(" ", 4, -1, disp);
    printText("      L: brings up the mempak", 4, -1, disp);
    printText("        menu", 5, -1, disp);
    printText(" ", 4, -1, disp);
    printText("      Z: about screen", 4, -1, disp);
    printText(" ", 4, -1, disp);
    printText("      A: start rom/directory", 4, -1, disp);
    printText("         mempak", 4, -1, disp);
    printText(" ", 4, -1, disp);
    printText("      B: back/cancel", 4, -1, disp);
    printText(" ", 4, -1, disp);
    printText("  START: start last rom", 4, -1, disp);
    printText(" ", 4, -1, disp);
    printText(" C-left: rom info/mempak", 4, -1, disp);
    printText("         content view", 4, -1, disp);
    printText(" ", 4, -1, disp);
    printText("C-right: rom config creen", 4, -1, disp);
    printText(" ", 4, -1, disp);
    printText("   C-up: view full filename", 4, -1, disp);
    printText(" ", 4, -1, disp);
    printText(" C-down: Toplist 15", 4, -1, disp);

}