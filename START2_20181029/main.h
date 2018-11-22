#ifndef __MAIN_H__
#define __MAIN_H__

#include "/usr/local/libjpeg-turbo_arm/include/jpeglib.h"

/* DEBUG LEVEL
 * 0 - DEBUG VERSION. PRINT SYSTEM INFORMATION, TIMES TAKEN IN EACH COMMAND, PACKAGES SENT VIA SPI.
 * 1 - DEBUG VERSION. PRINT SYSTEM INFORMATION.
 * 2 - RELEASE VERSION. PRINT NOTHING.
 */
#define DEBUG_LEVEL 0

#if DEBUG_LEVEL <= 1
#define PR(...) printf(__VA_ARGS__)
#else
#define PR(...)
#endif

/* The width and height of the video port, defined in video module. */
#define VIDEO_PORT_WIDTH        5184
#define VIDEO_PORT_HEIGHT       8000
#define VIDEO_PORT_FRAME_HEIGHT 7800

/* The width and edge of the cis. CIS_WIDTH = CIS_EDGE + CIS_AVAILABLE_WIDTH + CIS_EDGE. */
#define CIS_WIDTH_200DPI        1728
#define CIS_EDGE_200DPI         14

#define CIS_WIDTH_300DPI        2592
#define CIS_EDGE_300DPI         20

#define CIS_WIDTH_600DPI        5184
#define CIS_EDGE_600DPI         40

/* Available width per page, decided by cis and video port.
   Available height per page, decided by video port and frame height. */
#define IMAGE_200DPI_WIDTH  (CIS_WIDTH_200DPI - 2 * CIS_EDGE_200DPI)
#define IMAGE_200DPI_HEIGHT VIDEO_PORT_HEIGHT

#define IMAGE_300DPI_WIDTH  (CIS_WIDTH_300DPI - 2 * CIS_EDGE_300DPI)
#define IMAGE_300DPI_HEIGHT (VIDEO_PORT_HEIGHT + VIDEO_PORT_FRAME_HEIGHT)

#define IMAGE_600DPI_WIDTH  (CIS_WIDTH_600DPI - 2 * CIS_EDGE_600DPI)
#define IMAGE_600DPI_HEIGHT (VIDEO_PORT_HEIGHT + 5 * VIDEO_PORT_FRAME_HEIGHT)

#define LIMIT_MIN_MAX(x, min, max) (((x)<=(min)) ? (min) : (((x)>=(max))?(max):(x)))

typedef struct ImageAttributions_t {
    char color;
    int dpi;
    int depth;
    int width;
    int height;

    int leftEdge;
    int rightEdge;
    int topEdge;
    int bottomEdge;
}ImageAttributionsTypeDef;

int MainProcess(int dpi, char color, int videoPortOffset);
void *ThreadSendData(void);

void StateMachineSetTime(void);
void StateMachineGetTime(void);

#endif
