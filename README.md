# Simple-i8042-Keyboard-Driver-on-Linux
#### Welcome to clone and give me a star by the way if you feel comfortable when you run this.
This is a Linux kernel module.

## File description
* **ziqi_KBD_module_irq.c** is the interrupt-based keyboard driver.
* **test.c** is the user level program to communicate with the kernel level keyboard driver.
* keyboard_driver_poll.c is the poll-based keyboard driver.
* keyboard_driver_poll_test.c is the user level program.

## What do this program do?
1. This is a keyboard driver in Linux kernel level.   
    The purpose is to load our own keyboard driver to provide keyboard service for the user.  
    We use a user level program to communicate with the kernel level module.
2. The keyboard should still work if we unload the primary Linux keyboard.  
3. The shift + letter should be the uppercase.
4. Backspace and Enter work.
5. After the termination of this program, the original keyboard should work as usual.

## How to run this driver?

### Dependency / Environment
1. Linux operating system (Linux puppy 2.6.33.2).
2. Virtual Machine: vBox.

### Step by step
1. make file.
    ```
    # make
    ```

2. Remove the module if it exits.
    ```
    # rmmod ziqi_KBD_module_irq
    ```
  
3. Insert my keyboard driver.
    ```
    # insmod ziqi_KBD_module_irq.ko
    ```

4. Compile.
    ```
    # gcc -o test test.c
    ```

5. Run the user space program.
    ```
    # ./test
    ```
