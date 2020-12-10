#ifndef _UNVMFS_DIR_H
#define _UNVMFS_INODE_H

#include "hashmap.h"

 /* Structure of an dir in PMEM */
struct unvmfs_dir {

	/* first 40 bytes */
	u8			valid;		 		/* Is this dir valid? */
	u8			deleted;	 		/* Is this dir deleted? */
	u8			i_blk_type;	 		/* data block size this dir uses */
	u8			i_padding;
	u32			i_flags;	 		/* dir flags */
	u64			i_size;		 		/* Size of data in bytes */
	u32			i_ctime;	 		/* dir modification time */
	u32			i_mtime;	 		/* dir b-tree Modification time */
	u32			i_atime;	 		/* Access time */
	u16			i_mode;		 		/* dir mode */
	u16			i_links_count;	 	/* Links count */
	u64			i_xattr;	 		/* Extended attribute block */
	u32			i_uid;		 		/* Owner Uid */
	u32			i_gid;		 		/* Group Id */
	u32			i_create_time;	 	/* Create time */
	
	hashmap_map hash_root;          /* hashmap (for fast lookup inode) */
};


#endif /* _UNVMFS_DIR_H */


