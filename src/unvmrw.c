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
    inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
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
    inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
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

    inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
    if (inode_offset == OFFSET_NULL) {
        UNVMFS_LOG("read, no such inode");
        return INODE_FAILED;
    }
    inode = nvm_off2addr(inode_offset);

    pthread_rwlock_wrlock(&sb->rwlockp);
    list_move(&inode->l_node, &sb->s_list);
    pthread_rwlock_unlock(&sb->rwlockp);

    ++g_read_times_cnt;

    return nvmio_read(inode, buf, cnt);
}

ssize_t unvmwrite(int fd, const void *buf, size_t cnt)
{
    struct unvmfs_super_block *sb = get_superblock();
    u64 inode_offset;
    struct unvmfs_inode *inode = NULL;

    inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
    if (inode_offset == OFFSET_NULL) {
        UNVMFS_LOG("write, no such inode");
        return INODE_FAILED;
    }
    inode = nvm_off2addr(inode_offset);
    
    pthread_rwlock_wrlock(&sb->rwlockp);
    list_move(&inode->l_node, &sb->s_list);
    pthread_rwlock_unlock(&sb->rwlockp);

    ++g_write_times_cnt;

    return nvmio_write(inode, buf, cnt);
}

off_t unvmlseek(int fd, off_t offset, int whence)
{
    struct unvmfs_super_block *sb = get_superblock();
    u64 inode_offset;
    struct unvmfs_inode *inode = NULL;
    off_t off;

    inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
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

