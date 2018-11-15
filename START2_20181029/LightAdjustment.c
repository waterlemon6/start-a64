#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "LightAdjustment.h"
#include "KernelVideo.h"
#include "Correction.h"

unsigned char GetMaxPixelInOneLine(const unsigned char *src, int length)
{
    unsigned char max = 0;
    for(int i = 0; i < length; i++) {
        if(src[i] > max)
            max = src[i];
    }
    return max;
}

void GetMaxPixelInColorSpace(LightAdjustmentTypeDef *handler, unsigned char *originalImage, int height)
{
    int topPageLeftEdge = 0;
    int bottomPageLeftEdge = 0;
    int availableWidth = 0;

    switch (handler->imageAttr.dpi) {
        case 200:
            topPageLeftEdge = CIS_WIDTH_200DPI + CIS_EDGE_200DPI;
            bottomPageLeftEdge = CIS_EDGE_200DPI;
            availableWidth = IMAGE_200DPI_WIDTH;
            break;
        case 300:
            topPageLeftEdge = CIS_WIDTH_300DPI + CIS_EDGE_300DPI;
            bottomPageLeftEdge = CIS_EDGE_300DPI;
            availableWidth = IMAGE_300DPI_WIDTH;
            break;
        case 600:
            topPageLeftEdge = CIS_WIDTH_600DPI + CIS_EDGE_600DPI;
            bottomPageLeftEdge = CIS_EDGE_600DPI;
            availableWidth = IMAGE_600DPI_WIDTH;
            break;
        default:
            break;
    }

    switch (handler->imageAttr.color) {
        case 'C':
            switch (handler->imageAttr.dpi) {
                case 200:
                case 300:
                    VerticalSample(originalImage + VIDEO_PORT_WIDTH * 1, handler->sampleR, handler->sampleWidth, height, handler->imageAttr.depth);
                    VerticalSample(originalImage + VIDEO_PORT_WIDTH * 2, handler->sampleG, handler->sampleWidth, height, handler->imageAttr.depth);
                    VerticalSample(originalImage, handler->sampleB, handler->sampleWidth, height, handler->imageAttr.depth);
                    break;
                case 600:
                    VerticalSample(originalImage + VIDEO_PORT_WIDTH * 2, handler->sampleR, handler->sampleWidth, height, handler->imageAttr.depth * 2);
                    VerticalSample(originalImage + VIDEO_PORT_WIDTH * 4, handler->sampleG, handler->sampleWidth, height, handler->imageAttr.depth * 2);
                    VerticalSample(originalImage, handler->sampleB, handler->sampleWidth, height, handler->imageAttr.depth * 2);
                    break;
                default:
                    break;
            }
            handler->maxTopR = GetMaxPixelInOneLine(handler->sampleR + topPageLeftEdge, availableWidth);
            handler->maxBottomR = GetMaxPixelInOneLine(handler->sampleR + bottomPageLeftEdge, availableWidth);
            handler->maxTopG = GetMaxPixelInOneLine(handler->sampleG + topPageLeftEdge, availableWidth);
            handler->maxBottomG = GetMaxPixelInOneLine(handler->sampleG + bottomPageLeftEdge, availableWidth);
            handler->maxTopB = GetMaxPixelInOneLine(handler->sampleB + topPageLeftEdge, availableWidth);
            handler->maxBottomB = GetMaxPixelInOneLine(handler->sampleB + bottomPageLeftEdge, availableWidth);
            handler->maxTopIR = 0;
            handler->maxBottomIR = 0;
            break;
        case 'G':
            switch (handler->imageAttr.dpi) {
                case 200:
                case 300:
                    VerticalSample(originalImage, handler->sampleR, handler->sampleWidth, height, handler->imageAttr.depth);
                    break;
                case 600:
                    VerticalSample(originalImage, handler->sampleR, handler->sampleWidth, height, handler->imageAttr.depth * 2);
                    break;
                default:
                    break;
            }
            handler->maxTopR = GetMaxPixelInOneLine(handler->sampleR + topPageLeftEdge, availableWidth);
            handler->maxBottomR = GetMaxPixelInOneLine(handler->sampleR + bottomPageLeftEdge, availableWidth);
            handler->maxTopG = 0;
            handler->maxBottomG = 0;
            handler->maxTopB = 0;
            handler->maxBottomB = 0;
            handler->maxTopIR = 0;
            handler->maxBottomIR = 0;
            break;
        case 'I':
            switch (handler->imageAttr.dpi) {
                case 200:
                case 300:
                    VerticalSample(originalImage, handler->sampleIR, handler->sampleWidth, height, handler->imageAttr.depth);
                    break;
                case 600:
                    VerticalSample(originalImage, handler->sampleIR, handler->sampleWidth, height, handler->imageAttr.depth * 2);
                    break;
                default:
                    break;
            }
            handler->maxTopIR = GetMaxPixelInOneLine(handler->sampleIR + topPageLeftEdge, availableWidth);
            handler->maxBottomIR = GetMaxPixelInOneLine(handler->sampleIR + bottomPageLeftEdge, availableWidth);
            handler->maxTopR = 0;
            handler->maxBottomR = 0;
            handler->maxTopG = 0;
            handler->maxBottomG = 0;
            handler->maxTopB = 0;
            handler->maxBottomB = 0;
            break;
        default:
            break;
    }
}

