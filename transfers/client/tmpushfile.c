#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <linux/posix_types.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <errno.h>
#include <getopt.h>

#include "tmfile.h"
#include "progress_bar.h"
#include "crc16.h"

#if 0
static void uart_ioctl()
{

	int nbytes = 0;
	ioctl(uart_fd, FIONREAD, &nbytes);

	if (nbytes > 0)
	{
		printf("%s %d\n", __func__, nbytes);
	} else {
		printf("%s no data nbytes:%d\n", __func__, nbytes);
	}

}
#endif

int open_uart(char *pcDriverName)
{
	int fd;
	struct termios options;

	fd = open(pcDriverName, O_RDWR | O_NOCTTY); // block
	if (fd < 0)
	{
		perror("open uart");
		exit(1);
	}

	bzero(&options, sizeof(struct termios));

	tcgetattr(fd, &options);

	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);

	/* Make Raw Interface UART Interface */
	cfmakeraw(&options);

	/* Configure Speed */
	cfsetispeed(&options, B115200);              /* Input:  X Baud       */
	cfsetospeed(&options, B115200);              /* Output: X Baud       */

	/* Configure Control Options  */
	options.c_cflag |= (CLOCAL | CREAD);            /* Enable Receiver and set local mode */
	options.c_cflag |= PARENB;                      /* Even Parity          */
	options.c_cflag &= ~CSTOPB;                     /* Only one Stop Bit    */
	options.c_cflag &= ~CSIZE ;                     /* Char Size Mask       */
	options.c_cflag |=  CS8;                        /* 8 Databits           */

	/* Configure Hardware Flow    */
	options.c_cflag &= ~CRTSCTS;                    /* No RTS and CTS Steer */

	/* Configure Software Flow    */
	options.c_iflag &= ~(IGNBRK | BRKINT| PARMRK);  /* See termio.h         */
	options.c_iflag &= ~(IXON   | IXOFF | IXANY );  /* No RTS and CTS Steer */
	options.c_iflag &= ~(IGNPAR | INPCK | ISTRIP);  /* See termio.h         */
	options.c_iflag &= ~(INLCR  | IGNCR | ICRNL );  /* See termio.h         */
	options.c_iflag &= ~(IUCLC  );                  /* See termio.h         */

	/* Confige Output of Device   */
	options.c_iflag &= ~(OPOST  | OLCUC | ONLCR );  /* See termio.h         */
	options.c_iflag &= ~(OCRNL  | ONOCR | ONLRET);  /* See termio.h         */
	options.c_iflag &= ~(OFILL  | OFDEL );          /* See termio.h         */
	options.c_iflag &= ~(NL0 | CR0 | TAB0 | BS0 );  /* See termio.h         */
	options.c_iflag &= ~(VT0    | FF0   );          /* See termio.h         */

	/* Various Terminal Functions */
	options.c_lflag &= ~(ISIG   | ICANON| XCASE );  /* See termio.h         */
	options.c_lflag &= ~(NOFLSH | ECHO  | ECHOE );  /* See termio.h         */
	options.c_lflag &= ~(ECHOK  | ECHONL);          /* See termio.h         */

	/* Clear Special Control Char */
	memset(&options.c_cc, 0, sizeof(options.c_cc));

	options.c_cc[VMIN] = sizeof(struct serial_recv_head); // read block
	options.c_cc[VTIME] = 0;

	fcntl(fd, F_SETFL, 0); // write block

	/* Flush all Data */
	tcflush(fd, TCIOFLUSH);

	/* Write new Config immediately */
	tcsetattr(fd, TCSANOW, &options);

	return fd;
}

static void usage(void)
{
	printf("send - Utility for the push file interface\n\n");
	printf("Usage:\n"
			"\tsend -d <command> -f <command> -t <command>\n\n");

	printf("Options:\n"
			"\t-d <device>        Select the device\n"
			"\t-f <file>          Select the transport file\n"
			"\t-t <target>        Select the target file\n"
			"\t-h, --help         Display help\n"
			"\n");
}

