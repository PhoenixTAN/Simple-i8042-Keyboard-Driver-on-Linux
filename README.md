# Simple-i8042-Keyboard-Driver-on-Linux

A Linux kernel module.

# How to run this driver?

* ziqi_KBD_module_irq.c is the interrupt-based keyboard driver.
* test.c is the user level program to run.

```
# make
```

Remove the module if it exits.
```
# rmmod ziqi_KBD_module_irq
```
  
Insert my keyboard driver.
```
# insmod ziqi_KBD_module_irq.ko
```

Compile.
```
# gcc -o test test.c
```

Run the user space program.
```
# ./test
```
