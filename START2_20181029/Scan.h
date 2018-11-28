#ifndef __SCAN_H__
#define __SCAN_H__

#include "main.h"
#include "PrinterControl.h"

#define SCAN_TIME_PER_LINE_200DPI 167 // 8000000 / 1333 = 6000 lines, 1000000 / 6000 = 167 usecs.
#define SCAN_TIME_PER_LINE_200DPI_ONE_CHANNEL 300 // 8000000 / 2400 = 3333 lines, 1000000 / 3333 = 300 usecs
#define SCAN_TIME_PER_LINE_300DPI 250 // 8000000 / 2000 = 4000 lines, 1000000 / 4000 = 250 usecs.
#define SCAN_TIME_PER_LINE_600DPI 500 // 8000000 / 4000 = 2000 lines, 1000000 / 2000 = 500 usecs.

typedef struct CompressProcess_t{
    ImageAttributionsTypeDef imageAttr;

    int step;                       // Compress step.
    long scanLines;                 // Scan lines, decide by the scan time per line of FPGA.
    long shiftLines;                 // Shift lines, lines after memory copying in queue.
    long compressLines;              // Compress lines, lines processed by JPEG.

    long maxScanLines;               // Max Scan Lines of the top page and the bottom page.
    int frame;                      // How many frames will be sampled this time.
    int rowsPerLine;
    unsigned char *imagePosition;   // Image position of the original picture, used in memory copy.
}CompressProcessTypeDef;

enum {
    COMPRESS_STEP_IDLE = 0,
    COMPRESS_STEP_P1_ENCODE,
    COMPRESS_STEP_P2_ENCODE
};

void ScanTimeSet(void);
long ScanLinesGet(int dpi, int depth);

int CompressProcessPrepare(unsigned char page, ConfigMessageTypeDef *ConfigMessage, CompressProcessTypeDef *CompressProcess, int videoPortOffset);
void CompressProcessShiftPicture(unsigned char *pic1, unsigned char *pic2, unsigned char *src, ImageAttributionsTypeDef *attr);
void CompressProcessArrangeOneLine(const unsigned char *src, unsigned char *dst, ImageAttributionsTypeDef *attr);

#endif
