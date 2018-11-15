#ifndef __CORRECTION_H__
#define __CORRECTION_H__

#include "main.h"

typedef struct{
    ImageAttributionsTypeDef imageAttr;

    unsigned char *RK; // K -> Bright sample space
    unsigned char *RB; // B -> Dark sample space
    unsigned char *GK;
    unsigned char *GB;
    unsigned char *BK;
    unsigned char *BB;

    unsigned short *RK_Ext; // Bright sample extend space
    unsigned short *GK_Ext;
    unsigned short *BK_Ext;

    unsigned short sampleWidth;
}CorrectionParaTypeDef;

typedef enum {
    CORRECTION_PARA_CHANNEL_1 = 0,
    CORRECTION_PARA_CHANNEL_23,
    CORRECTION_PARA_CHANNEL_ALL
}CorrectionParaChannelTypeDef;

void VerticalSample(unsigned char *src, unsigned char *dst, int width, int height, int depth);
void CorrectionParaReplace(unsigned char *src, unsigned short *dst, int length);
void CorrectionParaOverlay(unsigned char *src, unsigned short *dst, int length);
void RecoverSample(unsigned short *src, unsigned char *dst, int length);
void CorrectionMix(unsigned char *brightK, unsigned char *darkB, int length, float lightStandard);

void CorrectionParaInit(CorrectionParaTypeDef *handler, int dpi, char color);
void CorrectionParaDeInit(CorrectionParaTypeDef *handler);
void CorrectionParaSet(CorrectionParaTypeDef *handler, CorrectionParaChannelTypeDef channel);

void CorrectionParaSave(CorrectionParaTypeDef *handler);
int CorrectionParaLocal(int dpi, char color, CorrectionParaChannelTypeDef channel);
void CorrectionParaDefault(void);
void CorrectionParaLoad(int dpi, char color, CorrectionParaChannelTypeDef channel);

void CorrectionNoPaperDataSave(CorrectionParaTypeDef *handler);
void CorrectionNoPaperDataLoad(CorrectionParaTypeDef *handler);

#endif
