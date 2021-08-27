#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/fs.h>
#include <linux/pagemap.h>
#include "ecryptfs_kernel.h"

int ecryptfs_write_lower(struct inode *ecryptfs_inode, char *data,
			 loff_t offset, size_t size)
{
	struct file *lower_file;
	mm_segment_t fs_save;
	ssize_t rc;

	lower_file = ecryptfs_inode_to_private(ecryptfs_inode)->lower_file;
	if (!lower_file)
		return -EIO;
	fs_save = get_fs();
	set_fs(get_ds());
	rc = vfs_write(lower_file, data, size, &offset);
	set_fs(fs_save);
	mark_inode_dirty_sync(ecryptfs_inode);
	return rc;
}

int ecryptfs_write_lower_page_segment(struct inode *ecryptfs_inode,
				      struct page *page_for_lower,
				      size_t offset_in_page, size_t size)
{
	char *virt;
	loff_t offset;
	int rc;

	offset = ((((loff_t)page_for_lower->index) << PAGE_CACHE_SHIFT)
		  + offset_in_page);
	virt = kmap(page_for_lower);
	rc = ecryptfs_write_lower(ecryptfs_inode, virt, offset, size);
	if (rc > 0)
		rc = 0;
	kunmap(page_for_lower);
	return rc;
}

int ecryptfs_write(struct inode *ecryptfs_inode, char *data, loff_t offset,
		   size_t size)
{
	struct page *ecryptfs_page;
	struct ecryptfs_crypt_stat *crypt_stat;
	char *ecryptfs_page_virt;
	loff_t ecryptfs_file_size = i_size_read(ecryptfs_inode);
	loff_t data_offset = 0;
	loff_t pos;
	int rc = 0;

	crypt_stat = &ecryptfs_inode_to_private(ecryptfs_inode)->crypt_stat;
	 
	if (offset > ecryptfs_file_size)
		pos = ecryptfs_file_size;
	else
		pos = offset;
	while (pos < (offset + size)) {
		pgoff_t ecryptfs_page_idx = (pos >> PAGE_CACHE_SHIFT);
		size_t start_offset_in_page = (pos & ~PAGE_CACHE_MASK);
		size_t num_bytes = (PAGE_CACHE_SIZE - start_offset_in_page);
		loff_t total_remaining_bytes = ((offset + size) - pos);

		if (fatal_signal_pending(current)) {
			rc = -EINTR;
			break;
		}

		if (num_bytes > total_remaining_bytes)
			num_bytes = total_remaining_bytes;
		if (pos < offset) {
			 
			loff_t total_remaining_zeros = (offset - pos);

			if (num_bytes > total_remaining_zeros)
				num_bytes = total_remaining_zeros;
		}
		ecryptfs_page = ecryptfs_get_locked_page(ecryptfs_inode,
							 ecryptfs_page_idx);
		if (IS_ERR(ecryptfs_page)) {
			rc = PTR_ERR(ecryptfs_page);
#ifdef MY_DEF_HERE
			printk(KERN_ERR "%s: Error getting page at "
			       "index [%lld] from eCryptfs inode "
			       "mapping; rc = [%d]\n", __func__,
			       (unsigned long long)ecryptfs_page_idx, rc);
#else
			printk(KERN_ERR "%s: Error getting page at "
			       "index [%ld] from eCryptfs inode "
			       "mapping; rc = [%d]\n", __func__,
			       ecryptfs_page_idx, rc);
#endif
			goto out;
		}
		ecryptfs_page_virt = kmap_atomic(ecryptfs_page, KM_USER0);

		if (pos < offset || !start_offset_in_page) {
			 
			memset(((char *)ecryptfs_page_virt
				+ start_offset_in_page), 0,
				PAGE_CACHE_SIZE - start_offset_in_page);
		}

		if (pos >= offset) {
			memcpy(((char *)ecryptfs_page_virt
				+ start_offset_in_page),
			       (data + data_offset), num_bytes);
			data_offset += num_bytes;
		}
		kunmap_atomic(ecryptfs_page_virt, KM_USER0);
		flush_dcache_page(ecryptfs_page);
		SetPageUptodate(ecryptfs_page);
		unlock_page(ecryptfs_page);
		if (crypt_stat->flags & ECRYPTFS_ENCRYPTED)
			rc = ecryptfs_encrypt_page(ecryptfs_page);
		else
			rc = ecryptfs_write_lower_page_segment(ecryptfs_inode,
						ecryptfs_page,
						start_offset_in_page,
						data_offset);
		page_cache_release(ecryptfs_page);
		if (rc) {
			printk(KERN_ERR "%s: Error encrypting "
			       "page; rc = [%d]\n", __func__, rc);
			goto out;
		}
		pos += num_bytes;
	}
	if (pos > ecryptfs_file_size) {
		i_size_write(ecryptfs_inode, pos);
		if (crypt_stat->flags & ECRYPTFS_ENCRYPTED) {
			int rc2;

			rc2 = ecryptfs_write_inode_size_to_metadata(
								ecryptfs_inode);
			if (rc2) {
#ifdef MY_ABC_HERE
				if (-EDQUOT != rc && -ENOSPC != rc)
#endif

				printk(KERN_ERR	"Problem with "
				       "ecryptfs_write_inode_size_to_metadata; "
				       "rc = [%d]\n", rc2);
				if (!rc)
					rc = rc2;
				goto out;
			}
		}
	}
out:
	return rc;
}

