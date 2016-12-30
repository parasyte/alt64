
#ifndef _MP3_H_
#define _MP3_H_

void mp3_Start(char *fname, long long *samples, int *rate, int *channels);
void mp3_Stop(void);
int mp3_Update(char *buf, int bytes);

#endif // _MP3_H_
