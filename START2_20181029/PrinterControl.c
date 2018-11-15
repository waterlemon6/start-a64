#include <stdio.h>
#include "main.h"
#include "PrinterControl.h"

/* Sort the command that the printer sent. Correct, update, scan, etc. */
StateMachineTypeDef PrinterCommandSort(const unsigned char* cmd, ConfigMessageTypeDef *ConfigMessage)
{
    StateMachineTypeDef state = STATE_MACHINE_IDLE;
    if(cmd[0] != 0x01)
        return state;

    switch(cmd[1]) {
        case 0x08:
            if((cmd[4] & 0x01) == 0)
                state = STATE_MACHINE_CORRECTION_DARK;
            else
                state = STATE_MACHINE_CORRECTION_BRIGHT;
            break;

        case 0x09:
            state = STATE_MACHINE_UPDATE;
            break;

        case 0x0B:
            PrinterScanInformationSort(cmd[2], &cmd[5], ConfigMessage);
            state = STATE_MACHINE_SCAN;
            break;

        case 0x0D:
            state = STATE_MACHINE_LIGHTNESS_ADJUST;
            break;

        case 0x0E:
            state = STATE_MACHINE_LUMINANCE_SET;
            break;

        case 0x0F:
            state = STATE_MACHINE_VERSION;
            break;

        case 0x13:
            state = STATE_MACHINE_SWITCH_MODE;
            switch(cmd[4]) {
                case 0:
                    ConfigMessage->dpi = 300;
                    break;
                case 1:
                    ConfigMessage->dpi = 600;
                    break;
                case 2:
                    ConfigMessage->dpi = 200;
                    break;
                default:
                    break;
            }
            switch(cmd[3]) {
                case 0:
                    ConfigMessage->color = 'G';
                    break;
                case 1:
                    ConfigMessage->color = 'C';
                    break;
                case 2:
                    ConfigMessage->color = 'I';
                    break;
                default:
                    break;
            }
            break;

        case 0x14:
            state = STATE_MACHINE_SLEEP;
            break;

        default:
            break;
    }
    return state;
}

/* Check the command that the printer sent. 0x010A000000 + command + 0x010B000000 */
int PrinterCheckSPIMessageHeadAndTail(const unsigned char *head, const unsigned char *tail)
{
    if((head[0] == 0x01) && (head[1] == 0x0A) && (head[2] == 0x00) && (head[3] == 0x00) && (head[4] == 0x00))
        if((tail[0] == 0x01) && (tail[1] == 0x0B) && (tail[2] == 0x00) && (tail[3] == 0x00) && (tail[4] == 0x00))
            return 1;

    return 0;
}

