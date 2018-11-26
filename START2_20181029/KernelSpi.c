#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include "KernelSpi.h"
#include "main.h"

int m_spiDescriptor;

void KernelSPIRegister(int spiDescriptor)
{
    m_spiDescriptor = spiDescriptor;
}

void KernelSPIResetTxFifo(void)
{
    ioctl(m_spiDescriptor, SPI_IOC_CMD0_RESET_TX_FIFO);
}

void KernelSPIResetRxFifo(void)
{
    ioctl(m_spiDescriptor, SPI_IOC_CMD1_RESET_RX_FIFO);
}

void KernelSPIResetAllFifo(void)
{
    KernelSPIResetTxFifo();
    KernelSPIResetRxFifo();
}

unsigned int KernelSPIGetBytesInTxFifo(void)
{
    unsigned int bytes = 0;
    ioctl(m_spiDescriptor, SPI_IOC_CMD2_GET_BYTES_IN_TX_FIFO, &bytes);
    return bytes;
}

unsigned int KernelSPIGetBytesInRxFifo(void)
{
    unsigned int bytes = 0;
    ioctl(m_spiDescriptor, SPI_IOC_CMD3_GET_BYTES_IN_RX_FIFO, &bytes);
    return bytes;
}

void KernelSPISendPackage(unsigned char* buffer, unsigned int size)
{
    write(m_spiDescriptor, buffer, size);
}

void KernelSPIReadPackage(unsigned char* buffer, unsigned int size)
{
    read(m_spiDescriptor, buffer, size);
}

unsigned int KernelSPIIsTxQueueEmpty(void)
{
    unsigned int ret = 0;
    ioctl(m_spiDescriptor, SPI_IOC_CMD4_IS_TX_QUEUE_EMPTY, &ret);
    return ret;
}

void KernelSPISendMessageInTxQueue(void)
{
    ioctl(m_spiDescriptor, SPI_IOC_CMD5_SEND_MESSAGE_IN_TX_QUEUE);
}

void KernelSPIShowTxQueueInformation(void)
{
    ioctl(m_spiDescriptor, SPI_IOC_CMD6_SHOW_TX_QUEUE_STATE);
}

void KernelSPIIOPulse(void)
{
    ioctl(m_spiDescriptor, SPI_IOC_CMD7_IO_PULSE);
}

unsigned int KernelSPIGetStandbyState(void)
{
    unsigned int state = 0;
    ioctl(m_spiDescriptor, SPI_IOC_CMD8_GET_STANDBY_STATE, &state);
    return state;
}

unsigned int KernelSPIReadCommand(unsigned char* cmd, int rxFifoResetRequest)
{
    int i = 0;
    unsigned int bytes = 0;
    if(rxFifoResetRequest)
        KernelSPIResetRxFifo();
    usleep(1000);

    while(1) {
        bytes = KernelSPIGetBytesInRxFifo();

        if(bytes == 31) {
            read(m_spiDescriptor, cmd, bytes);
            PR("Scan Command: ");
            for(i = 0; i < bytes; i++)
                PR("%.2X ", cmd[i]);
            PR("\n");
            return bytes;
        }
        else if(bytes == 15) {
            usleep(1000);
            if(KernelSPIGetBytesInRxFifo() != 15)
                continue;

            read(m_spiDescriptor, cmd, bytes);
            PR("Control Command: ");
            for(i = 0; i < bytes; i++)
                PR("%.2X ", cmd[i]);
            PR("\n");
            return bytes;
        }
        else
            usleep(1000);
    }
}