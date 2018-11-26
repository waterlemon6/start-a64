#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/clk.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dma/sunxi-dma.h>

struct spi_msg_t {
	unsigned char data[512];
	struct list_head queue;
};

struct dma_info_t {
	bool busy;
	int count_cb;
	int nent;
	struct dma_chan *chan;
	struct scatterlist sg[32];
	struct platform_device *dev;
};

void bsp_spi_init(struct platform_device *pdev);
void bsp_spi_gpio_init(struct device *dev);
void bsp_spi_gpio_deinit(struct device *dev);
void bsp_spi_sysclk_init(struct device *dev, unsigned int mclk);
void bsp_spi_enable(volatile unsigned char *base_addr);
void bsp_spi_disable(volatile unsigned char *base_addr);
void bsp_spi_slave_mode(volatile unsigned char *base_addr);
void bsp_spi_slave_transferctrl(volatile unsigned char *base_addr);
void bsp_spi_reset_txfifo(volatile unsigned char *base_addr);
void bsp_spi_reset_rxfifo(volatile unsigned char *base_addr);
unsigned int bsp_spi_query_txfifo(volatile unsigned char *base_addr);
unsigned int bsp_spi_query_rxfifo(volatile unsigned char *base_addr);

int bsp_spi_dmg_sg_cnt(void *addr, int len);
void bsp_spi_dma_init_sg(void *addr, int len, struct dma_info_t *dma);
void bsp_spi_cb_txdma(void* data);
void bsp_spi_prepare_txdma(volatile unsigned char *base_addr);
void bsp_spi_start_txdma(unsigned char* buffer, unsigned int length);

void bsp_io_init(void);
void bsp_io_deinit(void);
void bsp_io_pulse(void);

volatile unsigned int standby_state = 0;
volatile unsigned char *spi_virt_addr;
struct list_head queue_tx;
struct dma_info_t dma_tx;

/****************************************************************************************
 *
 *									BSP SPI Slave Device
 *
 ***************************************************************************************/
void bsp_spi_init(struct platform_device *pdev)
{
	spi_virt_addr = ioremap_nocache(0x01C69000, 0x400);

	bsp_spi_gpio_init(&pdev->dev);
	bsp_io_init();
	bsp_spi_sysclk_init(&pdev->dev, 100*1000*1000);

	bsp_spi_enable(spi_virt_addr);
	bsp_spi_slave_mode(spi_virt_addr);
	bsp_spi_slave_transferctrl(spi_virt_addr);

	bsp_spi_prepare_txdma(spi_virt_addr);
	dma_tx.dev = pdev;
}

void bsp_spi_gpio_init(struct device *dev)
{
	struct pinctrl *pctrl = NULL;
	struct pinctrl_state *pctrl_state = NULL;
	int ret = 0;

	pctrl = devm_pinctrl_get(dev);
	if (IS_ERR(pctrl)) {
		printk(KERN_ALERT "[spi] Error in pinctrl.\n");
		return;
	}

	pctrl_state = pinctrl_lookup_state(pctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR(pctrl_state)) {
		printk(KERN_ALERT "[spi] Error in pinctrl lookup state.\n");
		return;
	}

	ret = pinctrl_select_state(pctrl, pctrl_state);
	if (ret < 0) {
		printk(KERN_ALERT "[spi] Error in pinctrl select state.\n");
		return;
	}
}

void bsp_spi_gpio_deinit(struct device *dev)
{
	struct pinctrl *pctrl = NULL;
	struct pinctrl_state *pctrl_state = NULL;
	int ret = 0;

	pctrl = devm_pinctrl_get(dev);
	if (IS_ERR(pctrl)) {
		printk(KERN_ALERT "[spi] Error in pinctrl.\n");
		return;
	}

	pctrl_state = pinctrl_lookup_state(pctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR(pctrl_state)) {
		printk(KERN_ALERT "[spi] Error in pinctrl lookup state.\n");
		return;
	}

	ret = pinctrl_select_state(pctrl, pctrl_state);
	if (ret < 0) {
		printk(KERN_ALERT "[spi] Error in pinctrl select state.\n");
		return;
	}
}

