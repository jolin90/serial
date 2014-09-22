#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t u64; // stdint.h

#define HALF_LENGTH		30
#define FREE_CHAR       '-'
#define FULL_CHAR       '+'

struct progress_bar_body
{
	char head;
	char content1[HALF_LENGTH];
	char percent[6];
	char content2[HALF_LENGTH];
	char tail;
};

struct progress_bar
{
	u64 total;
	u64 current;
	int percent;
	int length;
	struct progress_bar_body body;
};

void progress_bar_init(struct progress_bar *bar, u64 total);
void progress_bar_add(struct progress_bar *bar, u64 val);
void progress_bar_finish(struct progress_bar *bar);
