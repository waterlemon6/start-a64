#ifndef __PRINTER_CONTROL_H__
#define __PRINTER_CONTROL_H__

typedef struct ConfigMessage_t {
    unsigned char pageOne;
    unsigned char pageTwo;

    char color;
    int dpi;

    int topHeight;
    int topLeftEdge;
    int topRightEdge;
    int topUpEdge;
    int bottomHeight;
    int bottomLeftEdge;
    int bottomRightEdge;
    int bottomUpEdge;

    int maxLines;
}ConfigMessageTypeDef;

enum {
    PAGE_NULL = 0,
    PAGE_TOP,
    PAGE_BOTTOM
};

typedef enum {
    STATE_MACHINE_IDLE = 0,
    STATE_MACHINE_VERSION,
    STATE_MACHINE_LUMINANCE_SET,
    STATE_MACHINE_SCAN,
    STATE_MACHINE_LIGHTNESS_ADJUST,
    STATE_MACHINE_CORRECTION_DARK,
    STATE_MACHINE_CORRECTION_BRIGHT,
    STATE_MACHINE_UPDATE,
    STATE_MACHINE_SWITCH_MODE,
    STATE_MACHINE_SLEEP
}StateMachineTypeDef;

StateMachineTypeDef PrinterCommandSort(const unsigned char* cmd, ConfigMessageTypeDef *ConfigMessage);
int PrinterCheckSPIMessageHeadAndTail(const unsigned char *head, const unsigned char *tail);
void PrinterScanInformationSort(unsigned char mode, const unsigned char* size, ConfigMessageTypeDef *ConfigMessage);
void PrinterScanShowConfigMessage(ConfigMessageTypeDef *ConfigMessage);

#endif
