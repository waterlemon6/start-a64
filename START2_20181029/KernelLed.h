#ifndef __KERNEL_LED_H__
#define __KERNEL_LED_H__

#define LED_IOC_MAGIC 0xE0
#define LED_IOC_CMD0_STATE0 _IO(LED_IOC_MAGIC, 0)
#define LED_IOC_CMD1_STATE1 _IO(LED_IOC_MAGIC, 1)
#define LED_IOC_CMD2_STATE2 _IO(LED_IOC_MAGIC, 2)
#define LED_IOC_CMD3_STATE3 _IO(LED_IOC_MAGIC, 3)

void KernelLEDRegister(int ledDescriptor);
void KernelLEDSetState(int state);

#endif
