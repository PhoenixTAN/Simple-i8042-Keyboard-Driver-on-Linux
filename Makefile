obj-m += keyboard_driver_poll.o
obj-m += ziqi_KBD_module_irq.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean