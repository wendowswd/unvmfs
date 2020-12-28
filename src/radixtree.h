#ifndef _UNVMFS_RADIXLOG_H
#define _UNVMFS_RADIXLOG_H

#include "types.h"

#define PER_NODE_SHIFT 8
#define PTRS_PER_NODE (1ULL << PER_NODE_SHIFT)       /* 256 */

typedef enum rdixtree_type_enum {
  RADIXTREE_INODE,
  RADIXTREE_PAGE,
} radixtree_type_t;

typedef struct radixtree_node_struct {
  u64 in_page;
  u64 next;

  int count;
  int index;
  u64 entries[PTRS_PER_NODE];
  //pthread_rwlock_t rwlockp;
  pthread_mutex_t mutex;
} radixtree_node_t;


typedef struct radixtree_struct {
  u64 count;
  
  u64 root;
  radixtree_type_t type;
  pthread_mutex_t mutex;
} radixtree_t;

void init_radixtree_root(radixtree_t *root, radixtree_type_t type);
void init_radixtree_node(radixtree_node_t *node);
u64 get_radixtree_node(radixtree_t *root, u64 index, radixtree_type_t type);
void set_radixtree_node(radixtree_t *root, u64 value, u64 index, radixtree_type_t type);

int atomic_increase(int *count);

#endif /* _UNVMFS_RADIXLOG_H */

