#ifndef _UNVMFS_H
#define _UNVMFS_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <dirent.h>
#include <sys/types.h>
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

// filebench
//extern int unvmfreemem(int fdfd, off64_t size);
extern int unvmpread(int fd, void *buf, size_t cnt, off_t offset);
#define pread(fd, buf, cnt, offset) unvmpread(fd, buf, cnt, offset)
extern int unvmpread64(int fd, void *buf, size_t cnt, off_t offset);
#define pread64(fd, buf, cnt, offset) unvmpread64(fd, buf, cnt, offset)
extern int unvmpwrite(int fd, const void *buf, size_t cnt, off_t offset);
#define pwrite(fd, buf, cnt, offset) unvmpwrite(fd, buf, cnt, offset)
extern int unvmpwrite64(int fd, const void *buf, size_t cnt, off_t offset);
#define pwrite64(fd, buf, cnt, offset) unvmpwrite(fd, buf, cnt, offset)
extern int unvmrename(const char *oldpath, const char *newpath);
#define rename(oldpath, newpath) unvmrename(oldpath, newpath)
extern int unvmlink(const char *existing, const char *new);
#define link(existing, new) unvmlink(existing, new)
extern int unvmsymlink(const char *existing, const char *new);
#define symlink(existing, new) unvmsymlink(existing, new)
extern int unvmunlink(char *pathname);
#define unlink(pathname) unvmunlink(pathname)
extern ssize_t unvmreadlink(const char *path, char *buf, size_t buf_size);
#define readlink(path, buf, buf_size) unvmreadlink(path, buf, buf_size)
extern int unvmmkdir(char *path, int perm);
#define mkdir(path, perm) unvmmkdir(path, perm)
extern int unvmrmdir(char *path);
#define rmdir(path) unvmrmdir(path)
extern DIR *unvmopendir(char *path);
#define unvmopendir(path) unvmopendir(path)
extern struct dirent *unvmreaddir(DIR *dirp);
#define readdir(dirp) unvmreaddir(dirp)
extern int unvmclosedir(DIR *dirp);
#define closedir(dirp) unvmclosedir(dirp)
extern int unvmstat(char *path, struct stat *statbufp);
#define stat(path, statbufp) unvmstat(path, statbufp)
extern int unvmfstat(int fd, struct stat *statbufp);
#define fstat(fd, statbufp) unvmfstat(fd, statbufp)
extern int unvmaccess(const char *path, int amode);
#define access(path, amode) unvmaccess(path, amode)

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _UNVMFS_H

