#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "internal.h"
#include "unvmrw.h"
#include "nvmio.h"
#include "hashmap.h"
#include "types.h"
#include "inode.h"
#include "allocator.h"
#include "radixtree.h"
#include "super.h"

#ifndef NULL
#define NULL 0
#endif

extern bool process_initialized;
extern u64 g_write_times_cnt;
extern u64 g_read_times_cnt;

int unvmcreat(const char *filename, mode_t mode) {
    s32 fd;
    struct unvmfs_super_block *sb = NULL;
    u64 inode_offset;
    struct unvmfs_inode *inode = NULL;
    
    if (__glibc_unlikely(!process_initialized)) {
        init_unvmfs();
    }

    sb = get_superblock();
    fd = hashmap_hash_s32(filename);
    pthread_rwlock_rdlock(&sb->rwlockp);
    inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
    pthread_rwlock_unlock(&sb->rwlockp);
    if (inode_offset != OFFSET_NULL) {
        UNVMFS_LOG("creat, inode exist");
        return EEXIST;
    }

    inode_offset = alloc_unvmfs_inode();
    if (inode_offset == OFFSET_NULL) {
        UNVMFS_LOG("creat, alloc inode failed");
        return INODE_FAILED;
    }

    inode = nvm_off2addr(inode_offset);
    inode->fd = fd;

    pthread_rwlock_wrlock(&sb->rwlockp);
    set_radixtree_node(&sb->hash_root, inode_offset, fd, RADIXTREE_INODE);
    list_add(&inode->l_node, &sb->s_list);
    pthread_rwlock_unlock(&sb->rwlockp);
    return fd;
}

int unvmopen(const char *path, int flags, ...) {
    s32 fd;
    struct unvmfs_super_block *sb = NULL;
    u64 inode_offset;
    struct unvmfs_inode *inode = NULL;

    if (__glibc_unlikely(!process_initialized)) {
        init_unvmfs();
    }

    sb = get_superblock();
    fd = hashmap_hash_s32(path);
    pthread_rwlock_rdlock(&sb->rwlockp);
    inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
    pthread_rwlock_unlock(&sb->rwlockp);
    if (inode_offset == OFFSET_NULL) {
        if (!(flags & O_CREAT)) {
            UNVMFS_LOG("open, inode not exist and !O_CREAT");
            return EEXIST;
        }
        inode_offset = alloc_unvmfs_inode();
        if (inode_offset == OFFSET_NULL) {
            UNVMFS_LOG("open, O_CREAT, alloc inode failed");
            return INODE_FAILED;
        }
    }

    inode = nvm_off2addr(inode_offset);
    if (flags & O_CREAT) {
        inode->fd = fd;
        pthread_rwlock_wrlock(&sb->rwlockp);
        set_radixtree_node(&sb->hash_root, inode_offset, fd, RADIXTREE_INODE);
        list_add(&inode->l_node, &sb->s_list);
        pthread_rwlock_unlock(&sb->rwlockp);
    }

    return fd;
}

int unvmclose(int fd)
{
    // todo
    return 0;
}

ssize_t unvmread(int fd, void *buf, size_t cnt)
{
    struct unvmfs_super_block *sb = get_superblock();
    u64 inode_offset;
    struct unvmfs_inode *inode = NULL;
    ssize_t ret;

    pthread_rwlock_rdlock(&sb->rwlockp);
    inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
    pthread_rwlock_unlock(&sb->rwlockp);
    if (inode_offset == OFFSET_NULL) {
        UNVMFS_LOG("read, no such inode");
        return INODE_FAILED;
    }
    inode = nvm_off2addr(inode_offset);

    pthread_rwlock_wrlock(&sb->rwlockp);
    list_move(&inode->l_node, &sb->s_list);
    pthread_rwlock_unlock(&sb->rwlockp);

    ++g_read_times_cnt;

    pthread_rwlock_rdlock(&inode->rwlockp);
    ret = nvmio_read(inode, buf, cnt, inode->file_off);
    update_inode_offset(inode, cnt);
    pthread_rwlock_unlock(&inode->rwlockp);
    
    return ret;
}

ssize_t unvmwrite(int fd, const void *buf, size_t cnt)
{
    struct unvmfs_super_block *sb = get_superblock();
    u64 inode_offset;
    struct unvmfs_inode *inode = NULL;
    ssize_t ret;

    pthread_rwlock_rdlock(&sb->rwlockp);
    inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
    pthread_rwlock_unlock(&sb->rwlockp);
    if (inode_offset == OFFSET_NULL) {
        UNVMFS_LOG("write, no such inode");
        return INODE_FAILED;
    }
    inode = nvm_off2addr(inode_offset);
    
    pthread_rwlock_wrlock(&sb->rwlockp);
    list_move(&inode->l_node, &sb->s_list);
    pthread_rwlock_unlock(&sb->rwlockp);

    ++g_write_times_cnt;

    pthread_rwlock_wrlock(&inode->rwlockp);
    ret = nvmio_write(inode, buf, cnt, inode->file_off);
    update_inode_info(inode, cnt);
    pthread_rwlock_unlock(&inode->rwlockp);

    return ret;
}

