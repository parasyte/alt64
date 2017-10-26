//
// Copyright (c) 2017 The Altra64 project contributors
// See LICENSE file in the project root for full license information.
//

#ifndef _DEBUG_H
#define	_DEBUG_H

#ifdef DEBUG
    #define TRACEF(disp, text, ...)    dbg_printf(disp, text, __VA_ARGS__);
    #define TRACE(disp, text)   printText(text, 3, -1, disp); 
#else
    #define TRACEF(disp, text, ...)    do { if (0)  dbg_printf(disp, text, __VA_ARGS__); sleep(5000); } while (0)
    #define TRACE(disp, text)    do { if (0) printText(text, 3, -1, disp); } didplay_show(disp); sleep(5000); while (0)
#endif

void dbg_printf(display_context_t disp, const char *fmt, ...);

#endif
