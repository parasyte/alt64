#include <stdarg.h>
#include <stdio.h>
#include <libdragon.h>
#include "types.h"
#include "debug.h"
#include "menu.h"
#include "sys.h"

void dbg_printf(display_context_t disp, const char *fmt, ...)
{
    char buf[256] = {0};
    setbuf(stderr, buf);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    u8 tmp[256] = {0};
    sprintf(tmp, "%s", buf);
    printText(tmp, 3, -1, disp);
    display_show(disp);
    sleep(5000);
}