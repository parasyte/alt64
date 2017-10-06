//
// Copyright (c) 2017 The Altra64 project contributors
// Portions (c) 2013 saturnu (Alt64) based on libdragon, Neo64Menu, ED64IO, libn64-hkz, libmikmod
// See LICENSE file in the project root for full license information.
//

#include "strlib.h"

char *strcpytrim(char *d, // destination
                 char *s, // source
                 int mode,
                 char *delim
                 ) {
    char *o = d; // save orig
    char *e = 0; // end space ptr.
    char dtab[256] = {0};
    if (!s || !d) return 0;

    if (!delim) delim = " \t\n\f";
    while (*delim) 
        dtab[*delim++] = 1;

    while ( (*d = *s++) != 0 ) { 
        if (!dtab[*d]) { // Not a match char
            e = 0;       // Reset end pointer
        } else {
            if (!e) e = d;  // Found first match.

            if ( mode == STRLIB_MODE_ALL || ((mode != STRLIB_MODE_RIGHT) && (d == o)) ) 
                continue;
        }
        d++;
    }
    if (mode != STRLIB_MODE_LEFT && e) { // for everything but trim_left, delete trailing matches.
        *e = 0;
    }
    return o;
}

// perhaps these could be inlined in strlib.h
char *strtriml(char *d, char *s) { return strcpytrim(d, s, STRLIB_MODE_LEFT, 0); }
char *strtrimr(char *d, char *s) { return strcpytrim(d, s, STRLIB_MODE_RIGHT, 0); }
char *strtrim(char *d, char *s) { return strcpytrim(d, s, STRLIB_MODE_BOTH, 0); }
char *strstrlibkill(char *d, char *s) { return strcpytrim(d, s, STRLIB_MODE_ALL, 0); }

char *triml(char *s) { return strcpytrim(s, s, STRLIB_MODE_LEFT, 0); }
char *trimr(char *s) { return strcpytrim(s, s, STRLIB_MODE_RIGHT, 0); }
char *trim(char *s) { return strcpytrim(s, s, STRLIB_MODE_BOTH, 0); }
char *strlibkill(char *s) { return strcpytrim(s, s, STRLIB_MODE_ALL, 0); }