void bsp_spi_sysclk_init(struct device *dev, unsigned int mod_clk)
{
	struct clk *pclk;  // PLL clock 	
	struct clk *mclk;  // spi module clock 
	int ret = 0;
	long rate = 0;

	pclk = of_clk_get(dev->of_node, 0);
	if (IS_ERR_OR_NULL(pclk)) {
		printk(KERN_ALERT "[spi] Error in pll clock.\n");
		return;
	}

	mclk = of_clk_get(dev->of_node, 1);
	if (IS_ERR_OR_NULL(mclk)) {
		printk(KERN_ALERT "[spi] Error in module clock.\n");
		return;
	}
	
	ret = clk_set_parent(mclk, pclk);
	if (ret != 0) {
		printk(KERN_ALERT "[spi] Error in clock parent.\n");
		return;
	}

	rate = clk_round_rate(mclk, mod_clk);
	ret = clk_set_rate(mclk, rate);
	if (ret != 0) {
		printk(KERN_ALERT "[spi] Error in clock rate.\n");
		return;
	}
	clk_prepare_enable(mclk);
}

void bsp_spi_enable(volatile unsigned char *base_addr)
{
	volatile unsigned char *addr = base_addr + 0x04;

	volatile unsigned int regval = readl((volatile void*)addr);
	regval |= (1 << 0);
	writel(regval, (volatile void*)addr);
}

void bsp_spi_disable(volatile unsigned char *base_addr)
{
	volatile unsigned char *addr = base_addr + 0x04;

	volatile unsigned int regval = readl((volatile void*)addr);
	regval &= ~(1 << 0);
	writel(regval, (volatile void*)addr);
}

void bsp_spi_slave_mode(volatile unsigned char *base_addr)
{
	volatile unsigned char *addr = base_addr + 0x04;

	volatile unsigned int regval = readl((volatile void*)addr);
	regval |= (0 << 1);
	writel(regval, (volatile void*)addr);
}

void bsp_spi_slave_transferctrl(volatile unsigned char *base_addr)
{
	volatile unsigned char *addr = base_addr + 0x08;
	volatile unsigned int regval = ((0 << 31)| //XCH
						   (0 << 12)| //FBS
						   (0 << 11)| //SDC
						   (0 << 10)| //RPSM
						   (0 << 9) | //DDB
						   (0 << 8) | //DHB
						   (1 << 7) | //SS_LEVEL
						   (1 << 6) | //SS_OWNER
						   (0 << 4) | //SS_SEL
						   (0 << 3) | //SSCTL
						   (1 << 2) | //SPOL
						   (0 << 1) | //CPOL
						   (0 << 0)); //CPHA
	writel(regval, (volatile void*)addr);
}

void bsp_spi_reset_txfifo(volatile unsigned char *base_addr)
{
	volatile unsigned char *addr = base_addr + 0x18;

	volatile unsigned int regval = readl((volatile void*)addr);
    regval |= (1 << 31);
    writel(regval, (volatile void*)addr);
}

void bsp_spi_reset_rxfifo(volatile unsigned char *base_addr)
{
	volatile unsigned char *addr = base_addr + 0x18;

	volatile unsigned int regval = readl((volatile void*)addr);
    regval |= (1 << 15);
    writel(regval, (volatile void*)addr);
}

unsigned int bsp_spi_query_txfifo(volatile unsigned char *base_addr)
{
	volatile unsigned char *addr = base_addr + 0x1C;	
	
    volatile unsigned int regval = readl((volatile void*)addr);
	regval = (regval >> 16) & 0xFF;
    return regval;
}

unsigned int bsp_spi_query_rxfifo(volatile unsigned char *base_addr)
{
    volatile unsigned char *addr = base_addr + 0x1C;	
	
    volatile unsigned int regval = readl((volatile void*)addr);
	regval = (regval >> 0) & 0xFF;
    return regval;
}

/****************************************************************************************
 *
 *									BSP SPI DMA
 *
 ***************************************************************************************/
