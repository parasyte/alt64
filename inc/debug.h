
#ifdef DEBUG
    #define TRACEF(disp, text, ...)    dbg_printf(disp, text, __VA_ARGS__);
    #define TRACE(disp, text)   printText(text, 3, -1, disp);
#else
    #define TRACEF(disp, text, ...)    do { if (0)  dbg_printf(disp, text, __VA_ARGS__);  } while (0)
    #define TRACE(disp, text)    do { if (0) printText(text, 3, -1, disp); } while (0)
#endif /* DEBUG */

#include <stdarg.h>
#include <stdio.h>

void dbg_printf(display_context_t disp, const char *fmt, ...)
{
    char buf[32];
    setbuf(stderr, buf);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    u8 tmp[32];
    sprintf(tmp, "%s", buf);
    printText(tmp, 3, -1, disp);
}