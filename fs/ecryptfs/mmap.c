#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/pagemap.h>
#include <linux/writeback.h>
#include <linux/page-flags.h>
#include <linux/mount.h>
#include <linux/file.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <asm/unaligned.h>
#include "ecryptfs_kernel.h"

struct page *ecryptfs_get_locked_page(struct file *file, loff_t index)
{
	struct dentry *dentry;
	struct inode *inode;
	struct address_space *mapping;
	struct page *page;

	dentry = file->f_path.dentry;
	inode = dentry->d_inode;
	mapping = inode->i_mapping;
	page = read_mapping_page(mapping, index, (void *)file);
	if (!IS_ERR(page))
		lock_page(page);
	return page;
}

static int ecryptfs_writepage(struct page *page, struct writeback_control *wbc)
{
	int rc;

	rc = ecryptfs_encrypt_page(page);
	if (rc) {
		ecryptfs_printk(KERN_WARNING, "Error encrypting "
				"page (upper index [0x%.16x])\n", page->index);
		ClearPageUptodate(page);
		goto out;
	}
	SetPageUptodate(page);
	unlock_page(page);
out:
	return rc;
}

static void set_header_info(char *page_virt,
			    struct ecryptfs_crypt_stat *crypt_stat)
{
	size_t written;
	size_t save_num_header_bytes_at_front =
		crypt_stat->num_header_bytes_at_front;

	crypt_stat->num_header_bytes_at_front =
		ECRYPTFS_MINIMUM_HEADER_EXTENT_SIZE;
	ecryptfs_write_header_metadata(page_virt + 20, crypt_stat, &written);
	crypt_stat->num_header_bytes_at_front =
		save_num_header_bytes_at_front;
}

static int
ecryptfs_copy_up_encrypted_with_header(struct page *page,
				       struct ecryptfs_crypt_stat *crypt_stat)
{
	loff_t extent_num_in_page = 0;
	loff_t num_extents_per_page = (PAGE_CACHE_SIZE
				       / crypt_stat->extent_size);
	int rc = 0;

	while (extent_num_in_page < num_extents_per_page) {
		loff_t view_extent_num = ((((loff_t)page->index)
					   * num_extents_per_page)
					  + extent_num_in_page);
		size_t num_header_extents_at_front =
			(crypt_stat->num_header_bytes_at_front
			 / crypt_stat->extent_size);

		if (view_extent_num < num_header_extents_at_front) {
			 
			char *page_virt;

			page_virt = kmap_atomic(page, KM_USER0);
			memset(page_virt, 0, PAGE_CACHE_SIZE);
			 
			if (view_extent_num == 0) {
				rc = ecryptfs_read_xattr_region(
					page_virt, page->mapping->host);
				set_header_info(page_virt, crypt_stat);
			}
			kunmap_atomic(page_virt, KM_USER0);
			flush_dcache_page(page);
			if (rc) {
				printk(KERN_ERR "%s: Error reading xattr "
				       "region; rc = [%d]\n", __func__, rc);
				goto out;
			}
		} else {
			 
			loff_t lower_offset =
				((view_extent_num * crypt_stat->extent_size)
				 - crypt_stat->num_header_bytes_at_front);

			rc = ecryptfs_read_lower_page_segment(
				page, (lower_offset >> PAGE_CACHE_SHIFT),
				(lower_offset & ~PAGE_CACHE_MASK),
				crypt_stat->extent_size, page->mapping->host);
			if (rc) {
				printk(KERN_ERR "%s: Error attempting to read "
				       "extent at offset [%lld] in the lower "
				       "file; rc = [%d]\n", __func__,
				       lower_offset, rc);
				goto out;
			}
		}
		extent_num_in_page++;
	}
out:
	return rc;
}