int LightAdjustmentJudge(LightAdjustmentTypeDef *handler)
{
    handler->lightTopR += (handler->aim - handler->maxTopR) * handler->proportion;
    handler->lightTopR = (unsigned short) LIMIT_MIN_MAX(handler->lightTopR, 100, handler->maxLight);
    handler->lightTopG += (handler->aim - handler->maxTopG) * handler->proportion;
    handler->lightTopG = (unsigned short) LIMIT_MIN_MAX(handler->lightTopG, 100, handler->maxLight);
    handler->lightTopB += (handler->aim - handler->maxTopB) * handler->proportion;
    handler->lightTopB = (unsigned short) LIMIT_MIN_MAX(handler->lightTopB, 100, handler->maxLight);
    handler->lightTopIR += (handler->aim - handler->maxTopIR) * handler->proportion;
    handler->lightTopIR = (unsigned short) LIMIT_MIN_MAX(handler->lightTopIR, 100, handler->maxLight);

    handler->lightBottomR += (handler->aim - handler->maxBottomR) * handler->proportion;
    handler->lightBottomR = (unsigned short) LIMIT_MIN_MAX(handler->lightBottomR, 100, handler->maxLight);
    handler->lightBottomG += (handler->aim - handler->maxBottomG) * handler->proportion;
    handler->lightBottomG = (unsigned short) LIMIT_MIN_MAX(handler->lightBottomG, 100, handler->maxLight);
    handler->lightBottomB += (handler->aim - handler->maxBottomB) * handler->proportion;
    handler->lightBottomB = (unsigned short) LIMIT_MIN_MAX(handler->lightBottomB, 100, handler->maxLight);
    handler->lightBottomIR += (handler->aim - handler->maxBottomIR) * handler->proportion;
    handler->lightBottomIR = (unsigned short) LIMIT_MIN_MAX(handler->lightBottomIR, 100, handler->maxLight);

    switch(handler->imageAttr.color) {
        case 'C':
            if(abs(handler->aim - handler->maxTopR) > handler->error)
                handler->finality = 0;
            if(abs(handler->aim - handler->maxTopG) > handler->error)
                handler->finality = 0;
            if(abs(handler->aim - handler->maxTopB) > handler->error)
                handler->finality = 0;
            handler->lightTopIR = 0;
            if(abs(handler->aim - handler->maxBottomR) > handler->error)
                handler->finality = 0;
            if(abs(handler->aim - handler->maxBottomG) > handler->error)
                handler->finality = 0;
            if(abs(handler->aim - handler->maxBottomB) > handler->error)
                handler->finality = 0;
            handler->lightBottomIR = 0;
            break;
        case 'G':
            if(abs(handler->aim - handler->maxTopR) > handler->error)
                handler->finality = 0;
            handler->lightTopG = (unsigned short) (handler->lightTopR * 0.8);
            handler->lightTopB = (unsigned short) (handler->lightTopR * 0.6);
            handler->lightTopIR = 0;
            if(abs(handler->aim - handler->maxBottomR) > handler->error)
                handler->finality = 0;
            handler->lightBottomG = (unsigned short) (handler->lightBottomR * 0.8);
            handler->lightBottomB = (unsigned short) (handler->lightBottomR * 0.6);
            handler->lightBottomIR = 0;
            break;
        case 'I':
            if(abs(handler->aim - handler->maxTopIR) > handler->error)
                handler->finality = 0;
            handler->lightTopR = 0;
            handler->lightTopG = 0;
            handler->lightTopB = 0;
            if(abs(handler->aim - handler->maxBottomIR) > handler->error)
                handler->finality = 0;
            handler->lightBottomR = 0;
            handler->lightBottomG = 0;
            handler->lightBottomB = 0;
            break;
        default:
            break;
    }

    if(handler->finality++ > 5)
        return 1;

    if(handler->overtime++ > 60) {
        PR("Over Time in Light Adjusting\n");
        return 1;
    }

    PR("    top light   top max   bottom light   bottom max.\n");
    PR("R   %4d         %4d       %4d            %4d.        \n", handler->lightTopR, handler->maxTopR, handler->lightBottomR, handler->maxBottomR);
    PR("G   %4d         %4d       %4d            %4d.        \n", handler->lightTopG, handler->maxTopG, handler->lightBottomG, handler->maxBottomG);
    PR("B   %4d         %4d       %4d            %4d.        \n", handler->lightTopB, handler->maxTopB, handler->lightBottomB, handler->maxBottomB);
    PR("IR  %4d         %4d       %4d            %4d.        \n", handler->lightTopIR, handler->maxTopIR, handler->lightBottomIR, handler->maxBottomIR);
    return 0;
}

