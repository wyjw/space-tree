#include "stdio.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <linux/treenvme_ioctl.h>

// fake struct (not real, don't use)
struct blck_tbl {
	int a;
};

int main(void) {
	int fd;
	
	struct treenvme_block_table tbl;	
	tbl.length_of_array = 4;
	tbl.smallest = 10;
	tbl.next_head = 46;

	//for (int i = 0; i)
	fd = open("/dev/treenvme0", O_RDWR);
	ioctl(fd, TREENVME_IOCTL_REGISTER_BLOCKTABLE, &tbl);	
}
