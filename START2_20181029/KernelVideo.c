#include <zconf.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include "KernelVideo.h"

int m_videoDescriptor;

void KernelVIDEORegister(int videoDescriptor)
{
    m_videoDescriptor = videoDescriptor;
}

void KernelVIDEOStartScan(unsigned short VD)
{
    KernelVIDEOWriteReg(1037, VD);
    KernelVIDEOWriteReg(1551, 1);
}

void KernelVIDEOStopScan(void)
{
    KernelVIDEOWriteReg(1551, 0);
}

void KernelVIDEOWriteReg(unsigned short address, unsigned short value)
{
    unsigned int data = ((unsigned int)address << 16) | value;
    ioctl(m_videoDescriptor, VIDEO_IOC_CMD0_WRITE_REG, &data);
}

void KernelVIDEOWriteData(unsigned char *data, unsigned int size)
{
    write(m_videoDescriptor, data, size);
}

void KernelVIDEOWriteConfigPara(void)
{
    KernelVIDEOWriteReg(0, 0);
    KernelVIDEOWriteReg(1024, 1333);//frequency
    KernelVIDEOWriteReg(1026, 1000);//R1
    KernelVIDEOWriteReg(1027, 1000);//R2
    KernelVIDEOWriteReg(1028, 800);//G1
    KernelVIDEOWriteReg(1029, 800);//G2
    KernelVIDEOWriteReg(1030, 600);//B1
    KernelVIDEOWriteReg(1031, 600);//B2
    KernelVIDEOWriteReg(1032, 800);//IR1 light
    KernelVIDEOWriteReg(1033, 800);//IR2 light
    KernelVIDEOWriteReg(1034, 6);//color space
    KernelVIDEOWriteReg(1035, 1);//dpi
    KernelVIDEOWriteReg(1036, 7200);//height, no effect
    KernelVIDEOWriteReg(1536, 36);//R1 gain, -128~127 ?
    KernelVIDEOWriteReg(1537, 36);//G1 gain
    KernelVIDEOWriteReg(1538, 36);//B1 gain
    KernelVIDEOWriteReg(1539, 127);//R1 offset, -128~127 ?
    KernelVIDEOWriteReg(1540, 127);//G1 offset
    KernelVIDEOWriteReg(1541, 127);//B1 offset
    KernelVIDEOWriteReg(1542, 36);//R2 gain
    KernelVIDEOWriteReg(1543, 36);//G2 gain
    KernelVIDEOWriteReg(1544, 36);//B2 gain
    KernelVIDEOWriteReg(1545, 127);//R2 offset
    KernelVIDEOWriteReg(1546, 127);//G2 offset
    KernelVIDEOWriteReg(1547, 127);//B2 offset
    KernelVIDEOWriteReg(1548, 15);//AD system
    KernelVIDEOWriteReg(1549, 0);//AD AFE
    KernelVIDEOWriteReg(1550, 1);//AD start
}

void KernelVIDEOWriteScanPara(int dpi, char color)
{
    unsigned short frequency = 0;

    switch (dpi) {
        case 200:
            dpi = 1;
            switch (color) {
                case 'C':
                    color = 6;
                    frequency = 1333;
                    break;
                case 'G':
                    color = 1;
                    frequency = 2400;
                    break;
                case 'I':
                    color = 2;
                    frequency = 2400;
                    break;
                default:
                    break;
            }
            break;
        case 300:
            dpi = 2;
            switch (color) {
                case 'C':
                    color = 6;
                    frequency = 2000;
                    break;
                case 'G':
                    color = 1;
                    frequency = 2000;
                    break;
                case 'I':
                    color = 2;
                    frequency = 2000;
                    break;
                default:
                    break;
            }
            break;
        case 600:
            dpi = 3;
            switch (color) {
                case 'C':
                    color = 6;
                    frequency = 4000;
                    break;
                case 'G':
                    color = 1;
                    frequency = 4000;
                    break;
                case 'I':
                    color = 2;
                    frequency = 4000;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    KernelVIDEOWriteReg(1024, frequency);
    KernelVIDEOWriteReg(1034, (unsigned short) color);
    KernelVIDEOWriteReg(1035, (unsigned short) dpi);
}

void KernelVIDEOWriteLightPara(unsigned short r1, unsigned short r2, unsigned short g1, unsigned short g2,
                               unsigned short b1, unsigned short b2, unsigned short ir1, unsigned short ir2)
{
    KernelVIDEOWriteReg(1026, r1);
    KernelVIDEOWriteReg(1027, r2);
    KernelVIDEOWriteReg(1028, g1);
    KernelVIDEOWriteReg(1029, g2);
    KernelVIDEOWriteReg(1030, b1);
    KernelVIDEOWriteReg(1031, b2);
    KernelVIDEOWriteReg(1032, ir1);
    KernelVIDEOWriteReg(1033, ir2);
}

void KernelVIDEOEnableCorrection(void)
{
    KernelVIDEOWriteReg(1552, 1);
    //KernelVIDEOWriteReg(1552, 3); // gray to white gradient, just for test.
}

void KernelVIDEODisableCorrection(void)
{
    KernelVIDEOWriteReg(1552, 0);
    //KernelVIDEOWriteReg(1552, 3); // gray to white gradient, just for test.
}

void KernelVIDEOWriteCorrectionPara(unsigned char *srcK, unsigned char *srcB, int length, unsigned short cmd)
{
    KernelVIDEOWriteReg(2048, cmd);
    /*for (int i = 0; i < length; i++) {
        srcK[i] = 0;
        srcB[i] = (unsigned char) (i % 64);
    }*/
    KernelVIDEOWriteData(srcK, (unsigned int)length);
    KernelVIDEOWriteData(srcB, (unsigned int)length);
    KernelVIDEOWriteReg(2049, 100);
}

unsigned int KernelVIDEOGetImageOffsetInPage(void)
{
    unsigned int offset = 0;
    ioctl(m_videoDescriptor, VIDEO_IOC_CMD1_GET_IMAGE_OFFSET_IN_PAGE, &offset);
    return offset;
}

void KernelVIDEOEnable(void)
{
    ioctl(m_videoDescriptor, VIDEO_IOC_CMD2_ENABLE);
}

void KernelVIDEOResume(void)
{
    ioctl(m_videoDescriptor, VIDEO_IOC_CMD3_RESUME);
}

void KernelVIDEODisable(void)
{
    ioctl(m_videoDescriptor, VIDEO_IOC_CMD4_DISABLE);
}

void KernelVIDEOReset(void)
{
    KernelVIDEODisable();
    usleep(1000);
    KernelVIDEOEnable();
}