int bsp_spi_dmg_sg_cnt(void *addr, int len)
{
	int npages = 0;
	char *bufp = (char *)addr;
	int mapbytes = 0;
	int bytesleft = len;

	while (bytesleft > 0) {
		if (bytesleft < (PAGE_SIZE - offset_in_page(bufp)))
			mapbytes = bytesleft;
		else
			mapbytes = PAGE_SIZE - offset_in_page(bufp);

		npages++;
		bufp += mapbytes;
		bytesleft -= mapbytes;
	}
	return npages;
}

void bsp_spi_dma_init_sg(void *addr, int len, struct dma_info_t *dma)
{
	int i;
	int npages = 0;
	void *bufp = addr;
	int mapbytes = 0;
	int bytesleft = len;

	npages = bsp_spi_dmg_sg_cnt(addr, len);
	if (npages > 32)
		npages = 32;

	sg_init_table(dma->sg, npages);
	for (i = 0; i < npages; i++) {
		if (bytesleft < (PAGE_SIZE - offset_in_page(bufp)))
			mapbytes = bytesleft;
		else
			mapbytes = PAGE_SIZE - offset_in_page(bufp);

		if (virt_addr_valid(bufp))
			sg_set_page(&dma->sg[i], virt_to_page(bufp), mapbytes, offset_in_page(bufp));
		else
			sg_set_page(&dma->sg[i], vmalloc_to_page(bufp), mapbytes, offset_in_page(bufp));
		
		bufp += mapbytes;
		bytesleft -= mapbytes;
	}

	dma->nent = npages;
	return;
}

void bsp_spi_cb_txdma(void* data)
{
	struct spi_msg_t *msg;
	dma_tx.count_cb++;

	if (!list_empty(&queue_tx)) {
		msg = list_entry(queue_tx.next, struct spi_msg_t, queue);
		if (msg != NULL) {
			list_del(&(msg->queue));
			kfree(msg);
		}
		else
			printk(KERN_ALERT "[spi] Error in tx dma callback, spi message list is error.\n");
	}
	else
		printk(KERN_ALERT "[spi] Error in tx dma callback, spi message list is empty.\n");

	dma_unmap_sg(&dma_tx.dev->dev, dma_tx.sg, dma_tx.nent, DMA_TO_DEVICE);
	dma_tx.busy = 0;
}

void bsp_spi_prepare_txdma(volatile unsigned char *base_addr)
{
	dma_cap_mask_t mask;
	struct dma_slave_config dma_conf = {0};
	volatile unsigned char *addr = base_addr + 0x18;

	volatile unsigned int regval = readl((volatile void*)addr);
	regval |= (1 << 24);
    writel(regval, (volatile void*)addr);

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_tx.chan = dma_request_channel(mask, NULL, NULL);
	if (dma_tx.chan == NULL) {
		printk(KERN_ALERT "[spi] Error in dma channel request.\n");
		return;
	}

	dma_conf.direction = DMA_MEM_TO_DEV;
	dma_conf.dst_addr = 0x01C69000 + 0x200;
	dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma_conf.src_maxburst = 1;
	dma_conf.dst_maxburst = 1;
	dma_conf.slave_id = ((DRQDST_SPI1TX << 16) | (DRQSRC_SDRAM));
	dmaengine_slave_config(dma_tx.chan, &dma_conf);
}

void bsp_spi_start_txdma(unsigned char* buffer, unsigned int length)
{
	int nents = 0;
	struct dma_async_tx_descriptor *dma_desc = NULL;

	bsp_spi_dma_init_sg(buffer, length, &dma_tx);
	nents = dma_map_sg(&dma_tx.dev->dev, dma_tx.sg, dma_tx.nent, DMA_TO_DEVICE);
	if (!nents) {
		printk(KERN_ALERT "[spi] Error in dma map sg.\n");
		return;
	}
	
	dma_desc = dmaengine_prep_slave_sg(dma_tx.chan, dma_tx.sg, nents, DMA_TO_DEVICE, DMA_PREP_INTERRUPT|DMA_CTRL_ACK);
	if (!dma_desc) {
		printk(KERN_ALERT "[spi] Error in dma descriptor.\n");
		return;
	}

	dma_desc->callback = bsp_spi_cb_txdma;
	dmaengine_submit(dma_desc);
	dma_async_issue_pending(dma_tx.chan);
}

