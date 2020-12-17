#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include <unvmfs.h>

#define handle_error(msg) \
	do { \
		perror(msg); \
		exit(EXIT_FAILURE); \
	} while (0)

#define FILE_PATH "/home/wenduo/unvmfs/testfile"
#define FILE_SIZE (1UL << 26)
#define BUF_SIZE (1UL << 10)

int main(void) {
	char write_buf[BUF_SIZE];
    char read_buf[BUF_SIZE];
	off_t i, j;
	int fd, s;

	fd = open(FILE_PATH, O_CREAT | O_RDWR);
	if (fd == -1) {
		handle_error("open");
	}

    memset(write_buf, 'a', BUF_SIZE);
	for (i = 0; i < FILE_SIZE; i += BUF_SIZE) {
		write(fd, write_buf, BUF_SIZE);
	}

    lseek(fd, 0, SEEK_SET);

    for (i = 0; i < FILE_SIZE; i += BUF_SIZE) {
        memset(read_buf, 0, BUF_SIZE);
		read(fd, read_buf, BUF_SIZE);
        for (j = 0; j < BUF_SIZE; ++j) {
            if (write_buf[j] != read_buf[j]) {
                printf("file op err \n");
                break;
            }
        }
        if (j < BUF_SIZE) {
            break;
        }
	}
    if (i == FILE_SIZE) {
        printf("file op success \n");
    }

	s = close(fd);
	if (s != 0) {
		handle_error("close");
	}
	return 0;
}
