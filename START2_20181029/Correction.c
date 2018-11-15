#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "main.h"
#include "Correction.h"
#include "KernelVideo.h"

void VerticalSample(unsigned char *src, unsigned char *dst, int width, int height, int depth)
{
    for(int col = 0; col < width; col++) {
        int sum = 0;
        for(int row = 0; row < height; row++) {
            sum += *(src + VIDEO_PORT_WIDTH * row * depth);
        }
        *dst++ = (unsigned char) (sum / height);
        src++;
    }
}

void CorrectionParaReplace(unsigned char *src, unsigned short *dst, int length)
{
    for(int i = 0; i < length; i++) {
        *dst = *src;
        dst++;
        src++;
    }
}

void CorrectionParaOverlay(unsigned char *src, unsigned short *dst, int length)
{
    for(int i = 0; i < length; i++) {
        *dst += *src;
        dst++;
        src++;
    }
}

void RecoverSample(unsigned short *src, unsigned char *dst, int length)
{
    for(int i = 0; i < length; i++) {
        *dst = (unsigned char)(*src >> 1);
        dst++;
        src++;
    }
}

void CorrectionMix(unsigned char *brightK, unsigned char *darkB, int length, float lightStandard)
{
    int i = 0;
    float temp;

    for(i = 0; i < length; i++) {
        //*darkB = 0;
        if(*brightK > 255)
            *brightK = 0;
        else if(*brightK <= 60)
            *brightK = 0;
        else {
            temp = lightStandard / ((float)(*brightK - *darkB)) - 1;
            *brightK = (unsigned char)(temp * 64.0f);
        }

        brightK++;
        darkB++;
    }
}

void CorrectionParaInit(CorrectionParaTypeDef *handler, int dpi, char color)
{
    handler->imageAttr.dpi = dpi;
    handler->imageAttr.color = color;

    switch (handler->imageAttr.dpi) {
        case 200:
            handler->sampleWidth = CIS_WIDTH_200DPI * 2;
            break;
        case 300:
            handler->sampleWidth = CIS_WIDTH_300DPI * 2;
            break;
        case 600:
            handler->sampleWidth = CIS_WIDTH_600DPI * 2;
            break;
        default:
            break;
    }

    switch (handler->imageAttr.color) {
        case 'C':
            handler->imageAttr.depth = 3;
            handler->RK = malloc(handler->sampleWidth);
            handler->RB = malloc(handler->sampleWidth);
            handler->GK = malloc(handler->sampleWidth);
            handler->GB = malloc(handler->sampleWidth);
            handler->BK = malloc(handler->sampleWidth);
            handler->BB = malloc(handler->sampleWidth);
            handler->RK_Ext = malloc(handler->sampleWidth * sizeof(unsigned short));
            handler->GK_Ext = malloc(handler->sampleWidth * sizeof(unsigned short));
            handler->BK_Ext = malloc(handler->sampleWidth * sizeof(unsigned short));
            break;
        case 'G':
        case 'I':
            handler->imageAttr.depth = 1;
            handler->RK = malloc(handler->sampleWidth);
            handler->RB = malloc(handler->sampleWidth);
            handler->GK = NULL;
            handler->GB = NULL;
            handler->BK = NULL;
            handler->BB = NULL;
            handler->RK_Ext = malloc(handler->sampleWidth * sizeof(unsigned short));
            handler->GK_Ext = NULL;
            handler->BK_Ext = NULL;
            break;
        default:
            break;
    }
}

void CorrectionParaDeInit(CorrectionParaTypeDef *handler)
{
    if(handler->RK)
        free(handler->RK);
    if(handler->GK)
        free(handler->GK);
    if(handler->BK)
        free(handler->BK);

    if(handler->RB)
        free(handler->RB);
    if(handler->GB)
        free(handler->GB);
    if(handler->BB)
        free(handler->BB);

    if(handler->RK_Ext)
        free(handler->RK_Ext);
    if(handler->GK_Ext)
        free(handler->GK_Ext);
    if(handler->BK_Ext)
        free(handler->BK_Ext);
}

