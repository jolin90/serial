#ifndef __TM_FILE_HEAD__
#define __TM_FILE_HEAD__

struct serial_send_head
{
	char buff_flag[20];
	char file_path[255];

	off_t file_size;
};

struct serial_send_data
{
	int length;
	off_t send_num;

	unsigned char data[255];
	unsigned short crc;
};

struct serial_recv_head
{
	off_t recv_num;
	char buff_flag[20];
};

static inline void print_struct_size(void)
{
	printf("%s serial_send_head:%d\n", __func__, sizeof(struct serial_send_head));
	printf("%s serial_send_data:%d\n", __func__, sizeof(struct serial_send_data));
	printf("%s serial_recv_head:%d\n", __func__, sizeof(struct serial_recv_head));
	printf("\n");
}

static inline void print_send_head(struct serial_send_head *head)
{
	printf("%s buff_flag:%s\n", __func__, head->buff_flag);
	printf("%s head->file_path:%s\n", __func__, head->file_path);
	printf("%s head->file_size:%ld\n", __func__, head->file_size);
	printf("\n");
}

static char *tran_res_ok = "success";
static char *tran_res_ng = "failed";

#endif
