#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "main.h"
#include "list.h"
#include "KernelLed.h"
#include "KernelSpi.h"
#include "KernelVideo.h"
#include "PrinterControl.h"
#include "ColorRemap.h"
#include "Scan.h"
#include "Jpeg.h"
#include "Correction.h"
#include "LightAdjustment.h"
#include "Update.h"

int main(int argc, char *argv[])
{
    int dpi = 200;
    char color = 'C';
    int videoPortOffset = -4;

    switch(argc) {
        case 2:
            dpi = strtol(argv[1], NULL, 10);
            break;
        case 3:
            dpi = strtol(argv[1], NULL, 10);
            color = argv[2][0];
            break;
        case 4:
            dpi = strtol(argv[1], NULL, 10);
            color = argv[2][0];
            videoPortOffset = strtol(argv[3], NULL, 10);
        default:
            break;
    }

    switch(dpi) {
        case 200:
        case 300:
        case 600:
            break;
        default:
            dpi = 200;
            break;
    }

    switch(color) {
        case 'c':
        case 'C':
            color = 'C';
            break;
        case 'g':
        case 'G':
            color = 'G';
            break;
        case 'i':
        case 'I':
            color = 'I';
            break;
        default:
            color = 'C';
            break;
    }

    int ret = MainProcess(dpi, color, videoPortOffset);
    switch(ret) {
        case 0:
            printf("Update successfully.\n");
            break;
        case -1:
            printf("Modules not installed.\n");
            break;
        case -2:
            printf("Update unsuccessfully.\n");
            break;
        default:
            break;
    }
    return 0;
}

pthread_t thread;
pthread_mutex_t scanMutex;
pthread_mutex_t sendPicture1Mutex;
pthread_mutex_t sendPicture2Mutex;
pthread_cond_t scanCond;
sem_t scanSem;

struct list_shift_picture{
    struct list_head queue;
    unsigned char data[VIDEO_PORT_WIDTH * 6];
};
struct list_shift_picture *shiftPicture;
struct list_head queueHead1;
struct list_head queueHead2;

struct jpeg_compress_struct JPEGCompressOne;
struct jpeg_error_mgr JPEGErrorOne;
struct jpeg_compress_struct JPEGCompressTwo;
struct jpeg_error_mgr JPEGErrorTwo;
unsigned char *JPEGDestinationOne = NULL;
unsigned long JpegLengthOne = 0;
unsigned char *JPEGDestinationTwo = NULL;
unsigned long JpegLengthTwo = 0;

ConfigMessageTypeDef ConfigMessage;
CompressProcessTypeDef CompressProcess;

unsigned char spiBuffer[512] = {0};
unsigned int spiBufferSize = 0;