void CorrectionParaSet(CorrectionParaTypeDef *handler, CorrectionParaChannelTypeDef channel)
{
    switch (channel) {
        case CORRECTION_PARA_CHANNEL_1:
            switch (handler->imageAttr.color) {
                case 'C':
                    KernelVIDEOWriteCorrectionPara(handler->RK, handler->RB, handler->sampleWidth, 201);
                    break;
                case 'G':
                    KernelVIDEOWriteCorrectionPara(handler->RK, handler->RB, handler->sampleWidth, 205);
                    break;
                case 'I':
                    KernelVIDEOWriteCorrectionPara(handler->RK, handler->RB, handler->sampleWidth, 204);
                    break;
                default:
                    break;
            }
            break;

        case CORRECTION_PARA_CHANNEL_23:
            if (handler->imageAttr.color == 'C') {
                KernelVIDEOWriteCorrectionPara(handler->GK, handler->GB, handler->sampleWidth, 202);
                KernelVIDEOWriteCorrectionPara(handler->BK, handler->BB, handler->sampleWidth, 203);
            }
            break;

        case CORRECTION_PARA_CHANNEL_ALL:
            switch (handler->imageAttr.color) {
                case 'C':
                    KernelVIDEOWriteCorrectionPara(handler->RK, handler->RB, handler->sampleWidth, 201);
                    KernelVIDEOWriteCorrectionPara(handler->GK, handler->GB, handler->sampleWidth, 202);
                    KernelVIDEOWriteCorrectionPara(handler->BK, handler->BB, handler->sampleWidth, 203);
                    break;
                case 'G':
                    KernelVIDEOWriteCorrectionPara(handler->RK, handler->RB, handler->sampleWidth, 205);
                    break;
                case 'I':
                    KernelVIDEOWriteCorrectionPara(handler->RK, handler->RB, handler->sampleWidth, 204);
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
}

void CorrectionParaSave(CorrectionParaTypeDef *handler)
{
    char fileName[128];
    int magic = 67890;

    sprintf(fileName, "para_correction_%d_%c", handler->imageAttr.dpi, handler->imageAttr.color);
    FILE *stream = fopen(fileName, "wb");
    if(stream == NULL)
        return;

    fwrite(&magic, sizeof(int), 1, stream);
    fwrite(handler->RK, (size_t) handler->sampleWidth, 1, stream);
    fwrite(handler->RB, (size_t) handler->sampleWidth, 1, stream);
    if(handler->imageAttr.depth == 3) {
        fwrite(handler->GK, (size_t) handler->sampleWidth, 1, stream);
        fwrite(handler->GB, (size_t) handler->sampleWidth, 1, stream);
        fwrite(handler->BK, (size_t) handler->sampleWidth, 1, stream);
        fwrite(handler->BB, (size_t) handler->sampleWidth, 1, stream);
    }
    fflush(stream);
    fclose(stream);
    sync();
}

int CorrectionParaLocal(int dpi, char color, CorrectionParaChannelTypeDef channel)
{
    char fileName[128];
    int magic;
    int ret = 0;

    sprintf(fileName, "para_correction_%d_%c", dpi, color);
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

    if(magic == 67890) {
        CorrectionParaTypeDef CorrectionParaHandle;
        CorrectionParaInit(&CorrectionParaHandle, dpi, color);
        fread(CorrectionParaHandle.RK, (size_t) CorrectionParaHandle.sampleWidth, 1, stream);
        fread(CorrectionParaHandle.RB, (size_t) CorrectionParaHandle.sampleWidth, 1, stream);
        if(CorrectionParaHandle.imageAttr.depth == 3) {
            fread(CorrectionParaHandle.GK, (size_t) CorrectionParaHandle.sampleWidth, 1, stream);
            fread(CorrectionParaHandle.GB, (size_t) CorrectionParaHandle.sampleWidth, 1, stream);
            fread(CorrectionParaHandle.BK, (size_t) CorrectionParaHandle.sampleWidth, 1, stream);
            fread(CorrectionParaHandle.BB, (size_t) CorrectionParaHandle.sampleWidth, 1, stream);
        }
        CorrectionParaSet(&CorrectionParaHandle, channel);
        CorrectionParaDeInit(&CorrectionParaHandle);
        ret = 1;
    }
    fclose(stream);
    return ret;
}

void CorrectionParaDefault(void)
{
    KernelVIDEODisableCorrection();
}

void CorrectionParaLoad(int dpi, char color, CorrectionParaChannelTypeDef channel)
{
    KernelVIDEOEnableCorrection();
    if (CorrectionParaLocal(dpi, color, channel)) {
        PR("Correction parameters is ready.\n");
    }
    else {
        KernelVIDEODisableCorrection();
        PR("Error in correction parameters updating.\n");
    }
}

void CorrectionNoPaperDataSave(CorrectionParaTypeDef *handler)
{
    char fileName[128];
    int magic = 12345;

    sprintf(fileName, "no_paper_data_%d_%c", handler->imageAttr.dpi, handler->imageAttr.color);
    FILE *stream = fopen(fileName, "wb");
    if(stream == NULL)
        return;

    fwrite(&magic, sizeof(int), 1, stream);
    fwrite(handler->RB, (size_t) handler->sampleWidth, 1, stream);
    if(handler->imageAttr.depth == 3) {
        fwrite(handler->GB, (size_t) handler->sampleWidth, 1, stream);
        fwrite(handler->BB, (size_t) handler->sampleWidth, 1, stream);
    }
    fflush(stream);
    fclose(stream);
    sync();
}

void CorrectionNoPaperDataLoad(CorrectionParaTypeDef *handler)
{
    char fileName[128];
    int magic;

    sprintf(fileName, "no_paper_data_%d_%c", handler->imageAttr.dpi, handler->imageAttr.color);
    FILE *stream = fopen(fileName, "rb");
    if(stream == NULL) {
        memset(handler->RB, 0, (size_t) handler->sampleWidth);
        if(handler->imageAttr.depth == 3) {
            memset(handler->GB, 0, (size_t) handler->sampleWidth);
            memset(handler->BB, 0, (size_t) handler->sampleWidth);
        }
        return;
    }

    fseek(stream, 0, SEEK_END);
    if(ftell(stream) == 0) {
        fclose(stream);
        memset(handler->RB, 0, (size_t) handler->sampleWidth);
        if(handler->imageAttr.depth == 3) {
            memset(handler->GB, 0, (size_t) handler->sampleWidth);
            memset(handler->BB, 0, (size_t) handler->sampleWidth);
        }
        return;
    }
    fseek(stream, 0, SEEK_SET);
    fread(&magic, sizeof(int), 1, stream);

    if(magic == 12345) {
        fread(handler->RB, (size_t) handler->sampleWidth, 1, stream);
        if(handler->imageAttr.depth == 3) {
            fread(handler->GB, (size_t) handler->sampleWidth, 1, stream);
            fread(handler->BB, (size_t) handler->sampleWidth, 1, stream);
        }
    }
    fclose(stream);
}