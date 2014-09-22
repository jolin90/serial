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
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <errno.h>
#include <getopt.h>

#include "tmfile.h"
#include "crc16.h"

int open_uart( char *pcDriverName )
{
	int fd = -1;  struct termios com_options;

	fd = open(pcDriverName ,O_RDWR | O_NONBLOCK | O_NOCTTY | O_SYNC);
	if (fd < 0)
	{
		perror("open uart");
		exit(1);
	}

	bzero(&com_options,sizeof(com_options));

	/* Get actual Setting of Device */
	tcgetattr(fd,&com_options);

	/* Make Raw Interface UART Interface */
	cfmakeraw(&com_options);

	/* Configure Speed */
	cfsetispeed(&com_options, B115200);              /* Input:  X Baud       */
	cfsetospeed(&com_options, B115200);              /* Output: X Baud       */

	/* Configure Control Options  */
	com_options.c_cflag |= (CLOCAL | CREAD);            /* Enable Receiver and set local mode
														   = Direct Connection  */

	com_options.c_cflag |= PARENB;                      /* Even Parity          */

	com_options.c_cflag &= ~CSTOPB;                     /* Only one Stop Bit    */
	com_options.c_cflag &= ~CSIZE ;                     /* Char Size Mask       */
	com_options.c_cflag |=  CS8;                        /* 8 Databits           */

	/* Configure Hardware Flow    */
	com_options.c_cflag &= ~CRTSCTS;                    /* No RTS and CTS Steer */

	/* Configure Software Flow    */
	com_options.c_iflag &= ~(IGNBRK | BRKINT| PARMRK);  /* See termio.h         */
	com_options.c_iflag &= ~(IXON   | IXOFF | IXANY );  /* No RTS and CTS Steer */
	com_options.c_iflag &= ~(IGNPAR | INPCK | ISTRIP);  /* See termio.h         */
	com_options.c_iflag &= ~(INLCR  | IGNCR | ICRNL );  /* See termio.h         */
	com_options.c_iflag &= ~(IUCLC  );                  /* See termio.h         */

	/* Confige Output of Device   */
	com_options.c_iflag &= ~(OPOST  | OLCUC | ONLCR );  /* See termio.h         */
	com_options.c_iflag &= ~(OCRNL  | ONOCR | ONLRET);  /* See termio.h         */
	com_options.c_iflag &= ~(OFILL  | OFDEL );          /* See termio.h         */
	com_options.c_iflag &= ~(NL0 | CR0 | TAB0 | BS0 );  /* See termio.h         */
	com_options.c_iflag &= ~(VT0    | FF0   );          /* See termio.h         */

	/* Various Terminal Functions */
	com_options.c_lflag &= ~(ISIG   | ICANON| XCASE );  /* See termio.h         */
	com_options.c_lflag &= ~(NOFLSH | ECHO  | ECHOE );  /* See termio.h         */
	com_options.c_lflag &= ~(ECHOK  | ECHONL);          /* See termio.h         */

	/* Clear Special Control Char */
	memset(&com_options.c_cc,0,sizeof(com_options.c_cc));

	/* Setup Timing of Interface: No Timing, we use poll() */
	// com_options.c_cc[VTIME] = 0  ;                      /* Get all Data         */
	// com_options.c_cc[VMIN]  = 0  ;                      /* Get all Data         */

	/* Flush all Data */
	tcflush(fd,TCIOFLUSH);                              /* IO Queue Flush       */

	/* Write new Config immediately */
	tcsetattr(fd,TCSANOW,&com_options);

	return fd;
}

static int read_msg(unsigned char* buf, int len, int* readbytes, int fd)
{
	struct pollfd pfd;
	int ret = 0;
	int err = 0;
	int left = len;
	int idx = 0;
	int sum = 0;

	pfd.fd      = fd;
	pfd.events  = POLLIN;

	while((left > 0) && !err)
	{
		err = poll(&pfd, 1, -1);

		if (err == -1) {
			printf("Error poll() !\n");
			fflush(stdout);
		} else {
			ret = read(fd, &buf[idx], left);
			if (ret < 0) {
				printf("Error read() !\n");
				fflush(stdout);
			}
			else {
				left -= ret;
				idx  += ret;
				sum  += ret;
				ret = 0;
				err = 0;
			}
		}
	}

	if(readbytes) *readbytes = sum;

	return err;
}

int write_file(int file_fd, int uart_fd, off_t size)
{
	struct serial_send_data send_data;
	struct serial_recv_head recv_head;

	off_t resize = size;
	int sum;

	while(1)
	{

		bzero(&send_data, sizeof(struct serial_send_data));
		bzero(&recv_head, sizeof(struct serial_recv_head));

		read_msg((unsigned char *)&send_data, sizeof(struct serial_send_data), &sum, uart_fd);
		if (send_data.length > 0)
		{
			if (send_data.crc != crc16(0, send_data.data, send_data.length))
			{
				strncpy(recv_head.buff_flag, tran_res_ng, strlen(tran_res_ng));
				write(uart_fd, &recv_head, sizeof(struct serial_recv_head));
				return 1;
			}

			write(file_fd, send_data.data, send_data.length);
			recv_head.recv_num = send_data.send_num;
			write(uart_fd, &recv_head, sizeof(struct serial_recv_head));
		}

		resize = resize - send_data.length;
		if (resize == 0)
			break;
	}

	bzero(&recv_head, sizeof(struct serial_recv_head));
	strncpy(recv_head.buff_flag, tran_res_ok, strlen(tran_res_ok));

	write(uart_fd, &recv_head, sizeof(struct serial_recv_head));

	return 0;
}

static void usage(void)
{
	printf("recv - Utility for the get file interface\n\n");
	printf("Usage:\n"
			"\trecv -d <command>\n\n");

	printf("Options:\n"
			"\t-d <device>        Select the device\n"
			"\t-h, --help         Display help\n"
			"\n");
}

static struct option main_options[] = {
	{ "device", 1, 0, 'd' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char *argv[])
{
	int ret;
	int opt, sum;
	int file_fd;
	int uart_fd;
	char *device = NULL;

	struct serial_send_head send_head;
	struct serial_recv_head recv_head;

	// print_struct_size();

	while ((opt=getopt_long(argc, argv, "d:f:t:h", main_options, NULL)) != EOF) {
		switch (opt) {
		case 'd':
			device = strdup(optarg);
			break;
		case 'h':
		default:
			usage();
			exit(0);
		}
	}

	if (!device)
	{
		usage();
		exit(0);
	}

	uart_fd = open_uart(device);

	bzero(&send_head, sizeof(struct serial_send_head));

	while(1)
	{
		read_msg((unsigned char *)&send_head, sizeof(struct serial_send_head), &sum, uart_fd);
		// print_send_head(&send_head);

		if (!strncmp(send_head.buff_flag, "jolinstart", strlen("jolinstart")))
		{
			file_fd = open(send_head.file_path, O_CREAT | O_WRONLY | O_TRUNC, 0755);
			if (file_fd < 0)
			{
				perror("open");
				bzero(&recv_head, sizeof(struct serial_recv_head));
				strncpy(recv_head.buff_flag, tran_res_ng, strlen(tran_res_ng));
				write(uart_fd, &recv_head, sizeof(struct serial_recv_head));
			} else {
				ret = write_file(file_fd, uart_fd, send_head.file_size);
				close(file_fd);
				if (!ret)
					printf("file transfers OK\n");
			}
		}
	}

	close(uart_fd);

	printf("done\n");
	return 0;
}
