#ifndef __LIGHT_ADJUSTMENT_H__
#define __LIGHT_ADJUSTMENT_H__

#include "main.h"

typedef struct {
    ImageAttributionsTypeDef imageAttr;

    unsigned short lightTopR;
    unsigned short lightTopG;
    unsigned short lightTopB;
    unsigned short lightTopIR;

    unsigned short lightBottomR;
    unsigned short lightBottomG;
    unsigned short lightBottomB;
    unsigned short lightBottomIR;

    unsigned char maxTopR;
    unsigned char maxTopG;
    unsigned char maxTopB;
    unsigned char maxTopIR;

    unsigned char maxBottomR;
    unsigned char maxBottomG;
    unsigned char maxBottomB;
    unsigned char maxBottomIR;

    unsigned char *sampleR;
    unsigned char *sampleG;
    unsigned char *sampleB;
    unsigned char *sampleIR;

    unsigned short maxLight;
    unsigned short sampleWidth;

    unsigned char finality;
    unsigned char overtime;
    unsigned char aim;
    unsigned char error;
    float proportion;
}LightAdjustmentTypeDef;

unsigned char GetMaxPixelInOneLine(const unsigned char *src, int length);
void GetMaxPixelInColorSpace(LightAdjustmentTypeDef *handler, unsigned char *originalImage, int height);
int LightAdjustmentJudge(LightAdjustmentTypeDef *handler);

void LightAdjustmentInit(LightAdjustmentTypeDef *handler, int dpi, char color);
void LightAdjustmentDeInit(LightAdjustmentTypeDef *handler);
void LightAdjustmentSet(LightAdjustmentTypeDef *handler);

void LightAdjustmentSave(LightAdjustmentTypeDef *handler);
int LightAdjustmentLocal(int dpi, char color);
void LightAdjustmentDefault(int dpi, char color);
void LightAdjustmentLoad(int dpi, char color);

#endif
