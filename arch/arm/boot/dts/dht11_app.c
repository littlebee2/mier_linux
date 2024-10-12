#include<stdio.h>
#include <fcntl.h>
#include <unistd.h>
int main(int argc,char *argv[])
{
	unsigned char buf[5] = {0};
	int fd = open("/dev/dht11", O_RDWR);
	if (fd == -1)
	{
		perror("open");
		return -1;
	}
	printf("open success!\n");
	while(1)
	{
		int ret = read(fd, buf, 5);
		if (ret == -1)
		{
			perror("read");
			return -1;
		}
		/*校验和*/
		int crc = buf[0] + buf[1] + buf[2] + buf[3];
		int i = 0;
		if (crc & 0xFF == buf[4])
		{
			printf("humidity %d% \n", buf[0]);
			printf("temperatuere %dC\n", buf[2]);
		}
		sleep(2);
	}

	close(fd);
	return 0;
}
