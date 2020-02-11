#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h> /* error codes */
#include <linux/proc_fs.h>
#include <asm/uaccess.h>  /* copy_[to/from]_user() / get_user() / put_user() */
#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/kernel.h>	/* We are doing kernel work.*/
#include <asm/io.h>
#include <linux/interrupt.h>	/* interrupt handler */
#include <linux/workqueue.h>	
#include <linux/kallsyms.h>  /* kallsyms_lookup_name( ) */
#include <linux/slab.h>		/* memory allocation */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ziqi-Tan U88387934");
MODULE_DESCRIPTION("Use irq handler to write a keyboard driver.");

#define IOCTL_DATA _IOW(0, 6, struct ioctl_data)
#define IOCTL_EXIT _IOW(0, 7, int)
#define IOCTL_DISABLE_I8042 _IOW(0, 8, int)
#define KBD_IRQ 1		// i8042_kb_irq
#define SCANCODE 0x60		//
#define MAKE_STATUS 0x80	// // high order bit: 1 for "release", 0 for "make".
#define CAPS_LOCKS 0x3a		// capital lock scancode
#define SHIFT_MAKE 0x2a
#define SHIFT_BREAK 0xaa
#define RSHIFT_MAKE 0x36
#define RSHIFT_BREAK 0xb6
#define BACKSPACE_MAKE 0x0e
#define BACKSPACE_BREAK 0x8e
#define ENTER_MAKE 0x1c
#define ENTER_BREAK 0x9c
#define BUFFER_LEN 127

// structs
struct ioctl_data {
	char key;
	char key_stack[BUFFER_LEN];
	int top;
};

// stack and cooperations
typedef struct Stack {
	char data[BUFFER_LEN];
	int top;
} Stack;

Stack* newStack() {
	Stack* stack = kmalloc(sizeof(struct Stack), GFP_KERNEL);  // TODO
	if (!stack) {
		printk("Not enough memory.\n");
		return NULL;
	}
	stack->top = -1;
	return stack;
}

bool isFull(Stack* stack) {
	if (stack->top == BUFFER_LEN - 1) {
		printk("Stack is full.\n");
		return true;
	}
	return false;
}

bool isEmpty(Stack* stack) {
	if (stack->top == -1) {
		printk("Stack is empty.\n");
		return true;
	}
	return false;
}

bool push(Stack* stack, char ch) {
	if (isFull(stack)) {
		return false;
	}
	stack->top++;
	stack->data[stack->top] = ch;
	return true;
}

char pop(Stack* stack) {
	if (isEmpty(stack)) {
		return NULL;
	}
	return stack->data[stack->top--];
}

char peak(Stack* stack) {
	if (isEmpty(stack)) {
		return NULL;
	}
	return stack->data[stack->top];
}

void clearStack(Stack* stack) {
	while (!isEmpty(stack)) {
		pop(stack);
	}
}

void printStack(Stack* stack) {
	// from bottom to peak
	printk("Current stack: ");
	int size = stack->top;
	int i;
	for (i = 0; i <= size; i++) {
		printk("%c ", stack->data[i]);
	}
	printk("\n");
}

// static struct
static struct proc_dir_entry *proc_entry;
static struct file_operations pseudo_dev_proc_operations;

/*
	A static method to declare a wait queue.
	A "wait queue" in the Linux kernel is a data structure 
	to manage threads that are waiting for some condition to become true;
	they are the normal means by which threads block (or "sleep") in kernel space.
*/
DECLARE_WAIT_QUEUE_HEAD(wq);


static volatile int ev_press = 0;	// a wake up condition for the thread
// static variables
static unsigned char KBD_key;
static bool shift = false;
static bool cap = false;
static Stack* stack;

// functions
static int pseudo_device_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
irqreturn_t ir_getchar(int irq, void* dev);
void disable_i8042_interrupt();
void reset_i8042_interrupt();
struct irq_desc *(*my_irq_to_desc)(unsigned int);
irqreturn_t (*my_i8042_interrupt)(int, void*);
void* i8042_dev_id;

// initialization routine when loading the module
static int __init initialization_routine(void) {
    printk("<1> Loading module\n");
    
	pseudo_dev_proc_operations.ioctl = pseudo_device_ioctl;
    // Start create proc entry
    proc_entry = create_proc_entry("ioctl_kb_driver", 0444, NULL);
    if(!proc_entry) {
        printk("<1> Error creating /proc entry.\n");
        return 1;
    }

    //proc_entry->owner = THIS_MODULE; <-- This is now deprecated
    proc_entry->proc_fops = &pseudo_dev_proc_operations;
	
	// new a stack for input buffer
	stack = newStack();

	return 0;
}

void my_printk(char *string)
{
    struct tty_struct *my_tty;

    my_tty = current->signal->tty;

    if (my_tty != NULL) {
        (*my_tty->driver->ops->write)(my_tty, string, strlen(string));
        (*my_tty->driver->ops->write)(my_tty, "\015\012", 2);
    }
} 

