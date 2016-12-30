#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <mad.h>
#include "fat.h"
#include "mp3.h"

#include <libdragon.h>

static struct mad_stream Stream;
static struct mad_header Header;
static struct mad_frame Frame;
static struct mad_synth Synth;
static mad_timer_t Timer;

typedef struct  {
    short left;
    short right;
} Sample;

static int eos;

#define INPUT_BUFFER_SIZE 2048
static unsigned char fileBuffer[INPUT_BUFFER_SIZE];
static unsigned char readBuffer[INPUT_BUFFER_SIZE];
static int useReadBuffer;
static int readPos;
static unsigned int readLen;
static int samplesRead;

char * mp3File;
char * mp3Fd;
long mp3File_fptr=0;
int mp3File_fsize;

extern int gBrowser;
extern char path[1024];

extern void c2wstrcpy(void *dst, void *src);
extern void c2wstrcat(void *dst, void *src);

static int mp3_seek(char* fd, int offset, int whence) {
    //todo filesize and mp3File_fptr;
    long offs = 0;
    // libff routine
    switch (whence)
    {
        case SEEK_SET:
            offs = offset;
            break;
        case SEEK_CUR:
            offs = mp3File_fptr + offset;
            break;
        case SEEK_END:
            offs = mp3File_fsize + offset;
            break;
    }
    //f_lseek(&mp3File, offs);
    mp3File_fptr=offs;

    return offs;
}

static int mp3_size(char* fd) {
    FatRecord rec_tmpf;
    u8 resp=0;
    resp = fatOpenFileByeName(fd, 0); //err if not found ^^

    int fsize =  file.sec_available*512; //fsize in bytes
    mp3File_fsize = fsize;
    //todo filesize
    return mp3File_fsize;
}

static void _f_read(char* fname, unsigned char *readBuffer, int size){
/*
    FatRecord rec_tmpf;
    u8 resp=0;
    resp = fatOpenFileByeName(fname, 0); //err if not found ^^

    int fsize =  file.sec_available*512; //fsize in bytes
    mp3File_fsize = fsize;

    //injecting in buffer... slow but working :/
    if(file.sec_available*512>=size){
    resp = fatReadPartialFile(readBuffer, size/512, mp3File_fptr);
    //resp = fatReadFile(readBuffer+mp3File_fptr, size/512);//file.sec_available);
    mp3File_fptr+=size;

    }
    //dma_write_s(buffer, 0xb0000000, fsize);
*/
}

static int mp3_read(char* fd, unsigned char *ptr, int size)
{
    int ts=size;
    _f_read(fd, ptr, size);
    return ts;
}

static int id3_tag_size(unsigned char const *buf, int remaining) {
    int size;

    if (remaining < 10)
        return 0;

    if (!strncmp((char*)buf, "ID3", 3) || !strncmp((char*)buf, "ea3", 3)) //skip past id3v2 header, which can cause a false sync to be found
    {
        //get the real size from the syncsafe int
        size = buf[6];
        size = (size<<7) | buf[7];
        size = (size<<7) | buf[8];
        size = (size<<7) | buf[9];

        size += 10;

        if (buf[5] & 0x10) //has footer
            size += 10;
    }
    return size;
}

//Seek next valid frame after ID3/EA3 header
//NOTE: adapted from Music prx 0.55 source
//      credit goes to joek2100.
static int MP3_SkipHdr(char* fd)
{
    int offset = 0;
    unsigned char buf[1024];
    unsigned char *pBuffer;
    int i;
    int size = 0;

    offset = mp3_seek(fd, 0, SEEK_CUR);
    mp3_read(fd, buf, sizeof(buf));
    if (!strncmp((char*)buf, "ID3", 3) || !strncmp((char*)buf, "ea3", 3)) //skip past id3v2 header, which can cause a false sync to be found
    {
        //get the real size from the syncsafe int
        size = buf[6];
        size = (size<<7) | buf[7];
        size = (size<<7) | buf[8];
        size = (size<<7) | buf[9];

        size += 10;

        if (buf[5] & 0x10) //has footer
            size += 10;

        offset += size;
    }
    mp3_seek(fd, offset, SEEK_SET);

    //now seek for a sync
    while(1) {
        offset = mp3_seek(fd, 0, SEEK_CUR);
        size = mp3_read(fd, buf, sizeof(buf));

        if (size <= 2)//at end of file
            return -1;

        if (!strncmp((char*)buf, "EA3", 3)) //oma mp3 files have non-safe ints in the EA3 header
        {
            mp3_seek(fd, (buf[4]<<8)+buf[5], SEEK_CUR);
            continue;
        }

        pBuffer = buf;
        for( i = 0; i < size; i++) {
            //if this is a valid frame sync (0xe0 is for mpeg version 2.5,2+1)
            if ( (pBuffer[i] == 0xff) && ((pBuffer[i+1] & 0xE0) == 0xE0) ) {
                offset += i;
                mp3_seek(fd, offset, SEEK_SET);
                return offset;
            }
        }
        //go back two bytes to catch any syncs that on the boundary
        mp3_seek(fd, -2, SEEK_CUR);
    }
}

static short convertSample(mad_fixed_t Fixed) {
    /* Clipping */
    if (Fixed >= MAD_F_ONE)
        return (32767);
    if (Fixed <= -MAD_F_ONE)
        return (-32768);

    /* Conversion. */
    Fixed = Fixed >> (MAD_F_FRACBITS - 15);

    return ((short)Fixed);
}

