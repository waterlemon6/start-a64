#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <asm/cacheflush.h>

#define VIDEO_PORT_WIDTH 5184
#define VIDEO_PORT_HEIGHT 8000

void bsp_csi_init(void);
void bsp_clk_init(void);
void bsp_gpio_init(void);
void bsp_csireg_init(void);
void bsp_csi_enable_module(void);
void bsp_csi_disable_module(void);
void bsp_csi_capture_video_start(void);
void bsp_csi_capture_video_stop(void);
void bsp_csi_set_buffer(unsigned char *buf);

void spi_gpio_init(void);
void spi_reg_write_word(unsigned int data);

unsigned char* imageBuf;
unsigned int offsetInPage;

/****************************************************************************************
 *
 *										BSP
 *
 ***************************************************************************************/
volatile uint32_t *ccu_base;
volatile uint32_t *csi_gpio_base;
volatile uint32_t *csi_base;

void bsp_csi_init(void)
{
	bsp_clk_init();
	bsp_gpio_init();
	bsp_csireg_init();
	bsp_csi_set_buffer(imageBuf);
	bsp_csi_capture_video_start();
}

#define CCU_BASE_ADDR		0x01C20000

#define CCU_PllVideo0		0x0010
#define CCU_PllVideo1		0x0030
#define CCU_BusColockGate0	0x0060
#define CCU_BusColockGate1	0x0064
#define CCU_DramColockGate	0x0100
#define CCU_CsiMiscColock	0x0130
#define CCU_CsiColock		0x0134
#define CCU_BusSwReset1		0x02C4
#define CCU_BusSwReset0		0x02C0

void bsp_clk_init(void)
{
    ccu_base = ioremap_nocache(CCU_BASE_ADDR, 1024);
	
	*(ccu_base + CCU_BusColockGate0/4) |= (1 << 6);
	msleep(10);
	*(ccu_base + CCU_BusColockGate1/4) |= (1 << 8);
	msleep(10);
	*(ccu_base + CCU_DramColockGate/4) |= (2 << 0);
	msleep(10);
	*(ccu_base + CCU_CsiMiscColock/4) |= 0x80000000;
	msleep(10);
	*(ccu_base + CCU_CsiColock/4) |= 0x00018205;
	msleep(10);
	*(ccu_base + CCU_CsiColock/4) |= 0x80000000;
	msleep(10);
	*(ccu_base + CCU_BusSwReset1/4) |= (1 << 8);
	msleep(10);
	*(ccu_base + CCU_BusSwReset0/4) |= (1 << 6);
	msleep(10);
}

void bsp_gpio_init(void)
{
	csi_gpio_base = ioremap_nocache(0x01C20800 + 0x90, 36);

	*csi_gpio_base = 0x22222222;
    *(csi_gpio_base + 1) &= (~0x00ffffff);
    *(csi_gpio_base + 1) |= 0x00222222;
	*(csi_gpio_base + 7) = 0;
	*(csi_gpio_base + 8) = 0;
}

#define CSI_BASE_ADDR		0x01CB0000

#define CSI_EN_REG			0x00
#define CSI_IF_CFG_REG		0x04
#define CSI_CAP_REG			0x08
#define CSI_FIFO_THRS_REG	0x10
#define CSI_PTN_LEN_REG		0x30
#define CSI_PTN_ADDR_REG	0x34
#define CSI_CFG_REG			0x44
#define CSI_SCALE_REG		0x4C
#define CSI_F0_BUFA_REG		0x50
#define CSI_CAP_STA_REG		0x6C
#define CSI_INT_EN_REG		0x70
#define CSI_INT_STA_REG		0x74
#define CSI_HSIZE_REG		0x80
#define CSI_VSIZE_REG		0x84
#define CSI_BUF_LEN_REG		0x88
#define CSI_FLIP_SIZE_REG	0x8C

