#include "internal.h"


void *g_nvm_base_addr = NULL;

inline void *get_superblock(void)
{
    return g_nvm_base_addr;
}

inline void *get_page_list_base_addr(void)
{
    return (char *)g_nvm_base_addr + PAGE_LIST_START * PAGE_SIZE;
}

inline void *get_inode_list_base_addr(void)
{
    return (char *)g_nvm_base_addr + INODE_TABLE_START * PAGE_SIZE;
}

inline void *get_radixtree_list_base_addr(void)
{
    return (char *)g_nvm_base_addr + RADIXTREE_START * PAGE_SIZE;
}

inline void *get_free_space_base_addr(void)
{
    return (char *)g_nvm_base_addr + FREE_SPACE_START * PAGE_SIZE;
}

inline void *get_local_page_list_addr(u64 num)
{
    return (char *)g_nvm_base_addr + PAGE_LIST_START * PAGE_SIZE + sizeof(pagelist_t) * num;
}

inline void *get_radixtree_table_addr(u64 num)
{
    return (char *)g_nvm_base_addr + RADIXTREE_START * PAGE_SIZE + sizeof(allocator_list_t) * num;
}

inline void *get_unvmfs_inode_table_addr(u64 num)
{
    return (char *)g_nvm_base_addr + INODE_TABLE_START * PAGE_SIZE + sizeof(allocator_list_t) * num;
}

inline void *get_nvm_page_node_addr(u64 offset)
{
    return (char *)g_nvm_base_addr + offset + PAGE_SIZE;
}

inline void *get_nvm_node2page_addr(void *addr)
{
    return (char *)addr - PAGE_SIZE;
}

inline void *get_nvm_page2node_addr(void *addr)
{
    return (char *)addr + PAGE_SIZE;
}

inline u64 get_nvm_page_off2free_space(u64 offset)
{
    return offset - FREE_SPACE_START * PAGE_SIZE;
}

inline u64 nvm_addr2off(void *addr)
{
    return (char *)addr - (char *)g_nvm_base_addr;
}

inline void *nvm_off2addr(u64 offset)
{
    return (char *)g_nvm_base_addr + offset;
}

