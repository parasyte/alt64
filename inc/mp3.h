
#ifndef _MP3_H_
#define _MP3_H_

void start_mp3(char *fname, long long *samples, int *rate, int *channels);
void stop_mp3(void);
int update_mp3(char *buf, int bytes);

#endif // _MP3_H_