/* Sort the scan information, including the mode and size of images.*/
void PrinterScanInformationSort(unsigned char mode, const unsigned char* size, ConfigMessageTypeDef *ConfigMessage)
{
    int temp = 0;

    ConfigMessage->topHeight = (size[0] << 8) + size[1];
    ConfigMessage->topLeftEdge = (size[2] << 8) + size[3];
    ConfigMessage->topRightEdge = (size[4] << 8) + size[5];
    ConfigMessage->topUpEdge = (size[6] << 8) + size[7];

    ConfigMessage->bottomHeight = (size[8] << 8) + size[9];
    ConfigMessage->bottomLeftEdge = (size[10] << 8) + size[11];
    ConfigMessage->bottomRightEdge = (size[12] << 8) + size[13];
    ConfigMessage->bottomUpEdge = (size[14] << 8) + size[15];

    if (ConfigMessage->topLeftEdge > ConfigMessage->topRightEdge)
        return;
    if (ConfigMessage->bottomLeftEdge > ConfigMessage->bottomRightEdge)
        return;

    switch (ConfigMessage->dpi) {
        case 200:
            ConfigMessage->topHeight = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->topHeight, 0, IMAGE_200DPI_HEIGHT / 3 - ConfigMessage->topUpEdge);
            ConfigMessage->topLeftEdge = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->topLeftEdge, CIS_EDGE_200DPI, IMAGE_200DPI_WIDTH + CIS_EDGE_200DPI);
            ConfigMessage->topRightEdge = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->topRightEdge, CIS_EDGE_200DPI, IMAGE_200DPI_WIDTH + CIS_EDGE_200DPI);
            ConfigMessage->bottomHeight = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->bottomHeight, 0, IMAGE_200DPI_HEIGHT / 3 - ConfigMessage->bottomUpEdge);
            ConfigMessage->bottomLeftEdge = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->bottomLeftEdge, CIS_EDGE_200DPI, IMAGE_200DPI_WIDTH + CIS_EDGE_200DPI);
            ConfigMessage->bottomRightEdge = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->bottomRightEdge, CIS_EDGE_200DPI, IMAGE_200DPI_WIDTH + CIS_EDGE_200DPI);
            temp = ConfigMessage->bottomLeftEdge;
            ConfigMessage->bottomLeftEdge = CIS_WIDTH_200DPI - ConfigMessage->bottomRightEdge;
            ConfigMessage->bottomRightEdge = CIS_WIDTH_200DPI - temp;
            break;
        case 300:
            ConfigMessage->topHeight = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->topHeight, 0, IMAGE_300DPI_HEIGHT / 3 - ConfigMessage->topUpEdge);
            ConfigMessage->topLeftEdge = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->topLeftEdge, CIS_EDGE_300DPI, IMAGE_300DPI_WIDTH + CIS_EDGE_300DPI);
            ConfigMessage->topRightEdge = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->topRightEdge, CIS_EDGE_300DPI, IMAGE_300DPI_WIDTH + CIS_EDGE_300DPI);
            ConfigMessage->bottomHeight = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->bottomHeight, 0, IMAGE_300DPI_HEIGHT / 3 - ConfigMessage->bottomUpEdge);
            ConfigMessage->bottomLeftEdge = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->bottomLeftEdge, CIS_EDGE_300DPI, IMAGE_300DPI_WIDTH + CIS_EDGE_300DPI);
            ConfigMessage->bottomRightEdge = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->bottomRightEdge, CIS_EDGE_300DPI, IMAGE_300DPI_WIDTH + CIS_EDGE_300DPI);
            temp = ConfigMessage->bottomLeftEdge;
            ConfigMessage->bottomLeftEdge = CIS_WIDTH_300DPI - ConfigMessage->bottomRightEdge;
            ConfigMessage->bottomRightEdge = CIS_WIDTH_300DPI - temp;
            break;
        case 600:
            ConfigMessage->topHeight = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->topHeight, 0, IMAGE_600DPI_HEIGHT / 6 - ConfigMessage->topUpEdge);
            ConfigMessage->topLeftEdge = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->topLeftEdge, CIS_EDGE_600DPI, IMAGE_600DPI_WIDTH + CIS_EDGE_600DPI);
            ConfigMessage->topRightEdge = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->topRightEdge, CIS_EDGE_600DPI, IMAGE_600DPI_WIDTH + CIS_EDGE_600DPI);
            ConfigMessage->bottomHeight = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->bottomHeight, 0, IMAGE_600DPI_HEIGHT / 6 - ConfigMessage->bottomUpEdge);
            ConfigMessage->bottomLeftEdge = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->bottomLeftEdge, CIS_EDGE_600DPI, IMAGE_600DPI_WIDTH + CIS_EDGE_600DPI);
            ConfigMessage->bottomRightEdge = (unsigned short) LIMIT_MIN_MAX(ConfigMessage->bottomRightEdge, CIS_EDGE_600DPI, IMAGE_600DPI_WIDTH + CIS_EDGE_600DPI);
            temp = ConfigMessage->bottomLeftEdge;
            ConfigMessage->bottomLeftEdge = CIS_WIDTH_600DPI - ConfigMessage->bottomRightEdge;
            ConfigMessage->bottomRightEdge = CIS_WIDTH_600DPI - temp;
            break;
        default:
            return;
    }

    switch((mode >> 4) & 0x03) {
        case 0:
            ConfigMessage->pageOne = PAGE_TOP;
            ConfigMessage->pageTwo = PAGE_BOTTOM;
            ConfigMessage->maxLines = (ConfigMessage->topHeight + ConfigMessage->topUpEdge) > (ConfigMessage->bottomHeight + ConfigMessage->bottomUpEdge) ?
                                     (ConfigMessage->topHeight + ConfigMessage->topUpEdge) : (ConfigMessage->bottomHeight + ConfigMessage->bottomUpEdge);
            break;
        case 1:
            ConfigMessage->pageOne = PAGE_TOP;
            ConfigMessage->pageTwo = PAGE_NULL;
            ConfigMessage->maxLines = ConfigMessage->topHeight + ConfigMessage->topUpEdge;
            break;
        case 2:
            ConfigMessage->pageOne = PAGE_BOTTOM;
            ConfigMessage->pageTwo = PAGE_NULL;
            ConfigMessage->maxLines = ConfigMessage->bottomHeight + ConfigMessage->bottomUpEdge;
            break;
        default:
            ConfigMessage->pageOne = PAGE_NULL;
            ConfigMessage->pageTwo = PAGE_NULL;
            return;
    }
}

void PrinterScanShowConfigMessage(ConfigMessageTypeDef *ConfigMessage)
{
    PR("----Scan Command----\n");
    PR("----pageOne: %d  pageTwo: %d\n", ConfigMessage->pageOne, ConfigMessage->pageTwo);
    PR("----colorSpace: %c\n", ConfigMessage->color);
    PR("----dpi: %d\n", ConfigMessage->dpi);
    PR("----\n");
    PR("----topHeight: %d\n", ConfigMessage->topHeight);
    PR("----topLeftEdge: %d\n", ConfigMessage->topLeftEdge);
    PR("----topRightEdge: %d\n", ConfigMessage->topRightEdge);
    PR("----topUpEdge: %d\n", ConfigMessage->topUpEdge);
    PR("----bottomHeight: %d\n", ConfigMessage->bottomHeight);
    PR("----bottomLeftEdge: %d\n", ConfigMessage->bottomLeftEdge);
    PR("----bottomRightEdge: %d\n", ConfigMessage->bottomRightEdge);
    PR("----bottomUpEdge: %d\n", ConfigMessage->bottomUpEdge);
    PR("----\n");
    PR("----maxLines: %d\n", ConfigMessage->maxLines);
    PR("--------End---------\n");
}

