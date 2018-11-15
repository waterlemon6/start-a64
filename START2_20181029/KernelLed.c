#include <sys/ioctl.h>
#include "KernelLed.h"

int m_ledDescriptor;

void KernelLEDRegister(int ledDescriptor)
{
    m_ledDescriptor = ledDescriptor;
}

void KernelLEDSetState(int state)
{
    switch (state) {
        case 0:
            ioctl(m_ledDescriptor, LED_IOC_CMD0_STATE0);
            break;
        case 1:
            ioctl(m_ledDescriptor, LED_IOC_CMD1_STATE1);
            break;
        case 2:
            ioctl(m_ledDescriptor, LED_IOC_CMD2_STATE2);
            break;
        case 3:
            ioctl(m_ledDescriptor, LED_IOC_CMD3_STATE3);
            break;
        default:
            break;
    }
}

