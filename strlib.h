#ifndef STRLIB_H_
enum strtrim_mode_t {
    STRLIB_MODE_ALL       = 0, 
    STRLIB_MODE_RIGHT     = 0x01, 
    STRLIB_MODE_LEFT      = 0x02, 
    STRLIB_MODE_BOTH      = 0x03
};

char *strcpytrim(char *d, // destination
                 char *s, // source
                 int mode,
                 char *delim
                 );

char *strtriml(char *d, char *s);
char *strtrimr(char *d, char *s);
char *strtrim(char *d, char *s); 
char *strstrlibkill(char *d, char *s);

char *triml(char *s);
char *trimr(char *s);
char *trim(char *s);
char *strlibkill(char *s);
#endif
