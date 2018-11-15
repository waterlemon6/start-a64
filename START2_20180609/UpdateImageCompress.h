#ifndef __UPDATE_IMAGE_COMPRESS_H__
#define __UPDATE_IMAGE_COMPRESS_H__

#define UPDATE_MAGIC_NUMBER 20180507

#define UPDATE_MARK_SHELL           1
#define UPDATE_MARK_JTAG_MODULE     2
#define UPDATE_MARK_JTAG_EXE        3
#define UPDATE_MARK_RBF             4
#define UPDATE_MARK_VIDEO_MODULE    5
#define UPDATE_MARK_SPI_MODULE      6
#define UPDATE_MARK_LED_MODULE      7
#define UPDATE_MARK_MAIN_EXE        8
#define UPDATE_MARK_9               9
#define UPDATE_MARK_10              10

struct imageHeader {
    unsigned int magic;
    unsigned int length; // Including header
    unsigned int check;
    unsigned int sectionCount;
    unsigned int sectionOffset[10];
};

struct sectionHeader {
    unsigned int length; // Including header
    unsigned int mark;
};

unsigned int GetCRC32CheckSum(unsigned char *pchMessage, unsigned int dwLength);

unsigned char *ReadOneSection(struct imageHeader *Image, const char *path, unsigned int mark);

#endif