static int fillFileBuffer() {
    int leftOver = Stream.bufend - Stream.next_frame;
    int want = INPUT_BUFFER_SIZE - leftOver;

    // move left-over bytes
    if (leftOver > 0)
        memmove(fileBuffer, fileBuffer + want, leftOver);

    // fill remainder of buffer
    unsigned char* bufferPos = fileBuffer + leftOver;
    while (want > 0) {
        int got = mp3_read(mp3Fd, bufferPos, want);
        if (got <= 0)
            return 1; // EOF

        want -= got;
        bufferPos += got;
    }
    return 0;
}

static void decode() {
    while (mad_frame_decode(&Frame, &Stream) == -1) {
        if ((Stream.error == MAD_ERROR_BUFLEN) || (Stream.error == MAD_ERROR_BUFPTR)) {
            if (fillFileBuffer()) {
                eos = 1;
                break;
            }
            mad_stream_buffer(&Stream, fileBuffer, INPUT_BUFFER_SIZE);
        }
        else if (Stream.error == MAD_ERROR_LOSTSYNC) {
            /* LOSTSYNC - due to ID3 tags? */
            int tagsize = id3_tag_size(Stream.this_frame, Stream.bufend - Stream.this_frame);
            if (tagsize > 0) {
                mad_stream_skip (&Stream, tagsize);
                continue;
            }
        }
    }

    mad_timer_add(&Timer, Frame.header.duration);
    mad_synth_frame(&Synth, &Frame);
}

static void convertLeftSamples(Sample* first, Sample* last, const mad_fixed_t* src) {
    for (Sample *dst = first; dst != last; ++dst)
        dst->left = convertSample(*src++);
}

static void convertRightSamples(Sample* first, Sample* last, const mad_fixed_t* src) {
    for (Sample *dst = first; dst != last; ++dst)
        dst->right = convertSample(*src++);
}

static void MP3_Callback(void *buffer, unsigned int samplesToWrite) {
    Sample *destination = (Sample*)buffer;

    while (samplesToWrite > 0) {
        while (!eos && (Synth.pcm.length == 0))
            decode();

        if (eos) {
            // done
            memset(destination, 0, samplesToWrite*4);
            break;
        }

        unsigned int samplesAvailable = Synth.pcm.length - samplesRead;
        if (samplesAvailable > samplesToWrite) {
            convertLeftSamples(destination, destination + samplesToWrite, &Synth.pcm.samples[0][samplesRead]);
            convertRightSamples(destination, destination + samplesToWrite, &Synth.pcm.samples[1][samplesRead]);

            samplesRead += samplesToWrite;
            samplesToWrite = 0;
        }
        else {
            convertLeftSamples(destination, destination + samplesAvailable, &Synth.pcm.samples[0][samplesRead]);
            convertRightSamples(destination, destination + samplesAvailable, &Synth.pcm.samples[1][samplesRead]);

            destination += samplesAvailable;
            samplesToWrite -= samplesAvailable;

            samplesRead = 0;
            decode();
        }
    }
}

static void MP3_Init() {
    /* First the structures used by libmad must be initialized. */
    mad_stream_init(&Stream);
    mad_header_init(&Header);
    mad_frame_init(&Frame);
    mad_synth_init(&Synth);
    mad_timer_reset(&Timer);
}

static void MP3_Exit() {
    mad_synth_finish(&Synth);
    mad_header_finish(&Header);
    mad_frame_finish(&Frame);
    mad_stream_finish(&Stream);
}

static void MP3_GetInfo(long long *samples, int *rate) {
    unsigned long FrameCount = 0;
    int bufferSize = 1024*512;
    unsigned char *localBuffer;
    long red = 0;
    double totalBitrate = 0.0;
    double mediumBitrate = 0.0;
    struct mad_stream stream;
    struct mad_header header;
    int size = mp3_size(mp3Fd);
    long count = size;

    mad_stream_init (&stream);
    mad_header_init (&header);

    localBuffer = (unsigned char *)malloc(bufferSize);

    for (int i=0; i<3; i++) {
        memset(localBuffer, 0, bufferSize);

        if (count > bufferSize)
            red = mp3_read(mp3Fd, localBuffer, bufferSize);
        else
            red = mp3_read(mp3Fd, localBuffer, count);
        count -= red;
        if (!red)
            break; // ran out of data

        mad_stream_buffer (&stream, localBuffer, red);

        while (1) {
            if (mad_header_decode(&header, &stream) == -1) {
                if (stream.buffer == NULL || stream.error == MAD_ERROR_BUFLEN) {
                    break;
                }
                else if (MAD_RECOVERABLE(stream.error)) {
                    continue;
                }
                else {
                    break;
                }
            }
            if (FrameCount++ == 0)
                *rate = header.samplerate;
            totalBitrate += header.bitrate;
        }
    }

    mediumBitrate = totalBitrate / (double)FrameCount;
    int secs = size * 8 / mediumBitrate;
    *samples = *rate * secs;

    mad_header_finish (&header);
    mad_stream_finish (&stream);

    if (localBuffer)
        free(localBuffer);

    mp3_seek(mp3Fd, 0, SEEK_SET);
}


void mp3_Start(char *fname, long long *samples, int *rate, int *channels) {
    sprintf(mp3Fd, "%s", fname);

    //if (mp3Fd[0]!=0)
    //{
        useReadBuffer = 0;
        MP3_GetInfo(samples, rate);
        *channels = 2;

        MP3_Init();
        MP3_SkipHdr(mp3Fd);
        eos = readLen = readPos = 0;
        useReadBuffer = 1;
        return;
    //}

    //*samples = 0;
}

void mp3_Stop(void) {
    MP3_Exit();
    mp3File_fptr=0;
/*
    if (mp3Fd > 0)
    {
        if (gBrowser)
            f_close(&mp3File);
        else
            dfs_close(mp3Fd);
    }
    mp3Fd = -1;
    */
}

int mp3_Update(char *buf, int bytes) {
    MP3_Callback(buf, bytes/4);
    return eos ? 0 : 1;
}
