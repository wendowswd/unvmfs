#ifndef _UNVMFS_H
#define _UNVMFS_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <sys/stat.h>
#include <sys/uio.h>
#include "../src/unvmrw.h"

extern void init_unvmfs(void);
#define creat(filename, mode) unvmcreat(filename, mode)
extern int unvmcreat(const char *filename, mode_t mode);
#define open(...) unvmopen(__VA_ARGS__)
extern int unvmopen(const char *Path, int flags, ...);
#define close(fd) unvmclose(fd)
extern int unvmclose(int fd);
#define read(fd, buf, cnt) unvmread(fd, buf, cnt)
extern ssize_t unvmread(int fd, void *buf, size_t cnt);
#define write(fd, buf, cnt) unvmwrite(fd, buf, cnt)
extern ssize_t unvmwrite(int fd, const void *buf, size_t cnt);
#define lseek(fd, offset, whence) unvmlseek(fd, offset, whence)
extern off_t unvmlseek(int fd, off_t offset, int whence);
#define truncate(fd, length) unvmftruncate(fd, length)
extern int unvmftruncate(int fd, off_t length);
#define fsync(fd) unvmfsync(fd)
extern int unvmfsync(int fd);



#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _UNVMFS_H

