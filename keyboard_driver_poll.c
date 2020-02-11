/*
 *  keyboard driver test module -- Ziqi Tan.
 *  Function: 
 *      You are required to write yet another ioctl call called my_getchar(), 
 *		that simply polls the PC keyboard for characters. 
 *		When a key has been pressed (indicated by bit 0 of the keyboard controller status register accessed via port 0x64, using port-based I/O) 
 *		you want to return from a busy-waiting loop with the appropriate character.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h> /* error codes */
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/tty.h>
#include <linux/sched.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ziqi-Tan");

static inline unsigned char inb(unsigned short usPort) {

	unsigned char uch;

	asm volatile("inb %1,%0" : "=a" (uch) : "Nd" (usPort));
	return uch;
}

static inline void outb(unsigned char uch, unsigned short usPort) {

	asm volatile("outb %0,%1" : : "a" (uch), "Nd" (usPort));

}

char my_getchar(void) {

	char c;

	static char scancode[128] = "\0\e1234567890-=\177\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

	/* Poll keyboard status register at port 0x64 checking bit 0 to see if
	 * output buffer is full. We continue to poll if the msb of port 0x60
	 * (data port) is set, as this indicates out-of-band data or a release
	 * keystroke
	 */
	while (!(inb(0x64) & 0x1) || ((c = inb(0x60)) & 0x80));
	printk("ASCII %d\n", (int)scancode[(int)c]);
	return scancode[(int)c];
}

#define IOCTL_DATA _IOW(0, 6, struct ioctl_data)

static int pseudo_device_ioctl(struct inode* inode, struct file* file,
	unsigned int cmd, unsigned long arg);

void enable_irq(int irq_num);
void disable_irq(int irq_num);

/* attribute structures */
struct ioctl_data {
	char key;
};

static struct file_operations pseudo_dev_proc_operations;

static struct proc_dir_entry* proc_entry;

static int __init initialization_routine(void) {
	printk("<1> Loading module\n");
	
	pseudo_dev_proc_operations.ioctl = pseudo_device_ioctl;

	/* Start create proc entry */
	proc_entry = create_proc_entry("ioctl_keyboard_driver", 0444, NULL);
	if (!proc_entry)
	{
		printk("<1> Error creating /proc entry.\n");
		return 1;
	}

	//proc_entry->owner = THIS_MODULE; <-- This is now deprecated
	proc_entry->proc_fops = &pseudo_dev_proc_operations;

	return 0;
}

/* 'printk' version that prints to active tty. */
void my_printk(char* string)
{
	struct tty_struct* my_tty;

	my_tty = current->signal->tty;

	if (my_tty != NULL) {
		(*my_tty->driver->ops->write)(my_tty, string, strlen(string));
		(*my_tty->driver->ops->write)(my_tty, "\015\012", 2);
	}
}


static void __exit cleanup_routine(void) {

	printk("<1> Dumping module\n");
	remove_proc_entry("ioctl_keyboard_driver", NULL);

	return;
}


/***
 * ioctl() entry point...
 */
static int pseudo_device_ioctl(struct inode* inode, struct file* file,
	unsigned int cmd, unsigned long arg)
{

	struct ioctl_data ioc_data;

	switch (cmd) {

		case IOCTL_DATA:
			disable_irq(1);
			ioc_data.key = my_getchar();
			copy_to_user((struct ioctl_data*)arg, &ioc_data,
				sizeof(struct ioctl_data));

			// printk("Print from kernel Pressing %c", ioc_data.key);
			enable_irq(1);
			break;

		default:
			return -EINVAL;
			break;
	}

	return 0;
}

module_init(initialization_routine);
module_exit(cleanup_routine);