static int ecryptfs_readpage(struct file *file, struct page *page)
{
	struct ecryptfs_crypt_stat *crypt_stat =
		&ecryptfs_inode_to_private(file->f_path.dentry->d_inode)->crypt_stat;
	int rc = 0;

	if (!crypt_stat
	    || !(crypt_stat->flags & ECRYPTFS_ENCRYPTED)
	    || (crypt_stat->flags & ECRYPTFS_NEW_FILE)) {
		ecryptfs_printk(KERN_DEBUG,
				"Passing through unencrypted page\n");
		rc = ecryptfs_read_lower_page_segment(page, page->index, 0,
						      PAGE_CACHE_SIZE,
						      page->mapping->host);
	} else if (crypt_stat->flags & ECRYPTFS_VIEW_AS_ENCRYPTED) {
		if (crypt_stat->flags & ECRYPTFS_METADATA_IN_XATTR) {
			rc = ecryptfs_copy_up_encrypted_with_header(page,
								    crypt_stat);
			if (rc) {
				printk(KERN_ERR "%s: Error attempting to copy "
				       "the encrypted content from the lower "
				       "file whilst inserting the metadata "
				       "from the xattr into the header; rc = "
				       "[%d]\n", __func__, rc);
				goto out;
			}

		} else {
			rc = ecryptfs_read_lower_page_segment(
				page, page->index, 0, PAGE_CACHE_SIZE,
				page->mapping->host);
			if (rc) {
				printk(KERN_ERR "Error reading page; rc = "
				       "[%d]\n", rc);
				goto out;
			}
		}
	} else {
		rc = ecryptfs_decrypt_page(page);
		if (rc) {
			ecryptfs_printk(KERN_ERR, "Error decrypting page; "
					"rc = [%d]\n", rc);
			goto out;
		}
	}
out:
	if (rc)
		ClearPageUptodate(page);
	else
		SetPageUptodate(page);
	ecryptfs_printk(KERN_DEBUG, "Unlocking page with index = [0x%.16x]\n",
			page->index);
	unlock_page(page);
	return rc;
}

static int fill_zeros_to_end_of_page(struct page *page, unsigned int to)
{
	struct inode *inode = page->mapping->host;
	int end_byte_in_page;

	if ((i_size_read(inode) / PAGE_CACHE_SIZE) != page->index)
		goto out;
	end_byte_in_page = i_size_read(inode) % PAGE_CACHE_SIZE;
	if (to > end_byte_in_page)
		end_byte_in_page = to;
	zero_user_segment(page, end_byte_in_page, PAGE_CACHE_SIZE);
out:
	return 0;
}

static int ecryptfs_write_begin(struct file *file,
			struct address_space *mapping,
			loff_t pos, unsigned len, unsigned flags,
			struct page **pagep, void **fsdata)
{
	pgoff_t index = pos >> PAGE_CACHE_SHIFT;
	struct page *page;
	loff_t prev_page_end_size;
	int rc = 0;

	page = grab_cache_page_write_begin(mapping, index, flags);
	if (!page)
		return -ENOMEM;
	*pagep = page;

	if (!PageUptodate(page)) {
		struct ecryptfs_crypt_stat *crypt_stat =
			&ecryptfs_inode_to_private(
				file->f_path.dentry->d_inode)->crypt_stat;

		if (!(crypt_stat->flags & ECRYPTFS_ENCRYPTED)
		    || (crypt_stat->flags & ECRYPTFS_NEW_FILE)) {
			rc = ecryptfs_read_lower_page_segment(
				page, index, 0, PAGE_CACHE_SIZE, mapping->host);
			if (rc) {
				printk(KERN_ERR "%s: Error attemping to read "
				       "lower page segment; rc = [%d]\n",
				       __func__, rc);
				ClearPageUptodate(page);
				goto out;
			} else
				SetPageUptodate(page);
		} else if (crypt_stat->flags & ECRYPTFS_VIEW_AS_ENCRYPTED) {
			if (crypt_stat->flags & ECRYPTFS_METADATA_IN_XATTR) {
				rc = ecryptfs_copy_up_encrypted_with_header(
					page, crypt_stat);
				if (rc) {
					printk(KERN_ERR "%s: Error attempting "
					       "to copy the encrypted content "
					       "from the lower file whilst "
					       "inserting the metadata from "
					       "the xattr into the header; rc "
					       "= [%d]\n", __func__, rc);
					ClearPageUptodate(page);
					goto out;
				}
				SetPageUptodate(page);
			} else {
				rc = ecryptfs_read_lower_page_segment(
					page, index, 0, PAGE_CACHE_SIZE,
					mapping->host);
				if (rc) {
					printk(KERN_ERR "%s: Error reading "
					       "page; rc = [%d]\n",
					       __func__, rc);
					ClearPageUptodate(page);
					goto out;
				}
				SetPageUptodate(page);
			}
		} else {
			rc = ecryptfs_decrypt_page(page);
			if (rc) {
				printk(KERN_ERR "%s: Error decrypting page "
				       "at index [%ld]; rc = [%d]\n",
				       __func__, page->index, rc);
				ClearPageUptodate(page);
				goto out;
			}
			SetPageUptodate(page);
		}
	}
	prev_page_end_size = ((loff_t)index << PAGE_CACHE_SHIFT);
	 
