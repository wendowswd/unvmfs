#include <pthread.h>

#include "internal.h"
#include "inode.h"
#include "radixtree.h"


void init_unvmfs_inode(struct unvmfs_inode *inode_node)
{
    inode_node->valid = 0;
    inode_node->deleted = 0;
    inode_node->i_size = 0;

    inode_node->fd = -1;
    inode_node->log_head = OFFSET_NULL;
    inode_node->log_tail = OFFSET_NULL;
    inode_node->file_off = 0;
    //inode_node->nvm_base_addr = OFFSET_NULL;

    init_radixtree_root(&inode_node->radix_tree, RADIXTREE_PAGE);
    pthread_rwlock_init(&inode_node->rwlockp, NULL);
}