void bsp_csireg_init(void)
{
	csi_base = ioremap_nocache(CSI_BASE_ADDR, 1024);

	*(csi_base) = 0;
	*(csi_base + CSI_IF_CFG_REG/4) = ((1 << 16) | (0 << 17) | (0 << 18));
	*(csi_base + CSI_CFG_REG/4) = (2 << 10);
	*(csi_base + CSI_FIFO_THRS_REG/4) &= (~0xfff);
	*(csi_base + CSI_FIFO_THRS_REG/4) |= 0x400;
    
	*(csi_base + CSI_HSIZE_REG/4) = (VIDEO_PORT_WIDTH << 16);
	*(csi_base + CSI_VSIZE_REG/4) = (VIDEO_PORT_HEIGHT << 16);
	*(csi_base + CSI_BUF_LEN_REG/4) = ((VIDEO_PORT_WIDTH/2) << 16) | VIDEO_PORT_WIDTH;
	*(csi_base + CSI_FLIP_SIZE_REG/4) = 0x1e00280;
}

void bsp_csi_enable_module(void)
{
	*(csi_base) |= (1 << 0);
}

void bsp_csi_disable_module(void)
{
  	*(csi_base) &= ~(1 << 0);
}

void bsp_csi_capture_video_start(void)
{
    *(csi_base + CSI_CAP_REG/4) |= (1 << 1);
}

void bsp_csi_capture_video_stop(void)
{
    *(csi_base + CSI_CAP_REG/4) &= ~(1 << 1);
}

void bsp_csi_set_buffer(unsigned char *buf)
{
    *(csi_base + CSI_F0_BUFA_REG/4) = ((virt_to_phys(buf)-0x40000000) >> 2);
}

/****************************************************************************************
 *
 *										 SPI
 *
 ***************************************************************************************/
#define PA_BASE 0
#define PB_BASE 32
#define PC_BASE 64
#define PD_BASE 96
#define PE_BASE 128
#define PF_BASE 160
#define PG_BASE 192
#define PH_BASE 224
#define PL_BASE 352

#define GPIOA(n) PA_BASE+n
#define GPIOB(n) PB_BASE+n
#define GPIOC(n) PC_BASE+n
#define GPIOD(n) PD_BASE+n
#define GPIOE(n) PE_BASE+n
#define GPIOF(n) PF_BASE+n
#define GPIOG(n) PG_BASE+n
#define GPIOH(n) PH_BASE+n
#define GPIOL(n) PL_BASE+n

#define MOSI_DATA_ON	gpio_direction_output(GPIOE(12), 1)
#define MOSI_DATA_OFF	gpio_direction_output(GPIOE(12), 0)
#define MOSI_REG_ON		gpio_direction_output(GPIOE(13), 1)
#define MOSI_REG_OFF	gpio_direction_output(GPIOE(13), 0)
#define SCK_ON			gpio_direction_output(GPIOE(14), 1)
#define SCK_OFF			gpio_direction_output(GPIOE(14), 0)
#define CSN_ON			gpio_direction_output(GPIOE(15), 1)
#define CSN_OFF			gpio_direction_output(GPIOE(15), 0)

void spi_gpio_init(void)
{
	gpio_request(GPIOE(12), NULL);
	gpio_request(GPIOE(13), NULL);
	gpio_request(GPIOE(14), NULL);
	gpio_request(GPIOE(15), NULL);
	CSN_ON;
	SCK_OFF;
	MOSI_DATA_OFF;
	MOSI_REG_OFF;
}

void spi_reg_write_word(unsigned int data)
{
	unsigned int i;
  
	CSN_OFF;
	for (i = 0; i < 32; i++) {
		asm("nop");
		if (data & 0x80000000)
			MOSI_REG_ON;
		else
			MOSI_REG_OFF;

		asm("nop");
		SCK_ON;
		data <<= 1;
		asm("nop");
		SCK_OFF;
	}
	CSN_ON;
}

