#ifndef _UNVMFS_INTERNAL_H
#define _UNVMFS_INTERNAL_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

#include "allocator.h"
#include "hashmap.h"
#include "radixtree.h"
#include "inode.h"
#include "types.h"

extern void *nvm_base_addr;

//#define CPU_NUMS 64

#define OFFSET_NULL (0xffffffffffffffff)

#define PAGE_SHIFT (12)
#define PAGE_SIZE (1ULL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))


/* superbolck */
#define SUPER_BLOCK_START   0           // Superblock
#define	RESERVE_INODE_START 1           // Reserved inodes
#define	DIR_TABLE_START     4           // dir table
#define	INODE_TABLE_START   8           // inode table
#define	PAGE_LIST_START     12          // page list table
#define	JOURNAL_START       16          // journal pointer table
#define	HASHMAP_START       20          // hashmap table
#define	RADIXTREE_START     24          // radix table
#define FREE_SPACE_START    28          // free space

#define NVM_MMAP_SIZE (1ULL << 36)       /* 64GB */
#define UNVMFS_PAGE_SIZE (PAGE_SIZE + sizeof(list_node_t))
#define NVM_FREE_NUM_PAGES ((NVM_MMAP_SIZE - FREE_SPACE_START * PAGE_SIZE) / (UNVMFS_PAGE_SIZE))
#define SB_INIT_NUMBER (1234567890ULL)
/* superbolck */


/* page list */
#define FILL_LOCAL_PAGE_LIST_MIN 2
#define FILL_LOCAL_PAGE_LIST_MAX 512

/* page list */

/* radixtree */
#define RADIXTREE_LEVEL 4
#define RADIXTREE_IDX_MASK (PTRS_PER_NODE - 1)

#define RADIXTREE_NODE_PER_ALLOC 1
#define RADIXTREE_NODE_PER_PAGE (PAGE_SIZE / sizeof(radixtree_node_t))
/* radixtree */

/* inode */
#define INODE_FAILED -1

#define UNVMFS_INODE_PER_ALLOC 1
#define UNVMFS_INODE_PER_PAGE (PAGE_SIZE / sizeof(struct unvmfs_inode))
/* inode */

/* hashmap */
#define HASHMAP_BITS 32
#define HASHMAP_MASK ((1UL << HASHMAP_BITS) - 1)
//#define HASH_ELEM_PER_PAGE (PAGE_SIZE / sizeof(hashmap_element))
/* hashmap */

/* write log */
#define MAX_NUM_PAGE_PER_ENTRY 64
#define WRITE_LOG_PERALLOC 1
/* write log */

/* garbage collection */
#define DOMINATION_RATE 0.75
#define WRITE_RATE 0.7
#define READ_RATE 0.9
#define WRITE_DOMINATION_THS ((NVM_MMAP_SIZE >> PAGE_SHIFT) * (1 - WRITE_RATE))
#define READ_DOMINATION_THS  ((NVM_MMAP_SIZE >> PAGE_SHIFT) * (1 - READ_RATE))
#define INVALID_IN_VALID 0.5
/* garbage collection */

#define NSECS_PER_SEC 1000000000
static inline u64 get_curr_time(void)
{
    struct timespec curr;
    clock_gettime(CLOCK_REALTIME, &curr);

    return curr.tv_sec * NSECS_PER_SEC + curr.tv_nsec;
}

static inline int get_cpuid(void)
{
    // todo
    return -1;
    // return sched_getcpu();
}

static inline int get_cpu_nums(void)
{
    return sysconf(_SC_NPROCESSORS_CONF);
}

static inline void *get_superblock(void)
{
    return nvm_base_addr;
}

static inline void *get_page_list_base_addr(void)
{
    return (char *)nvm_base_addr + PAGE_LIST_START * PAGE_SIZE;
}

static inline void *get_inode_list_base_addr(void)
{
    return (char *)nvm_base_addr + INODE_TABLE_START * PAGE_SIZE;
}

static inline void *get_radixtree_list_base_addr(void)
{
    return (char *)nvm_base_addr + RADIXTREE_START * PAGE_SIZE;
}

static inline void *get_free_space_base_addr(void)
{
    return (char *)nvm_base_addr + FREE_SPACE_START * PAGE_SIZE;
}

static inline void *get_local_page_list_addr(u64 num)
{
    return (char *)nvm_base_addr + PAGE_LIST_START * PAGE_SIZE + sizeof(pagelist_t) * num;
}

static inline void *get_radixtree_table_addr(u64 num)
{
    return (char *)nvm_base_addr + RADIXTREE_START * PAGE_SIZE + sizeof(allocator_list_t) * num;
}

static inline void *get_unvmfs_inode_table_addr(u64 num)
{
    return (char *)nvm_base_addr + INODE_TABLE_START * PAGE_SIZE + sizeof(allocator_list_t) * num;
}

static inline void *get_nvm_page_node_addr(u64 offset)
{
    return (char *)nvm_base_addr + offset + PAGE_SIZE;
}

static inline void *get_nvm_node2page_addr(void *addr)
{
    return (char *)addr - PAGE_SIZE;
}

static inline void *get_nvm_page2node_addr(void *addr)
{
    return (char *)addr + PAGE_SIZE;
}

static inline u64 get_nvm_page_off2free_space(u64 offset)
{
    return offset - FREE_SPACE_START * PAGE_SIZE;
}

static inline u64 nvm_addr2off(void *addr)
{
    return (char *)addr - (char *)nvm_base_addr;
}

static inline void *nvm_off2addr(u64 offset)
{
    return (char *)nvm_base_addr + offset;
}

// extern function
//extern static inline int get_cpuid(void);

#endif /* _UNVMFS_INTERNAL_H */