#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>

#define handle_error(msg) \
	do { \
		perror(msg); \
		exit(EXIT_FAILURE); \
	} while (0)

#define FILE_PATH "/mnt/pmem0_wenduo/testfile"
#define FILE_SIZE (1UL << 30)
#define BUF_SIZE (1UL << 12)
#define NSECS_PER_SEC 100000000

int main(void) {
    char write_buf[BUF_SIZE];
    char read_buf[BUF_SIZE];
    off_t i, j;
    int fd, s;
    char *addr = NULL;
    
    struct timespec start_time;
    struct timespec end_time;
    unsigned long long cnt;

    fd = open(FILE_PATH, O_CREAT | O_RDWR);
    if (fd == -1) {
        handle_error("open");
    }
    s = posix_fallocate(fd, 0, FILE_SIZE);
    if (__glibc_unlikely(s != 0)) {
      handle_error("fallocate");
    }

    memset(write_buf, 'a', BUF_SIZE);
    clock_gettime(CLOCK_REALTIME, &start_time);
    for (i = 0; i < FILE_SIZE; i += BUF_SIZE) {
        write(fd, write_buf, BUF_SIZE);
    }
    clock_gettime(CLOCK_REALTIME, &end_time);
    cnt = (end_time.tv_sec - start_time.tv_sec) * NSECS_PER_SEC + (end_time.tv_nsec - start_time.tv_nsec);
    printf("write latency:  %llu  ns\n", cnt);

    lseek(fd, 0, SEEK_SET);

    clock_gettime(CLOCK_REALTIME, &start_time);
    for (i = 0; i < FILE_SIZE; i += BUF_SIZE) {
        read(fd, read_buf, BUF_SIZE);
    }
    clock_gettime(CLOCK_REALTIME, &end_time);
    cnt = (end_time.tv_sec - start_time.tv_sec) * NSECS_PER_SEC + (end_time.tv_nsec - start_time.tv_nsec);
    printf("read latency:  %llu  ns\n", cnt);

    addr = (char *)mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
    if (__glibc_unlikely(addr == NULL)) {
        handle_error("mmap");
     }

    clock_gettime(CLOCK_REALTIME, &start_time);
    for (i = 0; i < FILE_SIZE; i += BUF_SIZE) {
        memcpy(addr + i, write_buf, BUF_SIZE);
    }
    clock_gettime(CLOCK_REALTIME, &end_time);
    cnt = (end_time.tv_sec - start_time.tv_sec) * NSECS_PER_SEC + (end_time.tv_nsec - start_time.tv_nsec);
    printf("mmap write latency:  %llu  ns\n", cnt);

    clock_gettime(CLOCK_REALTIME, &start_time);
    for (i = 0; i < FILE_SIZE; i += BUF_SIZE) {
        memcpy(read_buf, addr + i, BUF_SIZE);
    }
    clock_gettime(CLOCK_REALTIME, &end_time);
    cnt = (end_time.tv_sec - start_time.tv_sec) * NSECS_PER_SEC + (end_time.tv_nsec - start_time.tv_nsec);
    printf("mmap read latency:  %llu  ns\n", cnt);
    
    s = close(fd);
    if (s != 0) {
        handle_error("close");
    }
    return 0;
}