void spi_data_write_byte(unsigned char *data, unsigned int size)
{
	unsigned int i, j;
	unsigned char ch;
	
	for (i = 0; i < size; i++) {
		CSN_OFF;
		ch = data[i];
		for (j = 0; j < 8; j++) {
			asm("nop");
			if(ch & 0x80)
				MOSI_DATA_ON;
			else
				MOSI_DATA_OFF;
     	
			asm("nop");
			SCK_ON;
			ch <<= 1;
			asm("nop");
			SCK_OFF;
		}
		CSN_ON;
		asm("nop");
	}
}

/****************************************************************************************
 *
 *										VIDEO
 *
 ***************************************************************************************/
static int video_open(struct inode *inode, struct file *filp);
static int video_release(struct inode *inode, struct file *filp);
static ssize_t video_write(struct file *filp, const char __user *buffer, size_t count, loff_t *position);
static long video_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int video_mmap(struct file *filp, struct vm_area_struct *vma);

static struct file_operations video_fops = {
	.owner				= THIS_MODULE,
	.open 				= video_open,
	.write				= video_write,  
	.release			= video_release,
	.compat_ioctl		= video_ioctl,
	.mmap				= video_mmap,
};	

static struct miscdevice video_miscdev = {
	.minor				= MISC_DYNAMIC_MINOR,
	.name				= "START2_VIDEO",
	.fops				= &video_fops,
};

static int video_open(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "[video] Video open.\n");
	bsp_csi_enable_module();
	flush_cache_all();
	return 0;
}

static int video_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "[video] Video release.\n");
	bsp_csi_disable_module();
	return 0;
}

static ssize_t video_write(struct file *filp, const char __user *buffer, size_t count, loff_t *position)
{
	int ret;
	unsigned char* write_buffer;

	write_buffer = kzalloc(count, GFP_KERNEL);
	ret = copy_from_user(write_buffer, buffer, count);
	spi_data_write_byte(write_buffer, count);
	kfree(write_buffer);
	return ret;
}

static long	video_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned int temp;

	switch (_IOC_NR(cmd)) {
		case 0:
			__get_user(temp, (__u32 __user *)arg);
			spi_reg_write_word(temp);
			break;
		case 1:
			__put_user(offsetInPage, (__u32 __user *)arg);
			break;
		case 2:
			bsp_csi_enable_module();
			flush_cache_all();
			break;
		case 3:
			bsp_csi_init();
			bsp_csi_enable_module();
			flush_cache_all();
			break;
		case 4:
			bsp_csi_disable_module();
			break;
		default:
			break;
	}
	return 0;
}

static int video_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long start = vma->vm_start;  
	unsigned long size = PAGE_ALIGN(vma->vm_end - vma->vm_start);  

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	return remap_pfn_range(vma, start, (virt_to_phys(imageBuf) >> PAGE_SHIFT), size, vma->vm_page_prot);
}

extern unsigned char *pBootMemory;
static int video_init(void)
{
	printk(KERN_ALERT "-------START2_VIDEO Configuration.-------\n");

	//imageBuf = (u8*)kmalloc(VIDEO_PORT_WIDTH * VIDEO_PORT_HEIGHT, __GFP_DMA);
	imageBuf = pBootMemory;
	offsetInPage = virt_to_phys((void*)imageBuf) - (virt_to_phys((void*)imageBuf) & PAGE_MASK);
	memset(imageBuf, 0, VIDEO_PORT_WIDTH * VIDEO_PORT_HEIGHT);

	misc_register(&video_miscdev);
	spi_gpio_init();
	bsp_csi_init();

	return 0;
}

static void video_exit(void)
{
	printk(KERN_ALERT "-------START2_VIDEO Deconfiguration.\n-------");

	misc_deregister(&video_miscdev);
}

module_init(video_init);
module_exit(video_exit);

MODULE_LICENSE("GPL");