// i8042_free_irqs and i8042_interrupt are defined in /drivers/input/serio/i8042.c
// void i8042_free_irqs(void);
// irqreturn_t i8042_interrupt(int irq, void *dev_id);
void disable_i8042_interrupt() {
	
	/* it does not work.
	printk("<1> i8042_free_irqs address: %x\n", kallsyms_lookup_name("i8042_free_irqs"));
	func1* my_i8042_free_irqs = (func1*)(kallsyms_lookup_name("i8042_free_irqs"));
	my_i8042_free_irqs();
	*/

	// kallsyms_lookup_name(string functionName) is used to find the address of the functionName.
	printk("<1> irq_to_desc address: %x\n", kallsyms_lookup_name("irq_to_desc"));
	my_irq_to_desc = kallsyms_lookup_name("irq_to_desc");
	unsigned int irq_num = 1;
	i8042_dev_id = my_irq_to_desc(irq_num)->action->dev_id;
		
	msleep(800);		// wait until the enter release
	/*
	 * Since the keyboard handler won't co-exist with another handler,
	 * such as us, we have to disable it (free its IRQ) before we do
	 * anything. Since we don't know where it is, there's no way to
	 * reinstate it later - so the computer will have to be rebooted
	 * when we're done.
	 */
	free_irq(1, i8042_dev_id);

	/*
	 * Request IRQ 1, the keyboard IRQ, to go to our irq_handler.
	 * SA_SHIRQ means we're willing to have othe handlers on this IRQ.
	 * SA_INTERRUPT can be used to make the handler into a fast interrupt.
	 */
	request_irq(
		1, // The number of the keyboard IRQ on PCs
		ir_getchar,
		IRQF_SHARED,  // flags that will instruct the kernel about the desired behaviour (flags)
		"KBD_IRQ", // our handler *dev_name
		(irqreturn_t*)ir_getchar // *dev_id
	);

	return ;
}

void reset_i8042_interrupt() {
	printk("<1> i8042_interrupt address: %x\n", kallsyms_lookup_name("i8042_interrupt"));	
	my_i8042_interrupt = kallsyms_lookup_name("i8042_interrupt");
	msleep(800);
	free_irq(1, (irqreturn_t*)ir_getchar);	// clean up customer irq handler
	request_irq(1, my_i8042_interrupt, IRQF_SHARED, "i8042", i8042_dev_id);

	return ;
}

// clean up routine when unloading the module
static void __exit cleanup_routine(void) {
    printk("<1> Dumping module\n");
    remove_proc_entry("ioctl_kb_driver", NULL);
	return;
}


/* 
 * Customer irq handler:
 * This function services keyboard interrupts. It reads the relevant
 * information from the keyboard and then puts the non time critical
 * part into the work queue. This will be run when the kernel considers it safe.
 */
irqreturn_t ir_getchar(int irq, void* dev) {

    static char scancode[128] = "\0\e1234567890-=\177\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	static char shift_scancode[128] = "\0\e!@#$%^&*()_+\177\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0|ZXCVBNM<>?\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

    unsigned char c = inb(SCANCODE);		// 0x60

	if ( !(c & MAKE_STATUS) ) {		//0x80
		// key make

		// shift make
		if (c == SHIFT_MAKE || c == RSHIFT_MAKE) {
			shift = true;
		}
		// capical lock
		if (c == CAPS_LOCKS) {
			cap = !cap;
		}

		bool uppercase = !((!shift && !cap) || (shift && cap));
		// shift + cap + letter = lowercase
		// shift + cap + other character = shift + other character

		if ( scancode[(int)c] >= 'a' && scancode[(int)c] <= 'z') {
			// alphabeical letters
			KBD_key = uppercase ? shift_scancode[(int)c] : scancode[(int)c];
		}
		else {
			// non-alphabeical letters
			KBD_key = shift ? shift_scancode[(int)c] : scancode[(int)c];
		}

		if (c == ENTER_MAKE) {
			clearStack(stack);
		}
		else if (c == BACKSPACE_MAKE) {
			pop(stack);
		}
		else {
			push(stack, KBD_key);
		}
		
		// when an interrupt occurs, wait up the event
		ev_press = 1;
		wake_up_interruptible(&wq);
	}
	else {
		// key release
		if (c == SHIFT_BREAK || c == RSHIFT_BREAK ) {
			shift = false;
		}
	}

    return IRQ_RETVAL(IRQ_HANDLED);
}

// ioctl entry
static int pseudo_device_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    struct ioctl_data ioc_data;
	// TODO
    ev_press = 0;

    switch (cmd){
		case IOCTL_DISABLE_I8042:
			printk("<1> diabling...\n");
			disable_i8042_interrupt();			
			break;
        
		case IOCTL_DATA:	
			// currently, ev_press is false.
			// wait for a true ev_press to end the sleep
			// CPU will not busy-waiting here because of this statement.
            wait_event_interruptible(wq, ev_press);		
            ev_press = 0;

            ioc_data.key = KBD_key;
			// deep copy the stack
			int i;
			ioc_data.top = stack->top;
			for (i = 0; i <= stack->top; i++) {
				ioc_data.key_stack[i] = stack->data[i];
			}
            copy_to_user((struct ioctl_data *)arg, &ioc_data, sizeof(struct ioctl_data));
            break;

		case IOCTL_EXIT:
			printk("<1> reseting...\n");
			reset_i8042_interrupt();
			break;

        default:
            return -EINVAL;
            break;
    }
    return 0;
}

module_init(initialization_routine); 
module_exit(cleanup_routine);