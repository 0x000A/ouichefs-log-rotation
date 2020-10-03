#ifndef _OUICHEFS_H
#define __OUICHEFS_H


#define OUICHEFS_FILENAME_LEN            28
#define OUICHEFS_MAX_SUBFILES           128


struct ouichefs_dir_block {
	struct ouichefs_file {
		uint32_t inode;
		char filename[OUICHEFS_FILENAME_LEN];
	} files[OUICHEFS_MAX_SUBFILES];
};


struct ouichefs_inode_info {
	uint32_t index_block;
	struct inode vfs_inode;
};

struct ouichefs_sb_info {
	uint32_t magic;	        /* Magic number */

	uint32_t nr_blocks;      /* Total number of blocks (incl sb & inodes) */
	uint32_t nr_inodes;      /* Total number of inodes */

	uint32_t nr_istore_blocks;/* Number of inode store blocks */
	uint32_t nr_ifree_blocks; /* Number of inode free bitmap blocks */
	uint32_t nr_bfree_blocks; /* Number of block free bitmap blocks */

	uint32_t nr_free_inodes;  /* Number of free inodes */
	uint32_t nr_free_blocks;  /* Number of free blocks */

	unsigned long *ifree_bitmap; /* In-memory free inodes bitmap */
	unsigned long *bfree_bitmap; /* In-memory free blocks bitmap */

	int (*cleaning_policy)(struct inode *, struct ouichefs_dir_block *);
	void (*clean_blocks)(struct super_block *);
	struct super_block *sb;
};


struct inode *ouichefs_iget(struct super_block *sb, unsigned long ino);

/* Getter for superblock */
#define OUICHEFS_SB(sb) (sb->s_fs_info)
#define OUICHEFS_INODE(inode) (container_of(inode, struct ouichefs_inode_info, \
					    vfs_inode))


#endif /* _OUICHEFS_H */
