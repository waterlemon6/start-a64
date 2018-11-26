#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

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

static long	led_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);
static int led_open(struct inode *inode, struct file *filp);
static int led_release(struct inode *inode, struct file *filp);

static int led_init(void);
static void led_exit(void);

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = led_ioctl,
	.open = led_open,
	.release = led_release,
};

static struct miscdevice led_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "START2_LED",
	.fops = &led_fops,
};

static long	led_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	switch (_IOC_NR(cmd)) {
		case 0:
			gpio_direction_output(GPIOH(6), 1);
			gpio_direction_output(GPIOH(7), 1);
			break;
		case 1:
			gpio_direction_output(GPIOH(6), 0);
			gpio_direction_output(GPIOH(7), 1);
			break;
		case 2:
			gpio_direction_output(GPIOH(6), 1);
			gpio_direction_output(GPIOH(7), 0);
			break;
		case 3:
			gpio_direction_output(GPIOH(6), 0);
			gpio_direction_output(GPIOH(7), 0);
			break;
		default:
			printk(KERN_ALERT "[led] Led error cmd.\n");
			break;
	}
	return 0;
}

static int led_open(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "[led] Led open.\n");
	return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "[led] Led release.\n");
	return 0;
}

static int led_init(void)
{
	printk(KERN_ALERT "-------START2_LED Configuration.-------\n");

	gpio_request(GPIOH(6), NULL);
	gpio_request(GPIOH(7), NULL);
	gpio_direction_output(GPIOH(6), 1);
	gpio_direction_output(GPIOH(7), 1);

	misc_register(&led_miscdev);

	return 0;
}

static void led_exit(void)
{
	printk(KERN_ALERT "-------START2_LED Deconfiguration.-------\n");

	gpio_direction_output(GPIOH(6), 1);
	gpio_direction_output(GPIOH(7), 1);
	gpio_free(GPIOH(6));
	gpio_free(GPIOH(7));

	misc_deregister(&led_miscdev);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
