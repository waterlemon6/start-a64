#ifndef __KERNEL_VIDEO_H__
#define __KERNEL_VIDEO_H__

#define VIDEO_IOC_MAGIC 0xE0
#define VIDEO_IOC_CMD0_WRITE_REG                _IOW(VIDEO_IOC_MAGIC, 0, unsigned int)
#define VIDEO_IOC_CMD1_GET_IMAGE_OFFSET_IN_PAGE _IOR(VIDEO_IOC_MAGIC, 1, unsigned int)
#define VIDEO_IOC_CMD2_ENABLE                   _IO(VIDEO_IOC_MAGIC, 2)
#define VIDEO_IOC_CMD3_RESUME                   _IO(VIDEO_IOC_MAGIC, 3)
#define VIDEO_IOC_CMD4_DISABLE                  _IO(VIDEO_IOC_MAGIC, 4)

void KernelVIDEORegister(int videoDescriptor);

void KernelVIDEOStartScan(unsigned short VD);
void KernelVIDEOStopScan(void);

void KernelVIDEOWriteReg(unsigned short address, unsigned short value);
void KernelVIDEOWriteData(unsigned char *data, unsigned int size);
void KernelVIDEOWriteConfigPara(void);
void KernelVIDEOWriteScanPara(int dpi, char color);
void KernelVIDEOWriteLightPara(unsigned short r1, unsigned short r2, unsigned short g1, unsigned short g2,
                               unsigned short b1, unsigned short b2, unsigned short ir1, unsigned short ir2);
void KernelVIDEOEnableCorrection(void);
void KernelVIDEODisableCorrection(void);
void KernelVIDEOWriteCorrectionPara(unsigned char *srcK, unsigned char *srcB, int length, unsigned short cmd);

unsigned int KernelVIDEOGetImageOffsetInPage(void);
void KernelVIDEOEnable(void);
void KernelVIDEOResume(void);
void KernelVIDEODisable(void);
void KernelVIDEOReset(void);

#endif