void LightAdjustmentInit(LightAdjustmentTypeDef *handler, int dpi, char color)
{
    handler->finality = 0;
    handler->overtime = 0;
    handler->aim = 237;
    handler->error = 8;

    handler->imageAttr.dpi = dpi;
    handler->imageAttr.color = color;

    switch(handler->imageAttr.dpi) {
        case 200:
            handler->sampleWidth = CIS_WIDTH_200DPI * 2;
            switch(handler->imageAttr.color) {
                case 'C':
                    handler->imageAttr.depth = 3;
                    handler->maxLight = 1333;
                    handler->proportion = 2;

                    handler->lightTopR = 1000;
                    handler->lightTopG = 800;
                    handler->lightTopB = 600;
                    handler->lightTopIR = 0;
                    handler->lightBottomR = 1000;
                    handler->lightBottomG = 800;
                    handler->lightBottomB = 600;
                    handler->lightBottomIR = 0;
                    handler->sampleR = malloc(handler->sampleWidth);
                    handler->sampleG = malloc(handler->sampleWidth);
                    handler->sampleB = malloc(handler->sampleWidth);
                    handler->sampleIR = NULL;
                    break;
                case 'G':
                    handler->imageAttr.depth = 1;
                    handler->maxLight = 1333;
                    handler->proportion = 0.7;

                    handler->lightTopR = 400;
                    handler->lightTopG = (unsigned short) (handler->lightTopR * 0.8);
                    handler->lightTopB = (unsigned short) (handler->lightTopR * 0.6);
                    handler->lightTopIR = 0;
                    handler->lightBottomR = 400;
                    handler->lightBottomG = (unsigned short) (handler->lightBottomR * 0.8);
                    handler->lightBottomB = (unsigned short) (handler->lightBottomR * 0.6);
                    handler->lightBottomIR = 0;
                    handler->sampleR = malloc(handler->sampleWidth);
                    handler->sampleG = NULL;
                    handler->sampleB = NULL;
                    handler->sampleIR = NULL;
                    break;
                case 'I':
                    handler->imageAttr.depth = 1;
                    handler->maxLight = 1333;
                    handler->proportion = 2;

                    handler->lightTopR = 0;
                    handler->lightTopG = 0;
                    handler->lightTopB = 0;
                    handler->lightTopIR = 800;
                    handler->lightBottomR = 0;
                    handler->lightBottomG = 0;
                    handler->lightBottomB = 0;
                    handler->lightBottomIR = 800;
                    handler->sampleR = NULL;
                    handler->sampleG = NULL;
                    handler->sampleB = NULL;
                    handler->sampleIR = malloc(handler->sampleWidth);
                    break;
                default:
                    break;
            }
            break;
        case 300:
            handler->sampleWidth = CIS_WIDTH_300DPI * 2;
            switch(handler->imageAttr.color) {
                case 'C':
                    handler->imageAttr.depth = 3;
                    handler->maxLight = 2000;
                    handler->proportion = 2;

                    handler->lightTopR = 1500;
                    handler->lightTopG = 1200;
                    handler->lightTopB = 700;
                    handler->lightTopIR = 0;
                    handler->lightBottomR = 1500;
                    handler->lightBottomG = 1200;
                    handler->lightBottomB = 700;
                    handler->lightBottomIR = 0;
                    handler->sampleR = malloc(handler->sampleWidth);
                    handler->sampleG = malloc(handler->sampleWidth);
                    handler->sampleB = malloc(handler->sampleWidth);
                    handler->sampleIR = NULL;
                    break;
                case 'G':
                    handler->imageAttr.depth = 1;
                    handler->maxLight = 2000;
                    handler->proportion = 0.7;

                    handler->lightTopR = 500;
                    handler->lightTopG = (unsigned short) (handler->lightTopR * 0.8);
                    handler->lightTopB = (unsigned short) (handler->lightTopR * 0.6);
                    handler->lightTopIR = 0;
                    handler->lightBottomR = 500;
                    handler->lightBottomG = (unsigned short) (handler->lightBottomR * 0.8);
                    handler->lightBottomB = (unsigned short) (handler->lightBottomR * 0.6);
                    handler->lightBottomIR = 0;
                    handler->sampleR = malloc(handler->sampleWidth);
                    handler->sampleG = NULL;
                    handler->sampleB = NULL;
                    handler->sampleIR = NULL;
                    break;
                case 'I':
                    handler->imageAttr.depth = 1;
                    handler->maxLight = 2000;
                    handler->proportion = 2;

                    handler->lightTopR = 0;
                    handler->lightTopG = 0;
                    handler->lightTopB = 0;
                    handler->lightTopIR = 1000;
                    handler->lightBottomR = 0;
                    handler->lightBottomG = 0;
                    handler->lightBottomB = 0;
                    handler->lightBottomIR = 1000;
                    handler->sampleR = NULL;
                    handler->sampleG = NULL;
                    handler->sampleB = NULL;
                    handler->sampleIR = malloc(handler->sampleWidth);
                    break;
                default:
                    break;
            }
            break;
        case 600:
            handler->sampleWidth = CIS_WIDTH_600DPI * 2;
            switch(handler->imageAttr.color) {
                case 'C':
                    handler->imageAttr.depth = 3;
                    handler->maxLight = 4000;
                    handler->proportion = 2;

                    handler->lightTopR = 2700;
                    handler->lightTopG = 2100;
                    handler->lightTopB = 1200;
                    handler->lightTopIR = 0;
                    handler->lightBottomR = 2700;
                    handler->lightBottomG = 2100;
                    handler->lightBottomB = 1200;
                    handler->lightBottomIR = 0;
                    handler->sampleR = malloc(handler->sampleWidth);
                    handler->sampleG = malloc(handler->sampleWidth);
                    handler->sampleB = malloc(handler->sampleWidth);
                    handler->sampleIR = NULL;
                    break;
                case 'G':
                    handler->imageAttr.depth = 1;
                    handler->maxLight = 4000;
                    handler->proportion = 0.7;

                    handler->lightTopR = 900;
                    handler->lightTopG = (unsigned short) (handler->lightTopR * 0.8);
                    handler->lightTopB = (unsigned short) (handler->lightTopR * 0.6);
                    handler->lightTopIR = 0;
                    handler->lightBottomR = 900;
                    handler->lightBottomG = (unsigned short) (handler->lightBottomR * 0.8);
                    handler->lightBottomB = (unsigned short) (handler->lightBottomR * 0.6);
                    handler->lightBottomIR = 0;
                    handler->sampleR = malloc(handler->sampleWidth);
                    handler->sampleG = NULL;
                    handler->sampleB = NULL;
                    handler->sampleIR = NULL;
                    break;
                case 'I':
                    handler->imageAttr.depth = 1;
                    handler->maxLight = 4000;
                    handler->proportion = 2;

                    handler->lightTopR = 0;
                    handler->lightTopG = 0;
                    handler->lightTopB = 0;
                    handler->lightTopIR = 2300;
                    handler->lightBottomR = 0;
                    handler->lightBottomG = 0;
                    handler->lightBottomB = 0;
                    handler->lightBottomIR = 2300;
                    handler->sampleR = NULL;
                    handler->sampleG = NULL;
                    handler->sampleB = NULL;
                    handler->sampleIR = malloc(handler->sampleWidth);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

void LightAdjustmentDeInit(LightAdjustmentTypeDef *handler)
{
    if(handler->sampleR)
        free(handler->sampleR);
    if(handler->sampleG)
        free(handler->sampleG);
    if(handler->sampleB)
        free(handler->sampleB);
    if(handler->sampleIR)
        free(handler->sampleIR);
}

void LightAdjustmentSet(LightAdjustmentTypeDef *handler)
{
    KernelVIDEOWriteLightPara(handler->lightBottomR, handler->lightTopR, handler->lightBottomG, handler->lightTopG,
                              handler->lightBottomB, handler->lightTopB, handler->lightBottomIR, handler->lightTopIR);
}

void LightAdjustmentSave(LightAdjustmentTypeDef *handler)
{
    char fileName[128];
    int magic = 34567;

    sprintf(fileName, "para_light_%d_%c", handler->imageAttr.dpi, handler->imageAttr.color);
    FILE *stream = fopen(fileName, "wb");
    if(stream == NULL)
        return;

    fwrite(&magic, sizeof(int), 1, stream);
    fwrite(handler, sizeof(LightAdjustmentTypeDef), 1, stream);
    fflush(stream);
    fclose(stream);
    sync();
}

int LightAdjustmentLocal(int dpi, char color)
{
    char fileName[128];
    int magic;
    int ret = 0;

    sprintf(fileName, "para_light_%d_%c", dpi, color);
    FILE *stream = fopen(fileName, "rb");
    if(stream == NULL)
        return ret;

    fseek(stream, 0, SEEK_END);
    if(ftell(stream) == 0) {
        fclose(stream);
        return ret;
    }
    fseek(stream, 0, SEEK_SET);
    fread(&magic, sizeof(int), 1, stream);

    if(magic == 34567) {
        LightAdjustmentTypeDef LightAdjustmentHandler;
        fread(&LightAdjustmentHandler, sizeof(LightAdjustmentTypeDef), 1, stream);
        LightAdjustmentSet(&LightAdjustmentHandler);
        ret = 1;
    }
    fclose(stream);
    return ret;
}

void LightAdjustmentDefault(int dpi, char color)
{
    LightAdjustmentTypeDef LightAdjustmentHandler;
    LightAdjustmentHandler.imageAttr.dpi = dpi;
    LightAdjustmentHandler.imageAttr.color = color;
    switch (LightAdjustmentHandler.imageAttr.dpi) {
        case 200:
        case 300:
        case 600:
            switch (LightAdjustmentHandler.imageAttr.color) {
                case 'G':
                    LightAdjustmentHandler.lightTopR = 400;
                    LightAdjustmentHandler.lightTopG = 360;
                    LightAdjustmentHandler.lightTopB = 240;
                    LightAdjustmentHandler.lightTopIR = 0;
                    LightAdjustmentHandler.lightBottomR = 400;
                    LightAdjustmentHandler.lightBottomG = 360;
                    LightAdjustmentHandler.lightBottomB = 240;
                    LightAdjustmentHandler.lightBottomIR = 0;
                    break;
                case 'C':
                case 'I':
                    LightAdjustmentHandler.lightTopR = 1200;
                    LightAdjustmentHandler.lightTopG = 1000;
                    LightAdjustmentHandler.lightTopB = 800;
                    LightAdjustmentHandler.lightTopIR = 1200;
                    LightAdjustmentHandler.lightBottomR = 1200;
                    LightAdjustmentHandler.lightBottomG = 1000;
                    LightAdjustmentHandler.lightBottomB = 800;
                    LightAdjustmentHandler.lightBottomIR = 1200;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    LightAdjustmentSet(&LightAdjustmentHandler);
}

void LightAdjustmentLoad(int dpi, char color)
{
    if (LightAdjustmentLocal(dpi, color)) {
        PR("Light adjustment is ready.\n");
    }
    else {
        LightAdjustmentDefault(dpi, color);
        PR("Error in light adjustment updating.\n");
    }
}