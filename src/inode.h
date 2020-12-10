#ifndef _UNVMFS_INODE_H
#define _UNVMFS_INODE_H

#include<pthread.h>

#include "list.h"
#include "radixtree.h"
#include "types.h"

 /* Structure of an inode in PMEM */
struct unvmfs_inode {
	u8          valid;		 		/* Is this inode valid? */
	u8          deleted;	 		/* Is this inode deleted? */
	u8          i_blk_type;	 		/* data block size this inode uses */
	u8          i_padding;
	u32         i_flags;	 		/* Inode flags */
	u64         i_size;		 		/* Size of data in bytes */
	u32         i_ctime;	 		/* Inode modification time */
	u32         i_mtime;	 		/* Inode b-tree Modification time */
	u32         i_atime;	 		/* Access time */
	u16         i_mode;		 		/* File mode */
	u16         i_links_count;	 	/* Links count */
	u64         i_xattr;	 		/* Extended attribute block */
	u32         i_uid;		 		/* Owner Uid */
	u32         i_gid;		 		/* Group Id */
	u32         i_create_time;	 	/* Create time */

	u32         fd;					/* kernel file description */
	u64         log_head;	 		/* Log head pointer */
	u64         log_tail;	 		/* Log tail pointer */
    u64         file_off;           /* current file offset */
	//u64         nvm_base_addr;		/* mmap nvm */
	
    u64         next;               /* used for inode list */
    u64         in_page;            /* in which nvm page */
    radixtree_t radix_tree;			/* radix tree (for fast lookup pages) */
    struct list_head l_node;        /* inode list */
    //struct rb_root rb_tree;         /* 
    pthread_rwlock_t rwlockp;
};

void init_unvmfs_inode(struct unvmfs_inode *inode_node);

#endif /* _UNVMFS_INODE_H */