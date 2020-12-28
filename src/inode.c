#include <pthread.h>

#include "internal.h"
#include "inode.h"
#include "radixtree.h"
#include "allocator.h"


void init_unvmfs_inode(struct unvmfs_inode *inode_node)
{
    list_node_t *pages = NULL;

    UNVMFS_DEBUG("init_unvmfs_inode start");
    
    inode_node->valid = 0;
    inode_node->deleted = 0;
    inode_node->i_size = 0;

    inode_node->fd = -1;

    
    pages = alloc_free_pages(WRITE_LOG_PERALLOC);
        
    inode_node->log_head = pages->offset;
    inode_node->log_tail = pages->offset;
    inode_node->file_off = 0;
    //inode_node->nvm_base_addr = OFFSET_NULL;

    init_radixtree_root(&inode_node->radix_tree, RADIXTREE_PAGE);
    //pthread_rwlock_init(&inode_node->rwlockp, NULL);
    pthread_mutex_init(&inode_node->mutex, NULL);

    UNVMFS_DEBUG("init_unvmfs_inode success");
}