int MainProcess(int dpi, char color, int videoPortOffset)
{
    /* Modules process. */
    int ledDescriptor = open("/dev/START2_LED", O_RDWR);
    if (ledDescriptor < 0)
        return -1;
    KernelLEDRegister(ledDescriptor);

    int videoDescriptor = open("/dev/START2_VIDEO", O_RDWR);
    if (videoDescriptor < 0)
        return -1;
    KernelVIDEORegister(videoDescriptor);

    int spiDescriptor = open("/dev/START2_SPI", O_RDWR);
    if (spiDescriptor < 0)
        return -1;
    KernelSPIRegister(spiDescriptor);

    /* Initialization. */
    PR("\n            Start 2          \n");
    PR("Initial mode: %d dpi, %c color, offset: %d.\n", dpi, color, videoPortOffset);

    StateMachineTypeDef stateMachine = STATE_MACHINE_IDLE;
    LightAdjustmentTypeDef LightAdjustmentHandler;
    CorrectionParaTypeDef CorrectionParaHandle;
    unsigned char *originalImage = NULL;
    unsigned char *arrange = malloc(VIDEO_PORT_WIDTH * 6);
    unsigned char *updateData = NULL;
    size_t updatePackCount = 0;
    int updateFail = 0;
    int request = 1;
    int correctionStep = 0;

    /* Kernel layer process. */
    KernelLEDSetState(1);
    KernelVIDEOWriteConfigPara();
    usleep(10*1000);
    KernelVIDEOWriteScanPara(200, 'C');
    CorrectionParaLoad(200, 'C', CORRECTION_PARA_CHANNEL_23);
    KernelVIDEOWriteScanPara(300, 'C');
    CorrectionParaLoad(300, 'C', CORRECTION_PARA_CHANNEL_23);
    KernelVIDEOWriteScanPara(600, 'C');
    CorrectionParaLoad(600, 'C', CORRECTION_PARA_CHANNEL_23);
    KernelVIDEOWriteScanPara(dpi, color);
    LightAdjustmentLoad(dpi, color);
    CorrectionParaLoad(dpi, color, CORRECTION_PARA_CHANNEL_1);
    originalImage = mmap(NULL, VIDEO_PORT_WIDTH * VIDEO_PORT_HEIGHT, PROT_READ, MAP_SHARED, videoDescriptor, 0);
    originalImage += KernelVIDEOGetImageOffsetInPage();

    /* User layer process. */
    ConfigMessage.dpi = dpi;
    ConfigMessage.color = color;
    CompressProcess.step = COMPRESS_STEP_IDLE;
    JpegInitialize(&JPEGCompressOne, &JPEGErrorOne, &JPEGDestinationOne, &JpegLengthOne);
    JpegInitialize(&JPEGCompressTwo, &JPEGErrorTwo, &JPEGDestinationTwo, &JpegLengthTwo);
    ColorMapInit(0);
    INIT_LIST_HEAD(&queueHead1);
    INIT_LIST_HEAD(&queueHead2);
    pthread_mutex_init(&scanMutex, NULL);
    pthread_mutex_init(&sendPicture1Mutex, NULL);
    pthread_mutex_init(&sendPicture2Mutex, NULL);
    pthread_cond_init(&scanCond, NULL);
    sem_init(&scanSem, 0, 0);

    while (1){
        /* Resume process */
        if (KernelSPIGetStandbyState()) {
            PR("Resume Process.\n");
            KernelVIDEOResume();
            KernelVIDEOWriteReg(0, 0);
            usleep(20*1000);
            KernelVIDEOReset();
            KernelSPIResetAllFifo();
        }

        /* Ready for command. */
        PR("Ready for command.\n");
        KernelLEDSetState(3);
        spiBufferSize = KernelSPIReadCommand(spiBuffer, request);
        if (PrinterCheckSPIMessageHeadAndTail(&spiBuffer[0], &spiBuffer[spiBufferSize-5]))
            stateMachine = PrinterCommandSort(&spiBuffer[5], &ConfigMessage);
        KernelLEDSetState(1);

#if DEBUG_LEVEL <= 0
        StateMachineSetTime();
#endif
        switch (stateMachine) {
            case STATE_MACHINE_CORRECTION_DARK:
                PR("correction dark.\n");
                CorrectionParaInit(&CorrectionParaHandle, dpi, color);
                KernelVIDEOWriteScanPara(dpi, color);
                CorrectionParaDefault();
                KernelVIDEOStartScan(1);
                ScanTimeSet();
                while (ScanLinesGet(CorrectionParaHandle.imageAttr.dpi, CorrectionParaHandle.imageAttr.depth) < 300);

                switch (CorrectionParaHandle.imageAttr.dpi) {
                    case 200:
                    case 300:
                        VerticalSample(originalImage + VIDEO_PORT_WIDTH * 31 + videoPortOffset, CorrectionParaHandle.RB, CorrectionParaHandle.sampleWidth, 250, CorrectionParaHandle.imageAttr.depth);
                        if (CorrectionParaHandle.imageAttr.depth == 3) {
                            VerticalSample(originalImage + VIDEO_PORT_WIDTH * 32 + videoPortOffset, CorrectionParaHandle.GB, CorrectionParaHandle.sampleWidth, 250, CorrectionParaHandle.imageAttr.depth);
                            VerticalSample(originalImage + VIDEO_PORT_WIDTH * 30 + videoPortOffset, CorrectionParaHandle.BB, CorrectionParaHandle.sampleWidth, 250, CorrectionParaHandle.imageAttr.depth);
                        }
                        break;
                    case 600:
                        VerticalSample(originalImage + VIDEO_PORT_WIDTH * 32 + videoPortOffset, CorrectionParaHandle.RB, CorrectionParaHandle.sampleWidth, 250, CorrectionParaHandle.imageAttr.depth * 2);
                        if (CorrectionParaHandle.imageAttr.depth == 3) {
                            VerticalSample(originalImage + VIDEO_PORT_WIDTH * 34 + videoPortOffset, CorrectionParaHandle.GB, CorrectionParaHandle.sampleWidth, 250, CorrectionParaHandle.imageAttr.depth * 2);
                            VerticalSample(originalImage + VIDEO_PORT_WIDTH * 30 + videoPortOffset, CorrectionParaHandle.BB, CorrectionParaHandle.sampleWidth, 250, CorrectionParaHandle.imageAttr.depth * 2);
                        }
                        break;
                    default:
                        break;
                }
                KernelVIDEOStopScan();
                CorrectionNoPaperDataSave(&CorrectionParaHandle);
                CorrectionParaDeInit(&CorrectionParaHandle);
                break;

            case STATE_MACHINE_CORRECTION_BRIGHT:
                PR("correction bright.\n");
                KernelVIDEOWriteScanPara(dpi, color);
                CorrectionParaDefault();
                KernelVIDEOStartScan(1);
                ScanTimeSet();
                while (ScanLinesGet(CorrectionParaHandle.imageAttr.dpi, CorrectionParaHandle.imageAttr.depth) < 400);

                correctionStep++;
                if (correctionStep % 2) {
                    CorrectionParaInit(&CorrectionParaHandle, dpi, color);
                    switch (CorrectionParaHandle.imageAttr.dpi) {
                        case 200:
                        case 300:
                            VerticalSample(originalImage + VIDEO_PORT_WIDTH * 31 + videoPortOffset, CorrectionParaHandle.RK, CorrectionParaHandle.sampleWidth, 350, CorrectionParaHandle.imageAttr.depth);
                            CorrectionParaReplace(CorrectionParaHandle.RK, CorrectionParaHandle.RK_Ext, CorrectionParaHandle.sampleWidth);
                            if (CorrectionParaHandle.imageAttr.depth == 3) {
                                VerticalSample(originalImage + VIDEO_PORT_WIDTH * 32 + videoPortOffset, CorrectionParaHandle.GK, CorrectionParaHandle.sampleWidth, 350, CorrectionParaHandle.imageAttr.depth);
                                CorrectionParaReplace(CorrectionParaHandle.GK, CorrectionParaHandle.GK_Ext, CorrectionParaHandle.sampleWidth);
                                VerticalSample(originalImage + VIDEO_PORT_WIDTH * 30 + videoPortOffset, CorrectionParaHandle.BK, CorrectionParaHandle.sampleWidth, 350, CorrectionParaHandle.imageAttr.depth);
                                CorrectionParaReplace(CorrectionParaHandle.BK, CorrectionParaHandle.BK_Ext, CorrectionParaHandle.sampleWidth);
                            }
                            break;
                        case 600:
                            VerticalSample(originalImage + VIDEO_PORT_WIDTH * 32 + videoPortOffset, CorrectionParaHandle.RK, CorrectionParaHandle.sampleWidth, 350, CorrectionParaHandle.imageAttr.depth * 2);
                            CorrectionParaReplace(CorrectionParaHandle.RK, CorrectionParaHandle.RK_Ext, CorrectionParaHandle.sampleWidth);
                            if (CorrectionParaHandle.imageAttr.depth == 3) {
                                VerticalSample(originalImage + VIDEO_PORT_WIDTH * 34 + videoPortOffset, CorrectionParaHandle.GK, CorrectionParaHandle.sampleWidth, 350, CorrectionParaHandle.imageAttr.depth * 2);
                                CorrectionParaReplace(CorrectionParaHandle.GK, CorrectionParaHandle.GK_Ext, CorrectionParaHandle.sampleWidth);
                                VerticalSample(originalImage + VIDEO_PORT_WIDTH * 30 + videoPortOffset, CorrectionParaHandle.BK, CorrectionParaHandle.sampleWidth, 350, CorrectionParaHandle.imageAttr.depth * 2);
                                CorrectionParaReplace(CorrectionParaHandle.BK, CorrectionParaHandle.BK_Ext, CorrectionParaHandle.sampleWidth);
                            }
                            break;
                        default:
                            break;
                    }
                }
                else {
                    switch (CorrectionParaHandle.imageAttr.dpi) {
                        case 200:
                        case 300:
                            VerticalSample(originalImage + VIDEO_PORT_WIDTH * 31 + videoPortOffset, CorrectionParaHandle.RK, CorrectionParaHandle.sampleWidth, 350, CorrectionParaHandle.imageAttr.depth);
                            CorrectionParaOverlay(CorrectionParaHandle.RK, CorrectionParaHandle.RK_Ext, CorrectionParaHandle.sampleWidth);
                            if (CorrectionParaHandle.imageAttr.depth == 3) {
                                VerticalSample(originalImage + VIDEO_PORT_WIDTH * 32 + videoPortOffset, CorrectionParaHandle.GK, CorrectionParaHandle.sampleWidth, 350, CorrectionParaHandle.imageAttr.depth);
                                CorrectionParaOverlay(CorrectionParaHandle.GK, CorrectionParaHandle.GK_Ext, CorrectionParaHandle.sampleWidth);
                                VerticalSample(originalImage + VIDEO_PORT_WIDTH * 30 + videoPortOffset, CorrectionParaHandle.BK, CorrectionParaHandle.sampleWidth, 350, CorrectionParaHandle.imageAttr.depth);
                                CorrectionParaOverlay(CorrectionParaHandle.BK, CorrectionParaHandle.BK_Ext, CorrectionParaHandle.sampleWidth);
                            }
                            break;
                        case 600:
                            VerticalSample(originalImage + VIDEO_PORT_WIDTH * 32 + videoPortOffset, CorrectionParaHandle.RK, CorrectionParaHandle.sampleWidth, 350, CorrectionParaHandle.imageAttr.depth * 2);
                            CorrectionParaOverlay(CorrectionParaHandle.RK, CorrectionParaHandle.RK_Ext, CorrectionParaHandle.sampleWidth);
                            if (CorrectionParaHandle.imageAttr.depth == 3) {
                                VerticalSample(originalImage + VIDEO_PORT_WIDTH * 34 + videoPortOffset, CorrectionParaHandle.GK, CorrectionParaHandle.sampleWidth, 350, CorrectionParaHandle.imageAttr.depth * 2);
                                CorrectionParaOverlay(CorrectionParaHandle.GK, CorrectionParaHandle.GK_Ext, CorrectionParaHandle.sampleWidth);
                                VerticalSample(originalImage + VIDEO_PORT_WIDTH * 30 + videoPortOffset, CorrectionParaHandle.BK, CorrectionParaHandle.sampleWidth, 350, CorrectionParaHandle.imageAttr.depth * 2);
                                CorrectionParaOverlay(CorrectionParaHandle.BK, CorrectionParaHandle.BK_Ext, CorrectionParaHandle.sampleWidth);
                            }
                            break;
                        default:
                            break;
                    }
                }
                KernelVIDEOStopScan();
                break;

            case STATE_MACHINE_UPDATE:
                PR("update.\n");
                updateData = malloc(62);
                updatePackCount = 0;

                // Check the rest of packages according to check array.
                while(1) {
                    KernelSPIResetAllFifo();
                    KernelVIDEOReset();
                    KernelSPIIOPulse();
                    while (KernelSPIGetBytesInRxFifo() < 64)
                        usleep(1000);
                    KernelSPIReadPackage(spiBuffer, 64);

                    if (spiBuffer[63] == 0) {
                        PR("package %d\n", updatePackCount);
                        memcpy(updateData + updatePackCount*62, spiBuffer, 62);
                        updatePackCount++;
                        updateData = realloc(updateData, (updatePackCount+1)*62);

                        if (updateData == NULL) {
                            PR("Error In Update Process, Reallocate Failed.\n");
                            free(updateData);
                            sleep(1);
                            close(spiDescriptor);
                            close(videoDescriptor);
                            close(ledDescriptor);
                            return -2;
                        }
                    }
                    else if (spiBuffer[63] == 1) {
                        PR("update end.\n");
                        memcpy(updateData + updatePackCount*62, spiBuffer, 62);
                        updatePackCount++;

                        /*FILE *stream = fopen("update", "wb");
                        fwrite(updateData, updatePackCount*62, 1, stream);
                        fflush(stream);
                        fclose(stream);*/

                        if (!updateFail) {
                            UpdateDecompress(updateData, updatePackCount*62);
                            memset(spiBuffer, 0xFE, 512);
                        }
                        else
                            memset(spiBuffer, 0x01, 512);

                        KernelSPISendPackage(spiBuffer, 512);
                        KernelSPIIOPulse();
                        KernelLEDSetState(0);
                        sleep(1);
                        close(spiDescriptor);
                        close(videoDescriptor);
                        close(ledDescriptor);
                        return 0;
                    }
                    else {
                        updateFail = 1;
                        PR("Error In Update Package Receiving.\n");
                        for (int i = 0; i < 4; i++) {
                            for (int j = 0; j < 16; j++)
                                PR("%2x ", spiBuffer[i*16 + j]);
                            PR("\n");
                        }
                    }
                }

            case STATE_MACHINE_SCAN:
                /* Compress page one if it's available. */
                PrinterScanShowConfigMessage(&ConfigMessage);
                if (!CompressProcessPrepare(ConfigMessage.pageOne, &ConfigMessage, &CompressProcess)) {
                    PR("Failed in compress process preparing, page one.\n");
                    break;
                }

                /* Start to scan and compress. */
                KernelVIDEOWriteScanPara(CompressProcess.imageAttr.dpi, CompressProcess.imageAttr.color);
                KernelVIDEOStartScan((unsigned short)CompressProcess.frame);
                ScanTimeSet();
                CompressProcess.frame --;
                CompressProcess.step = COMPRESS_STEP_P1_ENCODE;
                CompressProcess.scanLines = 0;
                CompressProcess.shiftLines = 0;
                CompressProcess.compressLines = 0;
                CompressProcess.imagePosition = originalImage + videoPortOffset;
                JpegStart(&JPEGCompressOne, &CompressProcess.imageAttr);
                pthread_create(&thread, NULL, (void*)ThreadSendData, NULL);

                while (1) {
                    /* Shift lines to the queue, while lines scanned are more than lines shifted. */
                    CompressProcess.scanLines = ScanLinesGet(CompressProcess.imageAttr.dpi, CompressProcess.imageAttr.depth);
                    while (CompressProcess.scanLines > CompressProcess.shiftLines + 60) {
                        if (CompressProcess.shiftLines >= CompressProcess.maxScanLines)
                            break;

                        shiftPicture = malloc(sizeof(struct list_shift_picture));
                        memcpy(shiftPicture->data, CompressProcess.imagePosition, (size_t) (VIDEO_PORT_WIDTH * CompressProcess.rowsPerLine));
                        list_add_tail(&(shiftPicture->queue), &queueHead1);

                        CompressProcess.imagePosition += VIDEO_PORT_WIDTH * CompressProcess.rowsPerLine;
                        CompressProcess.shiftLines++;
                        if (!(CompressProcess.shiftLines*CompressProcess.rowsPerLine % 7800) && CompressProcess.frame) {
                            PR("New frame!\n");
                            CompressProcess.imagePosition = originalImage;
                            CompressProcess.frame --;
                        }
                    }

                    /* Compress lines from the queue, if lines shifted are more than lines compressed. */
                    if (CompressProcess.shiftLines >= CompressProcess.compressLines) {
                        if (!list_empty(&queueHead1)) {
                            shiftPicture = list_entry(queueHead1.next, struct list_shift_picture, queue);
                            if ((CompressProcess.compressLines >= CompressProcess.imageAttr.topEdge) && (CompressProcess.compressLines < CompressProcess.imageAttr.bottomEdge)) {
                                CompressProcessArrangeOneLine(shiftPicture->data, arrange, &CompressProcess.imageAttr);
                                pthread_mutex_lock(&sendPicture1Mutex);
                                jpeg_write_scanlines(&JPEGCompressOne, &arrange, 1);
                                pthread_mutex_unlock(&sendPicture1Mutex);
                            }
                            list_del(&(shiftPicture->queue));
                            list_add_tail(&(shiftPicture->queue), &queueHead2);
                            CompressProcess.compressLines ++;
                        }
                    }

                    /* quit, if lines compressed are enough. */
                    if (CompressProcess.compressLines >= CompressProcess.maxScanLines) {
                        jpeg_finish_compress(&JPEGCompressOne);
                        CompressProcess.step = COMPRESS_STEP_P2_ENCODE;
                        PR("first picture.\n");
                        break;
                    }
                }

                /* Compress page two if it's available. */
                if (!CompressProcessPrepare(ConfigMessage.pageTwo, &ConfigMessage, &CompressProcess)) {
                    PR("Failed in compress process preparing, page two.\n");
                    while (!list_empty(&queueHead2)) {
                        shiftPicture = list_entry(queueHead2.next, struct list_shift_picture, queue);
                        list_del(&(shiftPicture->queue));
                        free(shiftPicture);
                    }
                    break;
                }

                /* Start to compress. */
                CompressProcess.compressLines = 0;
                JpegStart(&JPEGCompressTwo, &CompressProcess.imageAttr);
                sem_post(&scanSem);

                while (1) {
                    /* Compress lines from the queue. */
                    if (!list_empty(&queueHead2)) {
                        shiftPicture = list_entry(queueHead2.next, struct list_shift_picture, queue);
                        if ((CompressProcess.compressLines >= CompressProcess.imageAttr.topEdge) && (CompressProcess.compressLines < CompressProcess.imageAttr.bottomEdge)) {
                            CompressProcessArrangeOneLine(shiftPicture->data, arrange, &CompressProcess.imageAttr);
                            pthread_mutex_lock(&sendPicture2Mutex);
                            jpeg_write_scanlines(&JPEGCompressTwo, &arrange, 1);
                            pthread_mutex_unlock(&sendPicture2Mutex);
                        }
                        list_del(&(shiftPicture->queue));
                        free(shiftPicture);
                        CompressProcess.compressLines ++;
                    }

                    /* quit, if lines compressed are enough. */
                    if (CompressProcess.compressLines >= CompressProcess.maxScanLines) {
                        jpeg_finish_compress(&JPEGCompressTwo);
                        CompressProcess.step = COMPRESS_STEP_IDLE;
                        PR("second picture.\n");
                        break;
                    }
                }
                break;

            case STATE_MACHINE_LIGHTNESS_ADJUST:
                PR("Lightness adjust.\n");
                LightAdjustmentInit(&LightAdjustmentHandler, dpi, color);
                KernelVIDEOWriteScanPara(dpi, color);
                CorrectionParaDefault();
                do {
                    LightAdjustmentSet(&LightAdjustmentHandler);
                    KernelVIDEOStartScan(1);
                    ScanTimeSet();
                    while (ScanLinesGet(LightAdjustmentHandler.imageAttr.dpi, LightAdjustmentHandler.imageAttr.depth) < 150);
                    KernelVIDEOStopScan();
                    KernelVIDEOReset();
                    GetMaxPixelInColorSpace(&LightAdjustmentHandler, originalImage + VIDEO_PORT_WIDTH * 30, 100);
                } while (!LightAdjustmentJudge(&LightAdjustmentHandler));
                LightAdjustmentSave(&LightAdjustmentHandler);
                LightAdjustmentLoad(dpi, color);
                LightAdjustmentDeInit(&LightAdjustmentHandler);
                KernelSPIIOPulse();
                usleep(20*1000);
                break;

            case STATE_MACHINE_LUMINANCE_SET:
                PR("Luminance set: %d\n", spiBuffer[9]&0x03);
                ColorMapSet(spiBuffer[9]&0x03);

                memset(spiBuffer, 0xFE, 512);
                spiBuffer[0] = 0x01;
                spiBuffer[1] = 0x0E;
                spiBuffer[2] = 0x00;
                spiBuffer[3] = 0x00;
                spiBuffer[4] = 0x00;
                KernelSPISendPackage(spiBuffer, 512);
                usleep(2000);
                break;

            case STATE_MACHINE_VERSION:
                PR("Version.\n");

                memset(spiBuffer, 0xFE, 512);
                spiBuffer[0] = 0x01;
                spiBuffer[1] = 0x0F;
                spiBuffer[2] = 0x00;
                spiBuffer[3] = 0x00;
                spiBuffer[4] = 0x00;
                spiBuffer[5] = 0x02;
                spiBuffer[6] = 0x00;
                spiBuffer[7] = 0x01;
                spiBuffer[8] = 0x01;
                KernelSPISendPackage(spiBuffer, 512);
                usleep(2000);
                break;

            case STATE_MACHINE_SWITCH_MODE:
                if ((dpi != ConfigMessage.dpi) || (color != ConfigMessage.color)) {
                    dpi = ConfigMessage.dpi;
                    color = ConfigMessage.color;
                    PR("Switch mode: %d dpi, %c color.\n", dpi, color);
                    KernelVIDEOWriteScanPara(dpi, color);
                    LightAdjustmentLoad(dpi, color);
                    CorrectionParaLoad(dpi, color, CORRECTION_PARA_CHANNEL_1);
                }
                else {
                    PR("Don't need to switch mode.\n");
                }

                memset(spiBuffer, 0xFE, 512);
                spiBuffer[0] = 0x01;
                spiBuffer[1] = 0x13;
                spiBuffer[2] = 0x00;
                spiBuffer[3] = 0x00;
                spiBuffer[4] = 0x00;
                KernelSPISendPackage(spiBuffer, 512);
                usleep(2000);
                break;

            default:
                break;
        }

        /* Reset for next command. */
        if (stateMachine == STATE_MACHINE_SCAN) {
            pthread_mutex_lock(&scanMutex);
            pthread_cond_wait(&scanCond, &scanMutex);
            pthread_mutex_unlock(&scanMutex);
            KernelVIDEOStopScan();

            if (ConfigMessage.pageOne != PAGE_NULL)
                JpegReset(&JPEGCompressOne, &JPEGErrorOne, &JPEGDestinationOne, &JpegLengthOne);
            if (ConfigMessage.pageTwo != PAGE_NULL)
                JpegReset(&JPEGCompressTwo, &JPEGErrorTwo, &JPEGDestinationTwo, &JpegLengthTwo);
            ConfigMessage.pageOne = PAGE_NULL;
            ConfigMessage.pageTwo = PAGE_NULL;
            usleep(2000);
        }
        KernelVIDEOReset();
        KernelSPIResetAllFifo();
        if (stateMachine == STATE_MACHINE_CORRECTION_DARK) {
            request = 0;
            KernelSPIIOPulse();
        }
        else if (stateMachine == STATE_MACHINE_CORRECTION_BRIGHT) {
            request = 0;
            if (!(correctionStep % 2)) {
                PR("correction mix.\n");
                CorrectionNoPaperDataLoad(&CorrectionParaHandle);
                RecoverSample(CorrectionParaHandle.RK_Ext, CorrectionParaHandle.RK, CorrectionParaHandle.sampleWidth);
                CorrectionMix(CorrectionParaHandle.RK, CorrectionParaHandle.RB, CorrectionParaHandle.sampleWidth, 255);
                if (CorrectionParaHandle.imageAttr.depth == 3) {
                    RecoverSample(CorrectionParaHandle.GK_Ext, CorrectionParaHandle.GK, CorrectionParaHandle.sampleWidth);
                    CorrectionMix(CorrectionParaHandle.GK, CorrectionParaHandle.GB, CorrectionParaHandle.sampleWidth, 255);
                    RecoverSample(CorrectionParaHandle.BK_Ext, CorrectionParaHandle.BK, CorrectionParaHandle.sampleWidth);
                    CorrectionMix(CorrectionParaHandle.BK, CorrectionParaHandle.BB, CorrectionParaHandle.sampleWidth, 255);
                }
                CorrectionParaSave(&CorrectionParaHandle);
                CorrectionParaDeInit(&CorrectionParaHandle);
                request = 1;
            }
            KernelSPIIOPulse();
        }
        else if (stateMachine == STATE_MACHINE_SLEEP) {
            PR("Go to sleep.\n");
            system("echo standby > /sys/power/state");
            usleep(100*1000);
        }
        stateMachine = STATE_MACHINE_IDLE;
#if DEBUG_LEVEL <= 0
        StateMachineGetTime();
#endif
    }
}

