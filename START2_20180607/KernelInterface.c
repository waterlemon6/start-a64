#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "KernelInterface.h"

int m_jtagDescriptor;

void KernelJTAGSetDescriptor(int jtagDescriptor)
{
    m_jtagDescriptor = jtagDescriptor;
}

void KernelJTAGSetDataAddress(void)
{
    jtagGPIODataAdress = mmap(NULL, 0x04, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, m_jtagDescriptor, 0);
    jtagGPIODataAdress += KernelJTAGGetGPIODataAddressOffsetInPage(m_jtagDescriptor) / 4;
}

unsigned int KernelJTAGGetGPIODataAddressOffsetInPage(int jtagDescriptor)
{
    unsigned int offset = 0;
    ioctl(jtagDescriptor, JTAG_IOC_CMD0, &offset);
    return offset;
}