/****************************************************************************************
 *
 *										IO
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

void bsp_io_init(void)
{
	gpio_request(GPIOD(5), NULL);
	gpio_direction_output(GPIOD(5), 0);
}

void bsp_io_deinit(void)
{
	gpio_free(GPIOD(5));
}

void bsp_io_pulse(void)
{
	gpio_direction_output(GPIOD(5), 1);
	udelay(1);
	gpio_direction_output(GPIOD(5), 0);
}

/****************************************************************************************
 *
 *										SPI
 *
 ***************************************************************************************/
static int spi_suspend(struct device *dev);
static int spi_resume(struct device *dev);

static int spi_open(struct inode *inode, struct file *filp);
static int spi_release(struct inode *inode, struct file *filp);
static long	spi_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);
static ssize_t spi_read(struct file *filp, char __user *buffer, size_t count, loff_t *position);
static ssize_t spi_write(struct file *filp, const char __user *buffer, size_t count, loff_t *position);

static int spi_probe(struct platform_device *pdev);
static int spi_remove(struct platform_device *pdev);
static int spi_init(void);
static void spi_exit(void);

static const struct dev_pm_ops spi_dev_pm_ops = {
	.suspend = spi_suspend,
	.resume  = spi_resume,
};

static const struct of_device_id spi_match[] = {
	{ .compatible = "allwinner,sun8i-spi", },
	{ .compatible = "allwinner,sun50i-spi", },
	{},
};
MODULE_DEVICE_TABLE(of, spi_match);

static struct platform_driver spi_driver = {
	.probe   = spi_probe,
	.remove  = spi_remove,
	.driver = {
        .name	= "spi",
		.owner	= THIS_MODULE,
		.pm		= &spi_dev_pm_ops,
		.of_match_table = spi_match,
	},
};

static struct file_operations spi_fops = {
	.owner = THIS_MODULE,
	.open = spi_open,
	.release = spi_release,
	.unlocked_ioctl = spi_ioctl,
	.read = spi_read,
	.write = spi_write,
};

static struct miscdevice spi_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "START2_SPI",
	.fops = &spi_fops,
};

static int spi_suspend(struct device *dev)
{
	dma_release_channel(dma_tx.chan);
	bsp_spi_disable(spi_virt_addr);
	bsp_io_deinit();
	bsp_spi_gpio_deinit(dev);
	return 0;
}

static int spi_resume(struct device *dev)
{
	bsp_spi_gpio_init(dev);
	bsp_io_init();
	bsp_spi_enable(spi_virt_addr);
	bsp_spi_prepare_txdma(spi_virt_addr);

	bsp_spi_reset_txfifo(spi_virt_addr);
	bsp_spi_reset_rxfifo(spi_virt_addr);
	dma_tx.busy = 0;
	standby_state = 1;
	return 0;
}

static int spi_open(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "[spi] Spi open.\n");

	bsp_spi_reset_txfifo(spi_virt_addr);
	bsp_spi_reset_rxfifo(spi_virt_addr);
	dma_tx.busy = 0;
	return 0;
}

static int spi_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "[spi] Spi release.\n");
	return 0;
}

static long	spi_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	unsigned int tmp;
	struct spi_msg_t *msg;

	switch (_IOC_NR(cmd)) {
		case 0:
			bsp_spi_reset_txfifo(spi_virt_addr);
			dma_tx.busy = 0;
			break;
		case 1:
			bsp_spi_reset_rxfifo(spi_virt_addr);
			break;
		case 2:
			tmp = bsp_spi_query_txfifo(spi_virt_addr);
			__put_user(tmp, (__u32 __user *)arg);;
			break;
		case 3:
			tmp = bsp_spi_query_rxfifo(spi_virt_addr);
			__put_user(tmp, (__u32 __user *)arg);
			break;
		case 4:
			tmp = list_empty(&queue_tx);
			__put_user(tmp, (__u32 __user *)arg);
			break;
		case 5:
			if ((dma_tx.busy == 0) && (bsp_spi_query_txfifo(spi_virt_addr) == 0)) {
				dma_tx.busy = 1;
				msg = list_entry(queue_tx.next, struct spi_msg_t, queue);
				if (msg != NULL) {
					bsp_spi_reset_txfifo(spi_virt_addr);
					bsp_spi_start_txdma(msg->data, 512);
					bsp_io_pulse();
				}
				else
					printk(KERN_ALERT "[spi] Error in spi queue empty, spi message list is empty.\n");
			}
			break;
		case 6:
			printk(KERN_ALERT "[spi] Current package: %d, tx fifo: %d.\n", dma_tx.count_cb, bsp_spi_query_txfifo(spi_virt_addr));
			break;
		case 7:
			bsp_io_pulse();
			break;
		case 8:
			tmp = standby_state;
			__put_user(tmp, (__u32 __user *)arg);
			standby_state = 0;
			break;
		default:
			break;
	}
	return 0;
}

