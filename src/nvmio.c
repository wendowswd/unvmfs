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
#include <pthread.h>
#include <sched.h>

#include "nvmio.h"
#include "inode.h"
#include "internal.h"
#include "super.h"
#include "allocator.h"
#include "debug.h"

extern void kernel_gc_thread_init(void);

static char *unvmfs_path = NULL;
static char *unvmfs_file = "unvmfs";
void *nvm_base_addr = NULL;

bool process_initialized = false;

void init_env(void)
{
	int fd, s;
	char filename[256];
	unsigned long long *init_num = NULL;
	
	//unvmfs_path = getenv("UNVMFS_PATH");
	unvmfs_path = "/mnt";
	if (__glibc_unlikely(unvmfs_path == NULL)) {
	  handle_error("unvmfs_path is NULL.");
	}

	sprintf(filename, "%s/%s", unvmfs_path, unvmfs_file);
	fd = open(filename, O_CREAT | O_RDWR);
	if (fd < 0) {
		handle_error("open unvmfs_path failed.");
	}
	s = posix_fallocate(fd, 0, NVM_MMAP_SIZE);
    if (__glibc_unlikely(s != 0)) {
      handle_error("fallocate");
    }
	nvm_base_addr = mmap(NULL, NVM_MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
	if (__glibc_unlikely(nvm_base_addr == NULL)) {
		  handle_error("nvm_base_addr is NULL.");
	}

	init_num = nvm_base_addr;
	if (__sync_bool_compare_and_swap(init_num, 0ULL, SB_INIT_NUMBER)) {
        kernel_page_list_init();
        
        kernel_inode_list_init();
        kernel_radixtree_list_init();

        kernel_superblock_init(nvm_base_addr, unvmfs_path);

        kernel_gc_thread_init();
	}
	
}

void init_unvmfs(void)
{
    if (__sync_bool_compare_and_swap(&process_initialized, false, true)) {
        init_env();
    }
}

bool is_last_entry(u64 tail)
{
    u64 offset = get_nvm_page_off2free_space(tail);
    u64 entry_end = (offset % UNVMFS_PAGE_SIZE) + sizeof(struct file_log_entry);

    return entry_end > PAGE_SIZE;
}

list_node_t *entry2log_page(struct file_log_entry *entry)
{
    char *free_space = get_free_space_base_addr();
    char *page_addr = free_space + (nvm_addr2off(entry) - nvm_addr2off(free_space)) / 
                                                        UNVMFS_PAGE_SIZE * UNVMFS_PAGE_SIZE;

    return get_nvm_page2node_addr(page_addr);
}

struct file_log_entry *get_log_entry(u64 *log_tail)
{
    u64 entry_size = sizeof(struct file_log_entry);
    list_node_t *pages = NULL;
    list_node_t *last_page = NULL;
    u64 free_space = nvm_addr2off(get_free_space_base_addr());
    u64 last_page_off = free_space + (*log_tail - free_space - 1) / UNVMFS_PAGE_SIZE * UNVMFS_PAGE_SIZE;
    
    if (is_last_entry(*log_tail)) {
        last_page = get_nvm_page_node_addr(last_page_off);
        pages = alloc_free_pages(WRITE_LOG_PERALLOC);
        last_page->next_offset = pages->offset;
        *log_tail = pages->offset + entry_size;
        
        pages->invalid_cnt = 0;
        pages->obj_cnt = 1;
        pages->invalid_pages = 0;
        pages->pages_cnt = 0;
        return nvm_off2addr(pages->offset);
    }

    *log_tail += entry_size;
    pages = entry2log_page(nvm_off2addr(*log_tail));
    ++pages->obj_cnt;
    return nvm_off2addr(*log_tail);
}

void init_log_entry(struct file_log_entry *entry, list_node_t *pages, u64 file_size, 
                            u64 alloc_num, u64 start_wrtie_off, u64 end_write_off)
{
    list_node_t *page_node;
    
    page_node = entry2log_page(entry);
    page_node->pages_cnt += alloc_num;
    
    memset(entry, 0, sizeof(struct file_log_entry));

    entry->data = nvm_addr2off(pages);
    entry->file_size = file_size;

    entry->start_write_off = start_wrtie_off;
    entry->end_write_off = end_write_off;
    
    entry->num_pages = alloc_num;
}

void update_inode_tail(struct unvmfs_inode *inode, u64 log_tail)
{
    inode->log_tail = log_tail;
}

u64 next_log_page(u64 tail)
{
    list_node_t *last_page = NULL;
    char * free_space = get_free_space_base_addr();
    char * last_page_addr = free_space + (tail - nvm_addr2off(free_space)) / 
                                                UNVMFS_PAGE_SIZE * UNVMFS_PAGE_SIZE;

    last_page = get_nvm_page2node_addr(last_page_addr);

    return last_page->next_offset;
}

void invalidate_old_log_entry(struct file_log_entry *old_entry, u64 page_off)
{
    u64 start_off = old_entry->start_write_off / PAGE_SIZE;
    u64 curr_off = page_off / PAGE_SIZE;
    u64 pos = curr_off - start_off;
    list_node_t *page_node = NULL;
    
    old_entry->bitmap |= (1ULL << pos);
    ++old_entry->invalid_pages;

    page_node = entry2log_page(old_entry);
    ++page_node->invalid_pages;
    if (old_entry->num_pages == old_entry->invalid_pages) {
        ++page_node->invalid_cnt;
    }
}


void update_radixtree(struct unvmfs_inode *inode, u64 old_log_tail)
{
    u32 i;
    u64 log_tail = inode->log_tail;
    u64 curr_off = old_log_tail;
    u64 entry_size = sizeof(struct file_log_entry);
    u64 old_entry_off;
    u64 page_off;
    u64 file_page;
    struct file_log_entry *entry = NULL;
    struct file_log_entry *old_entry = NULL;
    list_node_t *page_node = NULL;

    while (curr_off != log_tail) {
        if (is_last_entry(curr_off)) {
            curr_off = next_log_page(curr_off);
        }
        
        entry = nvm_off2addr(curr_off);
        page_off = entry->start_write_off / PAGE_SIZE * PAGE_SIZE;
        for (i = 0; i < entry->num_pages; ++i) {
            file_page = get_radixtree_node(&inode->radix_tree, page_off, RADIXTREE_PAGE);
            if (file_page != OFFSET_NULL) {
                page_node = get_nvm_page_node_addr(file_page);
                old_entry_off = page_node->log_entry;
                old_entry = nvm_off2addr(old_entry_off);
                invalidate_old_log_entry(old_entry, page_off);
            }
            set_radixtree_node(&inode->radix_tree, curr_off, page_off, RADIXTREE_PAGE);
            page_off += PAGE_SIZE;
        }

        curr_off += entry_size;
    }
}


ssize_t nvmio_write(struct unvmfs_inode *inode, const void *buf, size_t cnt)
{
    int i;
    u64 file_off;
    u64 file_size;
    u64 writed = 0;
    u64 offset;
    u32 page_nums;
    u32 start_page;
    int alloc_num;
    u32 wr_size;
    u32 total_size = cnt;
    u32 last_page_wr;
    const char *user_buf = buf;
    char *node2page;
    list_node_t *pages = NULL;
    list_node_t *part_page = NULL;
    list_node_t *page_addr_node = NULL;
    char *page_addr = NULL;
    u64 part_page_off;
    u64 start_write_off;
    u64 end_write_off;
    u64 log_tail;
    u64 old_log_tail;
    u64 entry_off;
    
    struct file_log_entry *entry = NULL;

    pthread_rwlock_wrlock(&inode->rwlockp);
    log_tail = inode->log_tail;
    file_off = inode->file_off;
    file_size = inode->i_size;
    offset = file_off & (~PAGE_MASK);
    page_nums = ((cnt + offset - 1) >> PAGE_SHIFT) + 1;
    //last_page_wr = (file_off + cnt - 1) & (~PAGE_MASK) + 1;
    start_page = file_off / PAGE_SIZE * PAGE_SIZE;
    

    while (page_nums > 0) {
        if (page_nums > MAX_NUM_PAGE_PER_ENTRY) {
            alloc_num = MAX_NUM_PAGE_PER_ENTRY;
        } else {
            alloc_num = page_nums;
        }

        pages = alloc_free_pages(alloc_num);
        if (pages == NULL) {
            // todo
        }
        node2page = get_nvm_node2page_addr(pages);

        wr_size = PAGE_SIZE * alloc_num - offset;
        if (total_size < wr_size) {
            last_page_wr = PAGE_SIZE - (wr_size - total_size);
            wr_size = total_size;
        } else {
            last_page_wr = PAGE_SIZE;
        }

        entry = get_log_entry(&log_tail);
        if (entry == NULL) {
            // todo
        }
        entry_off = nvm_addr2off(entry);

        // first page
        page_addr = node2page;
        page_addr_node = get_nvm_page2node_addr(page_addr);
        page_addr_node->log_entry = entry_off;
        memcpy(page_addr + offset, user_buf, PAGE_SIZE - offset);
        user_buf += PAGE_SIZE - offset;
        // middle pages
        for (i = 0; i < alloc_num - 2; ++i) {
            page_addr_node = get_nvm_page2node_addr(page_addr);
            page_addr_node->log_entry = entry_off;
            page_addr = nvm_off2addr(page_addr_node->next_offset);
            memcpy(page_addr, user_buf, PAGE_SIZE);
            user_buf += PAGE_SIZE;
        }
        // last page
        if (alloc_num > 1) {
            page_addr_node = get_nvm_page2node_addr(page_addr);
            page_addr_node->log_entry = entry_off;
            page_addr = nvm_off2addr(page_addr_node->next_offset);
            memcpy(page_addr, user_buf, last_page_wr);
            user_buf += last_page_wr;
        }

        // fill first and last page
        if (offset != 0) {
            part_page_off = get_radixtree_node(&inode->radix_tree, start_page, RADIXTREE_PAGE);
            part_page = nvm_off2addr(part_page_off);
            memcpy(node2page, part_page, offset);
        }
        if (last_page_wr != PAGE_SIZE) {
            part_page_off = get_radixtree_node(&inode->radix_tree, start_page, RADIXTREE_PAGE);
            if (part_page_off != OFFSET_NULL) {
                part_page = nvm_off2addr(part_page_off);
                memcpy(page_addr + last_page_wr, part_page + last_page_wr, PAGE_SIZE - last_page_wr);
            }
        }

        if (file_off + writed > file_size) {
            file_size = file_off + writed;
        }

        start_write_off = start_page;
        end_write_off = start_write_off + wr_size;
        init_log_entry(entry, get_nvm_page2node_addr(node2page), file_size, alloc_num, start_write_off, end_write_off);

        page_nums -= alloc_num;
        total_size -= wr_size;
        user_buf += wr_size;
        writed += wr_size;
        start_page += alloc_num * PAGE_SIZE;
        offset = 0;
        
    }
    
    old_log_tail = inode->log_tail;
    update_inode_tail(inode, log_tail);

    update_radixtree(inode, old_log_tail);

    pthread_rwlock_unlock(&inode->rwlockp);

    return cnt;
}

ssize_t nvmio_read(struct unvmfs_inode *inode, void *buf, size_t cnt)
{
    u64 file_off;
    u64 wr_size = 0;
    u64 offset;
    u32 start_page;
    u64 total_size = cnt;
    u64 part_page_off;
    char *page_addr = NULL;
    char *user_buf = buf;
    char fill_zero[PAGE_SIZE];

    memset(fill_zero, 0, PAGE_SIZE);

    pthread_rwlock_rdlock(&inode->rwlockp);
    file_off = inode->file_off;
    if (file_off + cnt > inode->i_size) {
        pthread_rwlock_unlock(&inode->rwlockp);
        return -1;
    }
    
    offset = file_off & (~PAGE_MASK);
    start_page = file_off / PAGE_SIZE * PAGE_SIZE;

    while (total_size > 0) {
        part_page_off = get_radixtree_node(&inode->radix_tree, start_page, RADIXTREE_PAGE);
        // to adapt lseek
        if (part_page_off != OFFSET_NULL) {
            page_addr = nvm_off2addr(part_page_off);
        } else {
            page_addr = fill_zero;
        }

        if (offset) {
            if (total_size < PAGE_SIZE - offset) {
                wr_size = total_size;
            } else {
                wr_size = PAGE_SIZE - offset;
            }
        } else if (total_size < PAGE_SIZE) {
            wr_size = total_size;
        } else {
            wr_size = PAGE_SIZE;
        }
        memcpy(user_buf, page_addr + offset, wr_size);
        
        user_buf += wr_size;
        total_size -= wr_size;
        start_page += PAGE_SIZE;
        offset = 0;
    }
    
    pthread_rwlock_unlock(&inode->rwlockp);

    return cnt;
}