static struct option main_options[] = {
	{ "device", 1, 0, 'd' },
	{ "file", 1, 0, 'f' },
	{ "target", 1, 0, 't' },
	{ 0, 0, 0, 0 }
};


int main(int argc, char *argv[])
{
	int opt;

	int file_fd;
	int uart_fd;
	char *device = NULL;
	char *file = NULL;
	char *target = NULL;

	int nread;
	int nwrite;

	off_t file_size = 0;
	off_t resize = 0;

	struct serial_send_head send_head;
	struct serial_send_data send_data;
	struct serial_recv_head recv_head;

	struct progress_bar bar;

	// print_struct_size();

	while ((opt=getopt_long(argc, argv, "d:f:t:h", main_options, NULL)) != EOF) {
		switch (opt) {
		case 'd':
			device = strdup(optarg);
			break;
		case 'f':
			file = strdup(optarg);
			break;
		case 't':
			target = strdup(optarg);
			break;
		case 'h':
		default:
			usage();
			exit(0);
		}
	}

	if (!device || !file || !target)
	{
		usage();
		exit(0);
	}

	uart_fd = open_uart(device);

	file_fd = open(file, O_RDONLY, 0755);
	if (file_fd < 0)
	{
		perror("open");
		close(uart_fd);
		exit(1);
	}

	file_size = lseek(file_fd, 0, SEEK_END);
	lseek(file_fd, 0, SEEK_SET);

	/* write head to serial */
	bzero(&send_head, sizeof(struct serial_send_head));
	strncpy(send_head.buff_flag, "jolinstart", strlen("jolinstart"));
	strncpy(send_head.file_path, target, strlen(target));
	send_head.file_size = file_size;
	nwrite = write(uart_fd, &send_head, sizeof(struct serial_send_head));
	if (nwrite != sizeof(struct serial_send_head))
	{
		printf("%s %d write err\n", __func__, __LINE__);
		exit(1);
	}

	// print_send_head(&send_head);

	bzero(&send_data, sizeof(struct serial_send_data));
	send_data.send_num = 0;

	progress_bar_init(&bar, file_size);

	while(resize < file_size)
	{
		// printf("%s %d while\n", __func__, __LINE__);

		memset(send_data.data, 0, sizeof(send_data.data));
		bzero(&recv_head, sizeof(struct serial_recv_head));

		nread = read(file_fd, send_data.data, sizeof(send_data.data));
		send_data.length = nread;
		send_data.crc = crc16(0, send_data.data, send_data.length);

		nwrite = write(uart_fd, &send_data, sizeof(struct serial_send_data));
		if (nwrite != sizeof(struct serial_send_data))
		{
			printf("%s %d write err\n", __func__, __LINE__);
			exit(1);
		}

		read(uart_fd, &recv_head, sizeof(struct serial_recv_head));
		if (!strncmp(tran_res_ng, recv_head.buff_flag, strlen(tran_res_ng))){
			printf("file transfers failed\n");
			exit(1);
		}

		if (recv_head.recv_num != send_data.send_num)
		{
			printf("err for recv recv_num:%ld send_num:%ld err\n", recv_head.recv_num, send_data.send_num);
			printf("file transfers failed\n");
			exit(1);
		}

		send_data.send_num++;
		resize = resize + nread;
		progress_bar_add(&bar, nread);
		// usleep(1);
	}

	progress_bar_finish(&bar);

	bzero(&recv_head, sizeof(struct serial_recv_head));
	read(uart_fd, &recv_head, sizeof(struct serial_recv_head));
	if (!strncmp(tran_res_ok, recv_head.buff_flag, strlen(tran_res_ok)))
	{
		printf("file transfers OK\n");
	} else {
		printf("file transfers failed\n");
	}

	close(uart_fd);
	close(file_fd);

	return 0;
}
