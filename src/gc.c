#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include "internal.h"
#include "super.h"
#include "inode.h"
#include "radixtree.h"
#include "nvmio.h"
#include "allocator.h"
#include "debug.h"

u64 g_write_times_cnt = 0;
u64 g_read_times_cnt = 0;

static inline bool entry_invalid_page(struct file_log_entry *entry, int idx)
{
    return entry->bitmap & (1ULL << idx);
}

void gc_log_block(struct unvmfs_inode *inode, list_node_t *page_node)
{
    u32 i, j, k;
    u32 num_entries;
    u32 entry_size = sizeof(struct file_log_entry);
    
    char *page = NULL;
    struct file_log_entry *log_entry = NULL;
    struct file_log_entry *new_entry = NULL;
    u64 data;
    u64 log_tail;

    list_node_t *prev_node = NULL;
    list_node_t *page_head = NULL;
    list_node_t *page_tail = NULL;
    list_node_t *free_head = NULL;
    u32 page_nums;
    u32 free_nums = 0;
    u64 start_write_off;
    u64 end_write_off;
    
    page = nvm_off2addr(page_node->offset);
    num_entries = page_node->obj_cnt;

    for (i = 0; i < num_entries; ++i) {
        log_entry = (struct file_log_entry *)(page + (i * entry_size));
        if (log_entry->num_pages == log_entry->invalid_pages) {

        }
        
        data = log_entry->data;
        j = 0;
        while (j < log_entry->num_pages) {
            // add valid pages to new log entry
            if (!entry_invalid_page(log_entry, j)) {
                page_head = get_nvm_page_node_addr(data);
                page_tail = page_head;
                page_nums = 0;
                k = j;
                while (j < log_entry->num_pages && !entry_invalid_page(log_entry, j)) {
                    prev_node = page_tail;
                    if (j < log_entry->num_pages - 1) {
                        page_tail = get_nvm_page_node_addr(page_tail->next_offset);
                    }
                    
                    ++j;
                    ++page_nums;
                }
                data = prev_node->next_offset;
                prev_node->next_offset = OFFSET_NULL;
                
                new_entry = get_log_entry(&log_tail);
                start_write_off = log_entry->start_write_off + k * PAGE_SIZE;
                end_write_off = (j == log_entry->num_pages ? log_entry->end_write_off : 
                                                    log_entry->start_write_off + j * PAGE_SIZE);
                
                init_log_entry(new_entry, page_head, inode->i_size, page_nums, start_write_off, end_write_off);

            }
            
            // free invalid pages
            if (j < log_entry->num_pages && entry_invalid_page(log_entry, j)) {
                page_head = get_nvm_page_node_addr(data);
                page_tail = page_head;
                while (j < log_entry->num_pages && entry_invalid_page(log_entry, j)) {
                    prev_node = page_tail;
                    if (j < log_entry->num_pages - 1) {
                        page_tail = get_nvm_page_node_addr(page_tail->next_offset);
                    }
                    
                    ++j;
                    ++free_nums;
                }
                data = prev_node->next_offset;
                prev_node->next_offset = OFFSET_NULL;

                if (free_head == NULL) {
                    free_head = page_head;
                } else {
                    prev_node->next_offset = free_head->next_offset;
                    free_head->next_offset = page_head->offset;
                }
            }

        }
    }

    free_pages(free_head, free_nums);

    update_inode_tail(inode, log_tail);

}

void start_gc_task(void)
{
    struct unvmfs_super_block *sb = NULL;
    struct unvmfs_inode *inode = NULL;
    struct list_head *entry = NULL;
    list_node_t *page_node = NULL;
    list_node_t *prev_node = NULL;
    u64 page_head;
    u64 page_tail;
    u64 prev_page = 0;
    u64 free_space_off;
    
    sb = get_superblock();

    pthread_rwlock_wrlock(&sb->rwlockp);
    entry = sb->s_list.prev;
    list_del(entry);
    list_add(entry, &sb->s_list);
    pthread_rwlock_unlock(&sb->rwlockp);

    inode = list_entry(entry, struct unvmfs_inode, l_node);
    
    pthread_rwlock_wrlock(&inode->rwlockp);
    page_head = inode->log_head;
    free_space_off = nvm_addr2off(get_free_space_base_addr());
    page_tail = free_space_off + (inode->log_tail - free_space_off) 
                                                / UNVMFS_PAGE_SIZE * UNVMFS_PAGE_SIZE;
    while (page_head != page_tail) {
        page_node = get_nvm_page_node_addr(page_head);
        if ((page_node->invalid_cnt > page_node->obj_cnt * INVALID_IN_VALID) || 
                                    (page_node->invalid_pages > page_node->pages_cnt * INVALID_IN_VALID)) {
            if (page_head == inode->log_head) {
                inode->log_head = page_node->next_offset;
            } else {
                prev_node = get_nvm_page_node_addr(prev_page);
                prev_node->next_offset = page_node->next_offset;
            }
            gc_log_block(inode, page_node);
            
        }
        prev_page = page_head;
        page_head = page_node->next_offset;
    }
    
    pthread_rwlock_unlock(&inode->rwlockp);
}

void *gc_task_thread(void *args)
{
    pagelist_t *g_list = NULL;
    u64 access_times;
    u64 read_times;
    u64 write_times;
    double w_rate;
    double r_rate;

    g_list = get_free_space_base_addr();

    while (1) {
        access_times = g_write_times_cnt + g_read_times_cnt;
        write_times = g_write_times_cnt;
        read_times = g_read_times_cnt;
        if (access_times == 0) {
            sleep(5);
            continue;
        }
        w_rate = (double)write_times / access_times;
        r_rate = (double)read_times / access_times;
        
        if ((w_rate >= DOMINATION_RATE && g_list->count < WRITE_DOMINATION_THS) || 
                (r_rate >= DOMINATION_RATE && g_list->count < READ_DOMINATION_THS)) {
            g_write_times_cnt = 0;
            g_read_times_cnt = 0;
            start_gc_task();
        }
        sleep(2);
    }

}

void kernel_gc_thread_init(void)
{
    int res;
    pthread_t thread_id;

    res = pthread_create(&thread_id, NULL, gc_task_thread, NULL);
    if (__glibc_unlikely(res != 0)) {
        handle_error("kernel_gc_thread_init failed");
    }
}

