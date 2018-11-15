#ifndef __KERNEL_INTERFACE_H__
#define __KERNEL_INTERFACE_H__

#define JTAG_IOC_MAGIC 0xE0
#define JTAG_IOC_CMD0 _IOR(JTAG_IOC_MAGIC, 0, unsigned int)

unsigned int *jtagGPIODataAdress;

static void TDI_WRITE(int x)
{
    if(x)
        *jtagGPIODataAdress |= (1 << 21);
    else
        *jtagGPIODataAdress &= ~(1 << 21);
}

static void TCK_WRITE(int x)
{
    if(x)
        *jtagGPIODataAdress |= (1 << 20);
    else
        *jtagGPIODataAdress &= ~(1 << 20);
}

static void TMS_WRITE(int x)
{
    if(x)
        *jtagGPIODataAdress |= (1 << 19);
    else
        *jtagGPIODataAdress &= ~(1 << 19);
}

static unsigned int TDO_READ(void)
{
    return ((*jtagGPIODataAdress >> 18) & 0x01);
}

void KernelJTAGSetDescriptor(int jtagDescriptor);
void KernelJTAGSetDataAddress(void);
unsigned int KernelJTAGGetGPIODataAddressOffsetInPage(int jtagDescriptor);

#endif