	if (index != 0) {
		if (prev_page_end_size > i_size_read(page->mapping->host)) {
			rc = ecryptfs_truncate(file->f_path.dentry,
					       prev_page_end_size);
			if (rc) {
				printk(KERN_ERR "%s: Error on attempt to "
				       "truncate to (higher) offset [%lld];"
				       " rc = [%d]\n", __func__,
				       prev_page_end_size, rc);
				goto out;
			}
		}
	}
	 
	if ((i_size_read(mapping->host) == prev_page_end_size)
	    && (pos != 0))
		zero_user(page, 0, PAGE_CACHE_SIZE);
out:
	return rc;
}

static int ecryptfs_write_inode_size_to_header(struct inode *ecryptfs_inode)
{
	char *file_size_virt;
	int rc;

	file_size_virt = kmalloc(sizeof(u64), GFP_KERNEL);
	if (!file_size_virt) {
		rc = -ENOMEM;
		goto out;
	}
	put_unaligned_be64(i_size_read(ecryptfs_inode), file_size_virt);
	rc = ecryptfs_write_lower(ecryptfs_inode, file_size_virt, 0,
				  sizeof(u64));
	kfree(file_size_virt);
#ifdef MY_ABC_HERE
	if (-EDQUOT == rc)
		return rc;   
#endif
	if (rc < 0)
		printk(KERN_ERR "%s: Error writing file size to header; "
		       "rc = [%d]\n", __func__, rc);
	else
		rc = 0;
out:
	return rc;
}

struct kmem_cache *ecryptfs_xattr_cache;

static int ecryptfs_write_inode_size_to_xattr(struct inode *ecryptfs_inode)
{
	ssize_t size;
	void *xattr_virt;
	struct dentry *lower_dentry =
		ecryptfs_inode_to_private(ecryptfs_inode)->lower_file->f_dentry;
	struct inode *lower_inode = lower_dentry->d_inode;
	int rc;

	if (!lower_inode->i_op->getxattr || !lower_inode->i_op->setxattr) {
		printk(KERN_WARNING
		       "No support for setting xattr in lower filesystem\n");
		rc = -ENOSYS;
		goto out;
	}
	xattr_virt = kmem_cache_alloc(ecryptfs_xattr_cache, GFP_KERNEL);
	if (!xattr_virt) {
		printk(KERN_ERR "Out of memory whilst attempting to write "
		       "inode size to xattr\n");
		rc = -ENOMEM;
		goto out;
	}
	mutex_lock(&lower_inode->i_mutex);
	size = lower_inode->i_op->getxattr(lower_dentry, ECRYPTFS_XATTR_NAME,
					   xattr_virt, PAGE_CACHE_SIZE);
	if (size < 0)
		size = 8;
	put_unaligned_be64(i_size_read(ecryptfs_inode), xattr_virt);
	rc = lower_inode->i_op->setxattr(lower_dentry, ECRYPTFS_XATTR_NAME,
					 xattr_virt, size, 0);
	mutex_unlock(&lower_inode->i_mutex);
	if (rc)
		printk(KERN_ERR "Error whilst attempting to write inode size "
		       "to lower file xattr; rc = [%d]\n", rc);
	kmem_cache_free(ecryptfs_xattr_cache, xattr_virt);
out:
	return rc;
}

int ecryptfs_write_inode_size_to_metadata(struct inode *ecryptfs_inode)
{
	struct ecryptfs_crypt_stat *crypt_stat;

	crypt_stat = &ecryptfs_inode_to_private(ecryptfs_inode)->crypt_stat;
	BUG_ON(!(crypt_stat->flags & ECRYPTFS_ENCRYPTED));
	if (crypt_stat->flags & ECRYPTFS_METADATA_IN_XATTR)
		return ecryptfs_write_inode_size_to_xattr(ecryptfs_inode);
	else
		return ecryptfs_write_inode_size_to_header(ecryptfs_inode);
}

