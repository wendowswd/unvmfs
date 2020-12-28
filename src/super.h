#ifndef _UNVMFS_SUPER_H
#define _UNVMFS_SUPER_H

#include <pthread.h>

#include "types.h"
#include "hashmap.h"
#include "list.h"
#include "radixtree.h"

#define MAX_VOLUME_NAME_LEN 64

struct unvmfs_super_block {
	u64         s_init;			/* initilization num */
	
	u32                 s_magic;		/* magic signature */
	u32                 s_blocksize;		/* blocksize in bytes */
	u64                 s_size;			/* total size of fs in bytes */
	s8                  s_volume_name[MAX_VOLUME_NAME_LEN];	/* volume name */

	/* s_mtime and s_wtime should be together and their order should not be
	 * changed. we use an 8 byte write to update both of them atomically
	 */
	u32                 s_mtime;		/* mount time */
	u32                 s_wtime;		/* write time */
    

	u64                 num_blocks;		/* total nvm blocks */
    u64                 free_num_pages; /* available page nums */
	radixtree_t         hash_root;		/* filesystem root */
    struct list_head    s_list;         /* inode list */
	u32                 cpu_nums;       /* cpu nums */

    //pthread_rwlock_t rwlockp;
    pthread_mutex_t mutex;
};

void kernel_superblock_init(struct unvmfs_super_block *addr, char *unvmfs_path);
void kernel_page_list_init(void);
void kernel_inode_list_init(void);
void kernel_radixtree_list_init(void);



#endif /* _UNVMFS_SUPER_H */