off_t unvmlseek(int fd, off_t offset, int whence)
{
    struct unvmfs_super_block *sb = get_superblock();
    u64 inode_offset;
    struct unvmfs_inode *inode = NULL;
    off_t off;

    pthread_rwlock_rdlock(&sb->rwlockp);
    inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
    pthread_rwlock_unlock(&sb->rwlockp);
    if (inode_offset == OFFSET_NULL) {
        UNVMFS_LOG("lseek, no such inode");
        return INODE_FAILED;
    }
    inode = nvm_off2addr(inode_offset);

    pthread_rwlock_rdlock(&inode->rwlockp);
    
    switch (whence) {
        case SEEK_SET:
            inode->file_off = offset;
            break;
        case SEEK_CUR:
            inode->file_off += offset;
            break;
        case SEEK_END:
            inode->file_off = inode->i_size;
            inode->file_off += offset;
            break;
        default:
            pthread_rwlock_unlock(&inode->rwlockp);
            // SEEK_DATA, SEEK_HOLE if needed
            return EINVAL;
    }

    off = inode->file_off;
    pthread_rwlock_unlock(&inode->rwlockp);

    return off;
}

int unvmftruncate(int fd, off_t length)
{
    // todo
    return 0;
}

int unvmfsync(int fd)
{
    // todo
    return 0;
}

ssize_t unvmpread(int fd, void *buf, size_t cnt, off_t offset)
{
    struct unvmfs_super_block *sb = get_superblock();
    u64 inode_offset;
    struct unvmfs_inode *inode = NULL;
    ssize_t ret;

    pthread_rwlock_rdlock(&sb->rwlockp);
    inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
    pthread_rwlock_unlock(&sb->rwlockp);
    if (inode_offset == OFFSET_NULL) {
        UNVMFS_LOG("read, no such inode");
        return INODE_FAILED;
    }
    inode = nvm_off2addr(inode_offset);

    pthread_rwlock_wrlock(&sb->rwlockp);
    list_move(&inode->l_node, &sb->s_list);
    pthread_rwlock_unlock(&sb->rwlockp);

    ++g_read_times_cnt;

    pthread_rwlock_rdlock(&inode->rwlockp);
    ret = nvmio_read(inode, buf, cnt, offset);
    //update_inode_offset(inode, cnt);
    pthread_rwlock_unlock(&inode->rwlockp);
    
    return ret;

}

ssize_t unvmpread64(int fd, void *buf, size_t cnt, off_t offset)
{
    return unvmpread(fd, buf, cnt, offset);
}

ssize_t unvmpwrite(int fd, const void *buf, size_t cnt, off_t offset)
{
    struct unvmfs_super_block *sb = get_superblock();
    u64 inode_offset;
    struct unvmfs_inode *inode = NULL;
    ssize_t ret;

    pthread_rwlock_rdlock(&sb->rwlockp);
    inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
    pthread_rwlock_unlock(&sb->rwlockp);
    if (inode_offset == OFFSET_NULL) {
        UNVMFS_LOG("write, no such inode");
        return INODE_FAILED;
    }
    inode = nvm_off2addr(inode_offset);
    
    pthread_rwlock_wrlock(&sb->rwlockp);
    list_move(&inode->l_node, &sb->s_list);
    pthread_rwlock_unlock(&sb->rwlockp);

    ++g_write_times_cnt;

    pthread_rwlock_wrlock(&inode->rwlockp);
    ret = nvmio_write(inode, buf, cnt, offset);
    update_inode_size(inode, cnt);
    pthread_rwlock_unlock(&inode->rwlockp);

    return ret;

}

ssize_t unvmpwrite64(int fd, const void *buf, size_t cnt, off_t offset)
{
    return unvmpwrite(fd, buf, cnt, offset);
}

int unvmrename(const char *oldpath, const char *newpath)
{
    //todo
    return 0;
}

int unvmlink(const char *existing, const char *new)
{
    //todo
    return 0;
}

int unvmsymlink(const char *existing, const char *new)
{
    //todo
    return 0;
}

int unvmunlink(char *pathname)
{
    //todo
    return 0;
}

ssize_t unvmreadlink(const char *path, char *buf, size_t buf_size)
{
    //todo
    return 0;
}

int unvmmkdir(char *path, int perm)
{
    return (mkdir(path, perm));
}

int unvmrmdir(char *path)
{
    return (rmdir(path));
}

DIR *unvmopendir(char *path)
{
    return (opendir(path));
}

struct dirent *unvmreaddir(DIR *dirp)
{
    return (readdir(dirp));

}

int unvmclosedir(DIR *dirp)
{
    return (closedir(dirp));
}

int unvmstat(char *path, struct stat64 *statbufp)
{
    //todo
    return 0;
}

int unvmfstat(int fd, struct stat64 *statbufp)
{
    //todo
    return 0;
}

int unvmaccess(const char *path, int amode)
{
    //todo
    return 0;
}