static int ecryptfs_write_end(struct file *file,
			struct address_space *mapping,
			loff_t pos, unsigned len, unsigned copied,
			struct page *page, void *fsdata)
{
	pgoff_t index = pos >> PAGE_CACHE_SHIFT;
	unsigned from = pos & (PAGE_CACHE_SIZE - 1);
	unsigned to = from + copied;
	struct inode *ecryptfs_inode = mapping->host;
	struct ecryptfs_crypt_stat *crypt_stat =
		&ecryptfs_inode_to_private(file->f_path.dentry->d_inode)->crypt_stat;
	int rc;

	if (crypt_stat->flags & ECRYPTFS_NEW_FILE) {
		ecryptfs_printk(KERN_DEBUG, "ECRYPTFS_NEW_FILE flag set in "
			"crypt_stat at memory location [%p]\n", crypt_stat);
		crypt_stat->flags &= ~(ECRYPTFS_NEW_FILE);
	} else
		ecryptfs_printk(KERN_DEBUG, "Not a new file\n");
	ecryptfs_printk(KERN_DEBUG, "Calling fill_zeros_to_end_of_page"
			"(page w/ index = [0x%.16x], to = [%d])\n", index, to);
	if (!(crypt_stat->flags & ECRYPTFS_ENCRYPTED)) {
		rc = ecryptfs_write_lower_page_segment(ecryptfs_inode, page, 0,
						       to);
		if (!rc) {
			rc = copied;
			fsstack_copy_inode_size(ecryptfs_inode,
				ecryptfs_inode_to_lower(ecryptfs_inode));
		}
		goto out;
	}
	 
	rc = fill_zeros_to_end_of_page(page, to);
	if (rc) {
		ecryptfs_printk(KERN_WARNING, "Error attempting to fill "
			"zeros in page with index = [0x%.16x]\n", index);
		goto out;
	}
	rc = ecryptfs_encrypt_page(page);
	if (rc) {
#ifdef MY_ABC_HERE
		if (-EDQUOT != rc)
#endif
		ecryptfs_printk(KERN_WARNING, "Error encrypting page (upper "
				"index [0x%.16x])\n", index);
		goto out;
	}
	if (pos + copied > i_size_read(ecryptfs_inode)) {
		i_size_write(ecryptfs_inode, pos + copied);
		ecryptfs_printk(KERN_DEBUG, "Expanded file size to "
				"[0x%.16x]\n", i_size_read(ecryptfs_inode));
	}
	rc = ecryptfs_write_inode_size_to_metadata(ecryptfs_inode);
	if (rc)
		printk(KERN_ERR "Error writing inode size to metadata; "
		       "rc = [%d]\n", rc);
	else
		rc = copied;
out:
	unlock_page(page);
	page_cache_release(page);
	return rc;
}

static sector_t ecryptfs_bmap(struct address_space *mapping, sector_t block)
{
	int rc = 0;
	struct inode *inode;
	struct inode *lower_inode;

	inode = (struct inode *)mapping->host;
	lower_inode = ecryptfs_inode_to_lower(inode);
	if (lower_inode->i_mapping->a_ops->bmap)
		rc = lower_inode->i_mapping->a_ops->bmap(lower_inode->i_mapping,
							 block);
	return rc;
}

#if defined(MY_ABC_HERE) && defined(SYNO_OLD_RECVFILE)
 
