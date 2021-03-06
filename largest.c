#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/bitmap.h>
#include <linux/buffer_head.h>
#include "ouichefs.h"

MODULE_DESCRIPTION("\"Largest\" log rotation policy for ouichefs file system");
MODULE_AUTHOR("Imad Eddine REZGUI, Sorbonne Universite");
MODULE_LICENSE("GPL");

// Parameter x
static int x;
module_param(x, int, 0);
MODULE_PARM_DESC(x, "ouichefs gets automatically cleaned if x\% blocks are used");
EXPORT_SYMBOL_GPL(x);

static struct file_system_type *ouichefs;



/*
 * Uses the policy to automatically remove the file with the oldest
 * modification time in the partition once x% of partition's blocks
 * are used
 */
void clean_blocks(struct super_block *sb)
{
	struct ouichefs_sb_info *sbi = OUICHEFS_SB(sb);
	struct inode *cur = NULL;
	struct inode *max_inode = NULL;
	struct inode *dir_inode = NULL;
	struct dentry dentry;
	loff_t size;
	loff_t max_size;
	struct buffer_head *bh = NULL;
	struct ouichefs_dir_block *dblock = NULL;
	struct ouichefs_inode_info *ci_dir = NULL;
	struct ouichefs_file *f = NULL;
	uint32_t ino = 0;
	int i;

	unsigned long p_free = (sbi->nr_free_blocks / sbi->nr_blocks) * 100;

	if (p_free < x)
		return;

	max_size = 0;

	/* look for the inode with the oldest modification time */
	while (ino < sbi->nr_inodes) {
		ino = find_next_zero_bit(sbi->ifree_bitmap,
					 sbi->nr_inodes, ino);

		if (ino == sbi->nr_inodes)
			break;


		cur = ouichefs_iget(sb, ino);
		ino++;

		/* inode is a directory or being used */
		if (S_ISDIR(cur->i_mode) || cur->i_count.counter > 1) {
			iput(cur);
			continue;
		}

		size = cur->i_size;

		/* inode does not have the oldest modification time */
		if (max_size >= size) {
			iput(cur);
			continue;
		}

		max_size = size;

		if (max_inode)
			iput(max_inode);

		max_inode = cur;
	}



	/* look for the inode's parent and remove it */
	if (!max_inode) {
		iput(max_inode);
		return;
	}

	ino = 0;
	while (ino < sbi->nr_inodes) {
		ino = find_next_zero_bit(sbi->ifree_bitmap,
					 sbi->nr_inodes, ino);

		if (ino == sbi->nr_inodes)
			break;

		cur = ouichefs_iget(sb, ino);
		ino++;

		/* Inode is not a directory */
		if (!S_ISDIR(cur->i_mode)) {
			iput(cur);
			continue;
		}

		ci_dir = OUICHEFS_INODE(cur);

		/* Read the directory index block on disk */
		bh = sb_bread(sb, ci_dir->index_block);
		if (!bh) {
			iput(cur);
			return;
		}

		dblock = (struct ouichefs_dir_block *)bh->b_data;


		/* Search for the file in directory */
		for (i = 0; i < OUICHEFS_MAX_SUBFILES; i++) {
			f = &dblock->files[i];
			if (!f->inode) {
				iput(cur);
				break;
			}

			if (f->inode == max_inode->i_ino) {
				dir_inode = cur;
				break;
			}
		}
		brelse(bh);
	}

	/* Remove inode */
	if (!dir_inode) {
		iput(cur);
		return;
	}

	pr_info("removing inode: %ld\n", max_inode->i_ino);
	dentry.d_inode = max_inode;
	max_inode->i_op->unlink(dir_inode, &dentry);
	iput(max_inode);
	iput(dir_inode);
}



/*
 * Removes the file with the largest size form the given directory
 * this function is called by the ouichefs file system once a directory
 * is full
 */
int policy(struct inode *dir, struct ouichefs_dir_block *dblock)
{
	struct super_block *sb = dir->i_sb;
	struct inode *cur = NULL;
	struct inode *max_inode = NULL;
	struct dentry dentry;
	loff_t size;
	loff_t max_size;
	int i;

	max_size = 0;

	for (i = 0; i < OUICHEFS_MAX_SUBFILES; i++) {
		cur = ouichefs_iget(sb, dblock->files[i].inode);

		/* inode is a directory or being used */
		if (S_ISDIR(cur->i_mode) || cur->i_count.counter > 1) {
			iput(cur);
			continue;
		}


		size = cur->i_size;

		/* inode does not have the largest size */
		if (max_size >= size) {
			iput(cur);
			continue;
		}

		max_size = size;

		if (max_inode)
			iput(max_inode);

		max_inode = cur;
	}


	if (!max_inode) {
		iput(max_inode);
		return -1;
	}

	pr_info("removing inode: %ld\n", max_inode->i_ino);
	dentry.d_inode = max_inode;
	max_inode->i_op->unlink(dir, &dentry);
	iput(max_inode);

	return 0;
}



/*
 * Adds a policy to the ouichefs file system that will be executed
 * when a directory get full.
 * If variable "x" is available, the same policy will be executed
 * when x% of blocks are full
 */
void add_policy(struct super_block *sb, void *unused)
{
	struct ouichefs_sb_info *sbi = OUICHEFS_SB(sb);

	sbi->sb = sb;
	sbi->cleaning_policy = policy;

	if (x)
		sbi->clean_blocks = clean_blocks;
}



/*
 * Removes all policies from the ouichefs file system
 */
void remove_policy(struct super_block *sb, void *unused)
{
	struct ouichefs_sb_info *sbi = OUICHEFS_SB(sb);

	sbi->cleaning_policy = NULL;
	sbi->clean_blocks = NULL;
	sbi->sb = NULL;
}



/*
 * Uses the policy
 */
static void execute_policy(struct super_block *sb, void *unused)
{
	clean_blocks(sb);
}



/*
 * Sysfs write function
 */
ssize_t ouichefs_store(
		struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t count)
{
	int input;
	int old_x = x;

	if (count > 2)
		return -1;

	input = buf[0] - '0';
	if (input) {
		x = 0;
		iterate_supers_type(ouichefs, execute_policy, NULL);
		x = old_x;
	}

	return count;
}



static struct kobj_attribute kernel = __ATTR_WO(ouichefs);

static int __init rotation_init(void)
{
	struct super_block *sb;
	struct ouichefs_sb_info *sbi;


	ouichefs = get_fs_type("ouichefs");

	if (!ouichefs)
		pr_info("ERROR");

	pr_info("largest file policy added to ouichefs\n");
	pr_info("files will be removed when %d%% of block are used\n", x);
	iterate_supers_type(ouichefs, add_policy, NULL);
	sysfs_create_file(kernel_kobj, &kernel.attr);

	return 0;
}
module_init(rotation_init);

static void __exit rotation_exit(void)
{
	pr_info("largest file policy removed from ouichefs\n");
	iterate_supers_type(ouichefs, remove_policy, NULL);
	sysfs_remove_file(kernel_kobj, &kernel.attr);
}
module_exit(rotation_exit);