int ecryptfs_read_lower(char *data, loff_t offset, size_t size,
			struct inode *ecryptfs_inode)
{
	struct file *lower_file;
	mm_segment_t fs_save;
	ssize_t rc;

	lower_file = ecryptfs_inode_to_private(ecryptfs_inode)->lower_file;
	if (!lower_file)
		return -EIO;
	fs_save = get_fs();
	set_fs(get_ds());
	rc = vfs_read(lower_file, data, size, &offset);
	set_fs(fs_save);
	return rc;
}

int ecryptfs_read_lower_page_segment(struct page *page_for_ecryptfs,
				     pgoff_t page_index,
				     size_t offset_in_page, size_t size,
				     struct inode *ecryptfs_inode)
{
	char *virt;
	loff_t offset;
	int rc;

	offset = ((((loff_t)page_index) << PAGE_CACHE_SHIFT) + offset_in_page);
	virt = kmap(page_for_ecryptfs);
	rc = ecryptfs_read_lower(virt, offset, size, ecryptfs_inode);
	if (rc > 0)
		rc = 0;
	kunmap(page_for_ecryptfs);
	flush_dcache_page(page_for_ecryptfs);
	return rc;
}

#if 0
 
int ecryptfs_read(char *data, loff_t offset, size_t size,
		  struct file *ecryptfs_file)
{
	struct inode *ecryptfs_inode = ecryptfs_file->f_dentry->d_inode;
	struct page *ecryptfs_page;
	char *ecryptfs_page_virt;
	loff_t ecryptfs_file_size = i_size_read(ecryptfs_inode);
	loff_t data_offset = 0;
	loff_t pos;
	int rc = 0;

	if ((offset + size) > ecryptfs_file_size) {
		rc = -EINVAL;
		printk(KERN_ERR "%s: Attempt to read data past the end of the "
			"file; offset = [%lld]; size = [%td]; "
		       "ecryptfs_file_size = [%lld]\n",
		       __func__, offset, size, ecryptfs_file_size);
		goto out;
	}
	pos = offset;
	while (pos < (offset + size)) {
		pgoff_t ecryptfs_page_idx = (pos >> PAGE_CACHE_SHIFT);
		size_t start_offset_in_page = (pos & ~PAGE_CACHE_MASK);
		size_t num_bytes = (PAGE_CACHE_SIZE - start_offset_in_page);
		size_t total_remaining_bytes = ((offset + size) - pos);

		if (num_bytes > total_remaining_bytes)
			num_bytes = total_remaining_bytes;
		ecryptfs_page = ecryptfs_get_locked_page(ecryptfs_inode,
							 ecryptfs_page_idx);
		if (IS_ERR(ecryptfs_page)) {
			rc = PTR_ERR(ecryptfs_page);
			printk(KERN_ERR "%s: Error getting page at "
			       "index [%ld] from eCryptfs inode "
			       "mapping; rc = [%d]\n", __func__,
			       ecryptfs_page_idx, rc);
			goto out;
		}
		ecryptfs_page_virt = kmap_atomic(ecryptfs_page, KM_USER0);
		memcpy((data + data_offset),
		       ((char *)ecryptfs_page_virt + start_offset_in_page),
		       num_bytes);
		kunmap_atomic(ecryptfs_page_virt, KM_USER0);
		flush_dcache_page(ecryptfs_page);
		SetPageUptodate(ecryptfs_page);
		unlock_page(ecryptfs_page);
		page_cache_release(ecryptfs_page);
		pos += num_bytes;
		data_offset += num_bytes;
	}
out:
	return rc;
}
#endif   