void *ThreadSendData(void)
{
    unsigned int spiLength = 0;
    const int delayTime = 1000;
    struct jpeg_destination_mgr mem_dst;
    PR("Hello world, here's thread!\n");

    /* Send jpeg data of page one */
    while (CompressProcess.step == COMPRESS_STEP_P1_ENCODE) {
        pthread_mutex_lock(&sendPicture1Mutex);
        mem_dst = *JPEGCompressOne.dest;
        mem_dst.term_destination(&JPEGCompressOne);
        if (JpegLengthOne >= spiLength + 5120) {
            KernelSPISendPackage(JPEGDestinationOne + spiLength, 512);
            spiLength += 512;
        }
        pthread_mutex_unlock(&sendPicture1Mutex);
        usleep(delayTime);
    }

    while (JpegLengthOne >= spiLength + 512) {
        KernelSPISendPackage(JPEGDestinationOne + spiLength, 512);
        spiLength += 512;
        usleep(delayTime);
    }
    PR("JPG length: %ld %d\n", JpegLengthOne, spiLength);

    memset(spiBuffer, 0xFF, 512);
    memcpy(spiBuffer, JPEGDestinationOne + spiLength, JpegLengthOne - spiLength);
    KernelSPISendPackage(spiBuffer, 512);
    usleep(delayTime);
    memset(spiBuffer, 0xFE, 512);
    KernelSPISendPackage(spiBuffer, 512);
    usleep(delayTime);

#if DEBUG_LEVEL <= 0
    KernelSPIShowTxQueueInformation();
#endif
    while (!(KernelSPIIsTxQueueEmpty)) {
        KernelSPISendMessageInTxQueue();
        usleep(delayTime);
    }
#if DEBUG_LEVEL <= 0
    KernelSPIShowTxQueueInformation();
#endif

    /* Quit, if page two isn't available*/
    if (ConfigMessage.pageTwo == PAGE_NULL) {
        pthread_mutex_lock(&scanMutex);
        pthread_cond_signal(&scanCond);
        pthread_mutex_unlock(&scanMutex);
        return NULL;
    }

    /* Send jpeg data of page two. */
    spiLength = 0;
    sem_wait(&scanSem);
    while (CompressProcess.step == COMPRESS_STEP_P2_ENCODE) {
        pthread_mutex_lock(&sendPicture2Mutex);
        mem_dst = *JPEGCompressTwo.dest;
        mem_dst.term_destination(&JPEGCompressTwo);
        if (JpegLengthTwo >= spiLength + 5120) {
            KernelSPISendPackage(JPEGDestinationTwo + spiLength, 512);
            spiLength += 512;
        }
        pthread_mutex_unlock(&sendPicture2Mutex);
        usleep(delayTime);
    }

    while (JpegLengthTwo >= spiLength + 512) {
        KernelSPISendPackage(JPEGDestinationTwo + spiLength, 512);
        spiLength += 512;
        usleep(delayTime);
    }
    PR("JPG length: %ld %d\n", JpegLengthTwo, spiLength);

    memset(spiBuffer, 0xFF, 512);
    memcpy(spiBuffer, JPEGDestinationTwo + spiLength, JpegLengthTwo - spiLength);
    KernelSPISendPackage(spiBuffer, 512);
    usleep(delayTime);
    memset(spiBuffer, 0xFE, 512);
    KernelSPISendPackage(spiBuffer, 512);
    usleep(delayTime);

#if DEBUG_LEVEL <= 0
    KernelSPIShowTxQueueInformation();
#endif
    while (!(KernelSPIIsTxQueueEmpty)) {
        KernelSPISendMessageInTxQueue();
        usleep(delayTime);
    }
#if DEBUG_LEVEL <= 0
    KernelSPIShowTxQueueInformation();
#endif

    /* Quit, if page two is available*/
    pthread_mutex_lock(&scanMutex);
    pthread_cond_signal(&scanCond);
    pthread_mutex_unlock(&scanMutex);
}

struct timeval tv1, tv2;
void StateMachineSetTime(void)
{
    gettimeofday(&tv1, NULL);
}

void StateMachineGetTime(void)
{
    __time_t time;
    gettimeofday(&tv2, NULL);
    time = tv2.tv_sec * 1000000 + tv2.tv_usec - tv1.tv_sec * 1000000 - tv1.tv_usec;
    printf("time: %li sec, %.6li usec.\n", time / 1000000, time % 1000000);
}