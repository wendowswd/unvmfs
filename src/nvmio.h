#ifndef _UNVMFS_NVMIO_H
#define _UNVMFS_NVMIO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "inode.h"
#include "types.h"
#include "allocator.h"

struct log_page_tail {

};

struct file_log_entry {
    u64 data;
    u64 file_size;

    u64 start_write_off;
    u64 end_write_off;

    u32 num_pages;
    u32 invalid_pages;
    u64 bitmap;
    //u64 in_page;
};


void init_unvmfs(void);
struct file_log_entry *get_log_entry(u64 *log_tail);
void init_log_entry(struct file_log_entry *entry, list_node_t *pages, u64 file_size, 
                            u64 alloc_num, u64 start_wrtie_off, u64 end_write_off);
void update_inode_tail(struct unvmfs_inode *inode, u64 log_tail);

ssize_t nvmio_write(struct unvmfs_inode *inode, const void *buf, size_t cnt);
ssize_t nvmio_read(struct unvmfs_inode *inode, void *buf, size_t cnt);



#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _UNVMFS_NVMIO_H */

