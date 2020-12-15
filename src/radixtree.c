#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "internal.h"
#include "radixtree.h"

void init_radixtree_root(radixtree_t *root, radixtree_type_t type)
{
    root->count = 0;
    root->root = alloc_radixtree_node();
    root->type = type;
    pthread_mutex_init(&root->mutex, NULL);
}


void init_radixtree_node(radixtree_node_t *node)
{
    node->count = 0;
    node->index = 0;
    memset(node->entries, 0xff, sizeof(node->entries));
    pthread_rwlock_init(&node->rwlockp, NULL);
}

u64 get_radixtree_node(radixtree_t *root, u64 index, radixtree_type_t type)
{
    int i;
    int idx;
    int id;
    radixtree_node_t *node = NULL;

    UNVMFS_DEBUG("get_radixtree_node start");

    if (root == NULL) {
        return OFFSET_NULL;
    }

    if (type == RADIXTREE_PAGE) {
        index >>= PAGE_SHIFT;
    }

    idx = index;
    node = nvm_off2addr(root->root);
    for (i = 1; i < RADIXTREE_LEVEL; ++i) {
        id = idx & RADIXTREE_IDX_MASK;
        idx >>= PER_NODE_SHIFT;
        if (node->entries[id] == OFFSET_NULL) {
            return OFFSET_NULL;
        }
        node = nvm_off2addr(node->entries[id]);
    }

    id = idx & RADIXTREE_IDX_MASK;

    UNVMFS_DEBUG("get_radixtree_node success");
    
    return node->entries[id];
}

void set_radixtree_node(radixtree_t *root, u64 value, u64 index, radixtree_type_t type)
{
    int i;
    int idx;
    int id;
    radixtree_node_t *node = NULL;
    u64 new_node;

    UNVMFS_DEBUG("set_radixtree_node start");

    if (root == NULL || node == NULL) {
        return;
    }

    if (type == RADIXTREE_PAGE) {
        index >>= PAGE_SHIFT;
    }

    idx = index;
    node = nvm_off2addr(root->root);
    for (i = 1; i < RADIXTREE_LEVEL; ++i) {
        id = idx & RADIXTREE_IDX_MASK;
        idx >>= PER_NODE_SHIFT;
        new_node = alloc_radixtree_node();
        if (!__sync_bool_compare_and_swap(&node->entries[id], OFFSET_NULL, new_node)) {
            free_radixtree_node(nvm_off2addr(new_node));
        }
        node = nvm_off2addr(node->entries[id]);
    }
    id = idx & RADIXTREE_IDX_MASK;
    if (!__sync_bool_compare_and_swap(&node->entries[id], OFFSET_NULL, value)) {
        free_unvmfs_inode(nvm_off2addr(value));
    }

    UNVMFS_DEBUG("set_radixtree_node success");
    return;
}

inline int atomic_increase(int *count) {
  int old, new;

  do {
    old = *count;
    new = *count + 1;
  } while (!__sync_bool_compare_and_swap(count, old, new));

  return new;
}