static ssize_t spi_read(struct file *filp, char __user *buffer, size_t count, loff_t *position)
{
	int i = 0;
	int ret = 0;
	volatile unsigned char *rxFIFO = spi_virt_addr + 0x300;
	unsigned char spi_buffer[64];

	if (count <= 64) {
		if (bsp_spi_query_rxfifo(spi_virt_addr) >= count) {
			for(i = 0; i < count; i++)
				spi_buffer[i] = readb(rxFIFO);
			ret = copy_to_user(buffer, spi_buffer, count);
			return ret;
		}
		else 
			return count;
	}
	else {
		printk(KERN_ALERT "[spi] Error in spi read, the size of buffer < 64.\n");
		return count;
	}
}

static ssize_t spi_write(struct file *filp, const char __user *buffer, size_t count, loff_t *position)
{
	int ret = 0;
	struct spi_msg_t *msg;

	if ((count > 64) && (count <= 512)) {
		//push the msg into the tail of the queue.
		msg = kmalloc(sizeof(struct spi_msg_t), GFP_KERNEL);
		ret = copy_from_user(msg->data, buffer, count);
		list_add_tail(&(msg->queue), &queue_tx);
	
		if ((dma_tx.busy == 0) && (bsp_spi_query_txfifo(spi_virt_addr) == 0)) {
			//pull the msg out of the head of the queue.
			dma_tx.busy = 1;
			msg = list_entry(queue_tx.next, struct spi_msg_t, queue);
			if(msg != NULL) {
				bsp_spi_reset_txfifo(spi_virt_addr);
				bsp_spi_start_txdma(msg->data, count);
				bsp_io_pulse();
			}
			else
				printk(KERN_ALERT "[spi] Error in spi write, spi message list is empty.\n");
		}

		return ret;
	}
	else {
		printk(KERN_ALERT "[spi] Error in spi write, 64 < the size of buffer <= 512.\n");
		return count;
	}
}

u64 spi_dma_mask = DMA_BIT_MASK(32);
static int spi_probe(struct platform_device *pdev)
{
	struct device_node *pnode = pdev->dev.of_node;

	if (pnode == NULL) {
		printk(KERN_ALERT "[spi] Error in spi probe, device node error.\n");
		return -1;
	}
	pdev->id = of_alias_get_id(pnode, "spi");
	printk(KERN_ALERT "[spi] Spi device node. %s  %s  %d\n", pnode->name, pnode->type, pdev->id);
	if (pdev->id != 1) {
		printk(KERN_ALERT "[spi] Error in spi probe, device id error.\n");
		return -1;
	}

	pdev->dev.dma_mask = &spi_dma_mask;
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	bsp_spi_init(pdev);

	return 0;
}

static int spi_remove(struct platform_device *pdev)
{
	printk(KERN_ALERT "[spi] Spi remove.\n");
	return 0;
}

static int spi_init(void)
{
	printk(KERN_ALERT "-------START2_SPI Configuration.-------\n");
	platform_driver_register(&spi_driver);
	INIT_LIST_HEAD(&queue_tx);
	misc_register(&spi_miscdev);
	return 0;
}

static void spi_exit(void)
{
	printk(KERN_ALERT "-------START2_SPI Deconfiguration.-------\n");
	misc_deregister(&spi_miscdev);
}

module_init(spi_init);
module_exit(spi_exit);

MODULE_LICENSE("GPL");
