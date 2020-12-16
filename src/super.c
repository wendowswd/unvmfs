#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>

#include "hashmap.h"
#include "super.h"
#include "internal.h"
#include "allocator.h"
#include "radixtree.h"
#include "types.h"

extern u32 g_cpu_nums;

void kernel_superblock_init(struct unvmfs_super_block *addr, char *unvmfs_path)
{
    UNVMFS_DEBUG("kernel_superblock_init start");

    addr->s_size = NVM_MMAP_SIZE;
    addr->s_blocksize = PAGE_SIZE;
    addr->num_blocks = NVM_MMAP_SIZE / addr->s_blocksize;
    addr->free_num_pages = NVM_FREE_NUM_PAGES;
    addr->cpu_nums = g_cpu_nums;

    INIT_LIST_HEAD(&addr->s_list);
    pthread_rwlock_init(&addr->rwlockp, NULL);
    memcpy(addr->s_volume_name, unvmfs_path, strlen(unvmfs_path));
    init_radixtree_root(&addr->hash_root, RADIXTREE_INODE);

    UNVMFS_DEBUG("kernel_superblock_init success");
}

void kernel_page_list_init(void)
{
    u64 i, j;
    u64 cpu_nums = g_cpu_nums;
    u64 free_num_pages = NVM_FREE_NUM_PAGES;
    char *page_list_base_addr = get_page_list_base_addr();
    char *free_space_base_addr = get_free_space_base_addr();
    u64 free_space_offset = nvm_addr2off(free_space_base_addr);
    
    pagelist_t *global_page_list = (pagelist_t *)page_list_base_addr;
    pagelist_t *local_page_list = NULL;

    list_node_t *node = NULL;
    list_node_t *prev = NULL;

    UNVMFS_DEBUG("kernel_page_list_init start");

    pthread_mutex_init(&global_page_list->mutex, NULL);
    global_page_list->count = free_num_pages;
    for (i = 0; i < free_num_pages; ++i) {
        node = (list_node_t *)(free_space_base_addr + (i * UNVMFS_PAGE_SIZE + PAGE_SIZE));
        node->offset = free_space_offset + i * UNVMFS_PAGE_SIZE;
        if (prev == NULL) {
            node->next_offset = OFFSET_NULL;
        } else {
            node->next_offset = prev->offset;
        }
        prev = node;
    }
    global_page_list->head = prev->offset;
    
    for (i = 1; i <= cpu_nums; ++i) {
        local_page_list = get_local_page_list_addr(i);
        node = get_nvm_page_node_addr(global_page_list->head);
        local_page_list->head = node->offset;
        for (j = 0; j < FILL_LOCAL_PAGE_LIST_MAX - 1; ++j) {
            node = get_nvm_page_node_addr(node->next_offset);
        }
        global_page_list->head = node->next_offset;
        node->next_offset = OFFSET_NULL;

        global_page_list->count -= FILL_LOCAL_PAGE_LIST_MAX;
        local_page_list->count = FILL_LOCAL_PAGE_LIST_MAX;
        pthread_mutex_init(&local_page_list->mutex, NULL);
        
    }

    UNVMFS_DEBUG("kernel_page_list_init success");
}

void kernel_inode_list_init(void)
{
    UNVMFS_DEBUG("kernel_inode_list_init start");

    int i;
    int cpu_nums = g_cpu_nums;
    allocator_list_t *alloc_list = NULL;

    for (i = 0; i < cpu_nums; ++i) {
        alloc_list = get_unvmfs_inode_table_addr(i);
        init_allocator_list(alloc_list);
        fill_unvmfs_inode_list(alloc_list);
    }

    UNVMFS_DEBUG("kernel_inode_list_init success");
}

void kernel_radixtree_list_init(void)
{
    UNVMFS_DEBUG("kernel_inode_list_init start");

    int i;
    int cpu_nums = g_cpu_nums;
    allocator_list_t *alloc_list = NULL;

    for (i = 0; i < cpu_nums; ++i) {
        alloc_list = get_radixtree_table_addr(i);
        init_allocator_list(alloc_list);
        fill_radixtree_node_list(alloc_list);
    }

    UNVMFS_DEBUG("kernel_radixtree_list_init success");
}