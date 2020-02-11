#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

struct ioctl_data {
	char keys;
};

#define IOCTL_DATA _IOW(0, 6, struct ioctl_data)

int main() {


	struct ioctl_data ioc_data;

	int fd = open("/proc/ioctl_keyboard_driver", O_RDONLY);

	while (1) {
		ioctl(fd, IOCTL_DATA, &ioc_data);
		char ch = ioc_data.keys;
		if (ch == '\n') {
			printf("\n User program end!");
			break;
		}
		else {
			printf("Print from user program: %c\n", ch);
		}
	}
	return 0;
}