static int ecryptfs_prepare_write(struct file *file, struct page *page,
				  unsigned from, unsigned to)
{
	loff_t prev_page_end_size;
	int rc = 0;

	if (!PageUptodate(page)) {
		struct ecryptfs_crypt_stat *crypt_stat =
			&ecryptfs_inode_to_private(
				file->f_path.dentry->d_inode)->crypt_stat;

		if (!(crypt_stat->flags & ECRYPTFS_ENCRYPTED)
		    || (crypt_stat->flags & ECRYPTFS_NEW_FILE)) {
			rc = ecryptfs_read_lower_page_segment(
				page, page->index, 0, PAGE_CACHE_SIZE,
				page->mapping->host);
			if (rc) {
				printk(KERN_ERR "%s: Error attemping to read "
				       "lower page segment; rc = [%d]\n",
				       __func__, rc);
				ClearPageUptodate(page);
				goto out;
			} else
				SetPageUptodate(page);
		} else if (crypt_stat->flags & ECRYPTFS_VIEW_AS_ENCRYPTED) {
			if (crypt_stat->flags & ECRYPTFS_METADATA_IN_XATTR) {
				rc = ecryptfs_copy_up_encrypted_with_header(
					page, crypt_stat);
				if (rc) {
					printk(KERN_ERR "%s: Error attempting "
					       "to copy the encrypted content "
					       "from the lower file whilst "
					       "inserting the metadata from "
					       "the xattr into the header; rc "
					       "= [%d]\n", __func__, rc);
					ClearPageUptodate(page);
					goto out;
				}
				SetPageUptodate(page);
			} else {
				rc = ecryptfs_read_lower_page_segment(
					page, page->index, 0, PAGE_CACHE_SIZE,
					page->mapping->host);
				if (rc) {
					printk(KERN_ERR "%s: Error reading "
					       "page; rc = [%d]\n",
					       __func__, rc);
					ClearPageUptodate(page);
					goto out;
				}
				SetPageUptodate(page);
			}
		} else {
			rc = ecryptfs_decrypt_page(page);
			if (rc) {
				printk(KERN_ERR "%s: Error decrypting page "
				       "at index [%ld]; rc = [%d]\n",
				       __func__, page->index, rc);
				ClearPageUptodate(page);
				goto out;
			}
			SetPageUptodate(page);
		}
	}
	prev_page_end_size = ((loff_t)page->index << PAGE_CACHE_SHIFT);
	 
	if (page->index != 0) {
		if (prev_page_end_size > i_size_read(page->mapping->host)) {
			rc = ecryptfs_truncate(file->f_path.dentry,
					       prev_page_end_size);
			if (rc) {
				printk(KERN_ERR "%s: Error on attempt to "
				       "truncate to (higher) offset [%lld];"
				       " rc = [%d]\n", __func__,
				       prev_page_end_size, rc);
				goto out;
			}
		}
	}
	 
	if ((i_size_read(page->mapping->host) == prev_page_end_size)
	    && (from != 0))
		zero_user(page, 0, PAGE_CACHE_SIZE);
out:
	return rc;
}

static int ecryptfs_commit_write(struct file *file, struct page *page,
				 unsigned from, unsigned to)
{
	loff_t pos;
	struct inode *ecryptfs_inode = page->mapping->host;
	struct ecryptfs_crypt_stat *crypt_stat =
		&ecryptfs_inode_to_private(file->f_path.dentry->d_inode)->crypt_stat;
	int rc;

	if (crypt_stat->flags & ECRYPTFS_NEW_FILE) {
		ecryptfs_printk(KERN_DEBUG, "ECRYPTFS_NEW_FILE flag set in "
			"crypt_stat at memory location [%p]\n", crypt_stat);
		crypt_stat->flags &= ~(ECRYPTFS_NEW_FILE);
	} else
		ecryptfs_printk(KERN_DEBUG, "Not a new file\n");
	ecryptfs_printk(KERN_DEBUG, "Calling fill_zeros_to_end_of_page"
			"(page w/ index = [0x%.16x], to = [%d])\n", page->index,
			to);
	 
	rc = fill_zeros_to_end_of_page(page, to);
	if (rc) {
		ecryptfs_printk(KERN_WARNING, "Error attempting to fill "
				"zeros in page with index = [0x%.16x]\n",
				page->index);
		goto out;
	}
	rc = ecryptfs_encrypt_page(page);
	if (rc) {
#ifdef MY_ABC_HERE
		if (-EDQUOT != rc)
#endif
		ecryptfs_printk(KERN_WARNING, "Error encrypting page (upper "
				"index [0x%.16x])\n", page->index);
		goto out;
	}
	pos = (((loff_t)page->index) << PAGE_CACHE_SHIFT) + to;
	if (pos > i_size_read(ecryptfs_inode)) {
		i_size_write(ecryptfs_inode, pos);
		ecryptfs_printk(KERN_DEBUG, "Expanded file size to "
				"[0x%.16x]\n", i_size_read(ecryptfs_inode));
	}
	rc = ecryptfs_write_inode_size_to_metadata(ecryptfs_inode);
	if (rc)
		printk(KERN_ERR "Error writing inode size to metadata; "
		       "rc = [%d]\n", rc);
out:
	return rc;
}
#endif

const struct address_space_operations ecryptfs_aops = {
	.writepage = ecryptfs_writepage,
	.readpage = ecryptfs_readpage,
#if defined(MY_ABC_HERE) && defined(SYNO_OLD_RECVFILE)
	.prepare_write = ecryptfs_prepare_write,
	.commit_write = ecryptfs_commit_write,
#endif
	.write_begin = ecryptfs_write_begin,
	.write_end = ecryptfs_write_end,
	.bmap = ecryptfs_bmap,
};
