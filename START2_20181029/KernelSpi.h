#ifndef __KERNEL_SPI_H__
#define __KERNEL_SPI_H__

#define SPI_IOC_MAGIC 0xE0
#define SPI_IOC_CMD0_RESET_TX_FIFO              _IO(SPI_IOC_MAGIC, 0)
#define SPI_IOC_CMD1_RESET_RX_FIFO              _IO(SPI_IOC_MAGIC, 1)
#define SPI_IOC_CMD2_GET_BYTES_IN_TX_FIFO       _IOR(SPI_IOC_MAGIC, 2, unsigned int)
#define SPI_IOC_CMD3_GET_BYTES_IN_RX_FIFO       _IOR(SPI_IOC_MAGIC, 3, unsigned int)
#define SPI_IOC_CMD4_IS_TX_QUEUE_EMPTY          _IOR(SPI_IOC_MAGIC, 4, unsigned int)
#define SPI_IOC_CMD5_SEND_MESSAGE_IN_TX_QUEUE   _IO(SPI_IOC_MAGIC, 5)
#define SPI_IOC_CMD6_SHOW_TX_QUEUE_STATE        _IO(SPI_IOC_MAGIC, 6)
#define SPI_IOC_CMD7_IO_PULSE                   _IO(SPI_IOC_MAGIC, 7)
#define SPI_IOC_CMD8_GET_STANDBY_STATE          _IOR(SPI_IOC_MAGIC, 8, unsigned int)

void KernelSPIRegister(int spiDescriptor);

void KernelSPIResetTxFifo(void);
void KernelSPIResetRxFifo(void);
void KernelSPIResetAllFifo(void);

unsigned int KernelSPIGetBytesInTxFifo(void);
unsigned int KernelSPIGetBytesInRxFifo(void);

void KernelSPISendPackage(unsigned char* buffer, unsigned int size);
void KernelSPIReadPackage(unsigned char* buffer, unsigned int size);

unsigned int KernelSPIIsTxQueueEmpty(void);
void KernelSPISendMessageInTxQueue(void);
void KernelSPIShowTxQueueInformation(void);

void KernelSPIIOPulse(void);
unsigned int KernelSPIGetStandbyState(void);

unsigned int KernelSPIReadCommand(unsigned char* cmd, int rxfifoResetRequest);

#endif
