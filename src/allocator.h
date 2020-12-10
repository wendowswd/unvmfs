#ifndef _UNVMFS_ALLOCATOR_H
#define _UNVMFS_ALLOCATOR_H

#include "radixtree.h"
#include "inode.h"
#include "types.h"

typedef enum allocator_type_enum {
    ALLOCATOR_INODE,
    ALLOCATOR_RADIXTREE,
} allocator_type_t;

typedef struct list_node_struct {
    u64 next_offset;    // next page offset
    u64 offset;         // current page offset

    u32 obj_cnt;        // page sliced into several objs
    u32 invalid_cnt;    // invalid objs
    u32 pages_cnt;      // total pages data
    u32 invalid_pages;  // invalid pages data
    u64 log_entry;

} list_node_t;

typedef struct pagelist_struct {
    u64 head;
    u64 count;
    pthread_mutex_t mutex;
} pagelist_t;

typedef struct allocator_list_struct {
    u64 head;
    u64 count;
    allocator_type_t type;
    pthread_mutex_t mutex;
} allocator_list_t;



//void init_page_list(void *addr);
void init_allocator_list(allocator_list_t *alloc_list);
list_node_t *alloc_free_pages(u32 page_num);
void free_pages(list_node_t *node, u32 page_num);
void fill_radixtree_node_list(allocator_list_t *alloc_list);
u64 alloc_radixtree_node(void);
void free_radixtree_node(radixtree_node_t *node);
void fill_unvmfs_inode_list(allocator_list_t *alloc_list);
u64 alloc_unvmfs_inode(void);
void free_unvmfs_inode(struct unvmfs_inode *node);

#endif /* _UNVMFS_ALLOCATOR_H */

