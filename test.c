#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define BUFFER_LEN 127

struct ioctl_data {
	char key;
	char key_stack[BUFFER_LEN];
	int top;
};

#define IOCTL_DATA _IOW(0, 6, struct ioctl_data)
#define IOCTL_DISABLE_I8042 _IOW(0, 8, int)
#define IOCTL_EXIT _IOW(0, 7, int)

int main () {
	
	struct ioctl_data ioc_data;
	int data = 1;
  	int fd = open ("/proc/ioctl_kb_driver", O_RDONLY);
	ioctl(fd, IOCTL_DISABLE_I8042, &data);
	while(1) {
	  	ioctl (fd, IOCTL_DATA, &ioc_data);
	  	char ch = ioc_data.key;	  	
		
		system("clear");
		printf("\n# User space: ");
		int i;
		for (i = 0; i <= ioc_data.top; i++) {
			printf("%c", ioc_data.key_stack[i]);
		}
		printf("\n");
		// exit the program
		if (ioc_data.key_stack[0] == 'e' && ioc_data.key_stack[1] == 'x' && ioc_data.key_stack[2] == 'i' && ioc_data.key_stack[3] == 't' ) {			
			printf("\nProgram terminated!\n");
			break;
		}
  	}
	ioctl(fd, IOCTL_EXIT, &data);
  	return 0;
}