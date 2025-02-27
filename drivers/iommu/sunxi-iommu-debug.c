/*
 * Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/iommu.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include <asm/barrier.h>
#include "sunxi-iommu.h"
#include <linux/iommu.h>
#include <asm/cacheflush.h>
#include <linux/module.h>
#include <linux/compiler.h>


#ifdef CONFIG_SUNXI_IOMMU_TESTS

#ifdef CONFIG_64BIT

#define kstrtoux kstrtou64
#define kstrtox_from_user kstrtoll_from_user
#define kstrtosize_t kstrtoul

#else

#define kstrtoux kstrtou32
#define kstrtox_from_user kstrtoint_from_user
#define kstrtosize_t kstrtouint

#endif

#define ION_KERNEL_USER_ERR(str)	pr_err("%s failed!", #str)
struct ion_facade {
	struct ion_client *client;
	struct ion_handle *handle;
	dma_addr_t dma_address;
	void *virtual_address;
	size_t address_length;
	struct sg_table *sg_table;
};

static struct dentry *iommu_debugfs_top;

static LIST_HEAD(iommu_debug_devices);
static struct dentry *debugfs_tests_dir;

struct iommu_debug_device {
	struct device *dev;
	struct iommu_domain *domain;
	u64 iova;
	u64 phys;
	size_t len;
	struct list_head list;
};

static const char * const _size_to_string(unsigned long size)
{
	switch (size) {
	case SZ_4K:
		return "4K";
	case SZ_8K:
		return "8K";
	case SZ_16K:
		return "16K";
	case SZ_64K:
		return "64K";
	case SZ_2M:
		return "2M";
	case SZ_1M:
		return "1M";
	case SZ_1M * 4:
		return "4M";
	case SZ_1M * 8:
		return "8M";
	case SZ_1M * 16:
		return "16M";
	case SZ_1M * 32:
		return "32M";
	}
	return "unknown size, please add to _size_to_string";
}

static int iommu_debug_profiling_fast_dma_api_show(struct seq_file *s,
						 void *ignored)
{
	int i, experiment;
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	u64 map_elapsed_ns[10], unmap_elapsed_ns[10];
	dma_addr_t dma_addr;
	void *virt;
	const char * const extra_labels[] = {
		"not coherent",
		"coherent",
	};
	unsigned long extra_attrs[] = {
		0,
		DMA_ATTR_SKIP_CPU_SYNC,
	};

	virt = kmalloc(1518, GFP_KERNEL);
	if (!virt)
		goto out;

	for (experiment = 0; experiment < 2; ++experiment) {
		size_t map_avg = 0, unmap_avg = 0;

		for (i = 0; i < 10; ++i) {
			struct timespec tbefore, tafter, diff;
			u64 ns;

			getnstimeofday(&tbefore);
			dma_addr = dma_map_single_attrs(
				dev, virt, SZ_4K, DMA_TO_DEVICE,
				extra_attrs[experiment]);
			getnstimeofday(&tafter);
			diff.tv_sec = tafter.tv_sec - tbefore.tv_sec;
			diff.tv_nsec = tafter.tv_nsec - tbefore.tv_nsec;
			ns = timespec_to_ns(&diff);
			if (dma_mapping_error(dev, dma_addr)) {
				seq_puts(s, "dma_map_single failed\n");
				goto out_disable_config_clocks;
			}
			map_elapsed_ns[i] = ns;
			getnstimeofday(&tbefore);
			dma_unmap_single_attrs(
				dev, dma_addr, SZ_4K, DMA_TO_DEVICE,
				extra_attrs[experiment]);
			getnstimeofday(&tafter);
			diff.tv_sec = tafter.tv_sec - tbefore.tv_sec;
			diff.tv_nsec = tafter.tv_nsec - tbefore.tv_nsec;
			ns = timespec_to_ns(&diff);
			unmap_elapsed_ns[i] = ns;
		}
		seq_printf(s, "%13s %24s (ns): [", extra_labels[experiment],
			   "dma_map_single_attrs");
		for (i = 0; i < 10; ++i) {
			map_avg += map_elapsed_ns[i];
			seq_printf(s, "%5llu%s", map_elapsed_ns[i],
				   i < 9 ? ", " : "");
		}
		map_avg /= 10;
		seq_printf(s, "] (avg: %zu)\n", map_avg);

		seq_printf(s, "%13s %24s (ns): [", extra_labels[experiment],
			   "dma_unmap_single_attrs");
		for (i = 0; i < 10; ++i) {
			unmap_avg += unmap_elapsed_ns[i];
			seq_printf(s, "%5llu%s", unmap_elapsed_ns[i],
				   i < 9 ? ", " : "");
		}
		unmap_avg /= 10;
		seq_printf(s, "] (avg: %zu)\n", unmap_avg);
	}

out_disable_config_clocks:
	kfree(virt);
out:
	return 0;
}

static int iommu_debug_profiling_fast_dma_api_open(struct inode *inode,
						 struct file *file)
{
	return single_open(file, iommu_debug_profiling_fast_dma_api_show,
			   inode->i_private);
}

static const struct file_operations iommu_debug_profiling_fast_dma_api_fops = {
	.open	 = iommu_debug_profiling_fast_dma_api_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};


/* Creates a fresh fast mapping and applies @fn to it */
static int __apply_to_new_mapping(struct seq_file *s,
				    int (*fn)(struct device *dev,
					      struct seq_file *s,
					      struct iommu_domain *domain,
					      void *priv),
				    void *priv)
{
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	int ret = -EINVAL;

	ret = fn(dev, s, global_iommu_domain, priv);
	return ret;
}

#if 0
static int __tlb_stress_sweep(struct device *dev, struct seq_file *s)
{
	int i, ret = 0;
	unsigned long iova;
	const unsigned long max = SZ_1G * 4UL;
	void *virt;
	phys_addr_t phys;
	dma_addr_t dma_addr;

	/*
	 * we'll be doing 4K and 8K mappings.  Need to own an entire 8K
	 * chunk that we can work with.
	 */
	virt = (void *)__get_free_pages(GFP_KERNEL, get_order(SZ_8K));
	phys = virt_to_phys(virt);

	/* fill the whole 4GB space */
	for (iova = 0, i = 0; iova < max; iova += SZ_8K, ++i) {
		dma_addr = dma_map_single(dev, virt, SZ_8K, DMA_TO_DEVICE);
		if (dma_addr == 0) {
			dev_err(dev, "Failed map on iter %d\n", i);
			ret = -EINVAL;
			goto out;
		}
	}

	/*
	 * free up 4K at the very beginning, then leave one 4K mapping,
	 * then free up 8K.  This will result in the next 8K map to skip
	 * over the 4K hole and take the 8K one.
	 */
	dma_unmap_single(dev, 0, SZ_4K, DMA_TO_DEVICE);
	dma_unmap_single(dev, SZ_8K, SZ_4K, DMA_TO_DEVICE);
	dma_unmap_single(dev, SZ_8K + SZ_4K, SZ_4K, DMA_TO_DEVICE);

	/* remap 8K */
	dma_addr = dma_map_single(dev, virt, SZ_8K, DMA_TO_DEVICE);

	/*
	 * now remap 4K.  We should get the first 4K chunk that was skipped
	 * over during the previous 8K map.  If we missed a TLB invalidate
	 * at that point this should explode.
	 */
	dma_addr = dma_map_single(dev, virt, SZ_4K, DMA_TO_DEVICE);


	/* we're all full again. unmap everything. */
	for (dma_addr = 0; dma_addr < max; dma_addr += SZ_8K)
		dma_unmap_single(dev, dma_addr, SZ_8K, DMA_TO_DEVICE);

out:
	free_pages((unsigned long)virt, get_order(SZ_8K));
	return ret;
}
#else
static int __tlb_stress_sweep(struct device *dev, struct seq_file *s)
{
	return 0;
}
#endif

struct fib_state {
	unsigned long cur;
	unsigned long prev;
};

static __maybe_unused void fib_init(struct fib_state *f)
{
	f->cur = f->prev = 1;
}

static __maybe_unused unsigned long get_next_fib(struct fib_state *f)
{
	int next = f->cur + f->prev;

	f->prev = f->cur;
	f->cur = next;
	return next;
}

/*
 * Not actually random.  Just testing the fibs (and max - the fibs).
 */
#if 0
static int __rand_va_sweep(struct device *dev, struct seq_file *s,
			   const size_t size)
{
	u64 iova;
	const unsigned long max = SZ_1G * 4UL;
	int i, remapped, unmapped, ret = 0;
	void *virt;
	dma_addr_t dma_addr, dma_addr2;
	struct fib_state fib;

	virt = (void *)__get_free_pages(GFP_KERNEL, get_order(size));
	if (!virt) {
		if (size > SZ_8K) {
			dev_err(dev,
				"Failed to allocate %s of memory, which is a lot. Skipping test for this size\n",
				_size_to_string(size));
			return 0;
		}
		return -ENOMEM;
	}

	/* fill the whole 4GB space */
	for (iova = 0, i = 0; iova < max; iova += size, ++i) {
		dma_addr = dma_map_single(dev, virt, size, DMA_TO_DEVICE);
		if (dma_addr == 0) {
			dev_err(dev, "Failed map on iter %d\n", i);
			ret = -EINVAL;
			goto out;
		}
	}

	/* now unmap "random" iovas */
	unmapped = 0;
	fib_init(&fib);
	for (iova = get_next_fib(&fib) * size;
	     iova < max - size;
	     iova = get_next_fib(&fib) * size) {
		dma_addr = iova;
		dma_addr2 = max - size - iova;
		if (dma_addr == dma_addr2) {
			WARN(1,
			"%s test needs update! The random number sequence is folding in on itself and should be changed.\n",
			__func__);
			return -EINVAL;
		}
		dma_unmap_single(dev, dma_addr, size, DMA_TO_DEVICE);
		dma_unmap_single(dev, dma_addr2, size, DMA_TO_DEVICE);
		unmapped += 2;
	}

	/* and map until everything fills back up */
	for (remapped = 0;; ++remapped) {
		dma_addr = dma_map_single(dev, virt, size, DMA_TO_DEVICE);
		if (dma_addr == 0)
			break;
	}

	if (unmapped != remapped) {
		dev_err(dev,
			"Unexpected random remap count! Unmapped %d but remapped %d\n",
			unmapped, remapped);
		ret = -EINVAL;
	}

	for (dma_addr = 0; dma_addr < max; dma_addr += size)
		dma_unmap_single(dev, dma_addr, size, DMA_TO_DEVICE);

out:
	free_pages((unsigned long)virt, get_order(size));
	return ret;
}
#else
static int __rand_va_sweep(struct device *dev, struct seq_file *s,
			   const size_t size)
{
	return 0;
}
#endif

struct dma_addr_list {
	dma_addr_t addr;
	void *next;
};

static int __full_va_sweep(struct device *dev, struct seq_file *s,
			   const size_t size, struct iommu_domain *domain)
{
	unsigned long iova;
	void *virt;
	phys_addr_t phys;
	const unsigned long max = SZ_128M;
	struct dma_addr_list *phead, *p, *ptmp;

	phead = kmalloc(sizeof(struct dma_addr_list), GFP_KERNEL);
	memset(phead, 0, sizeof(struct dma_addr_list));
	virt = (void *)__get_free_pages(GFP_KERNEL, get_order(size));
	if (!virt) {
		if (size > SZ_8K) {
			dev_err(dev,
				"Failed to allocate %s of memory, which is a lot. Skipping test for this size\n",
				_size_to_string(size));
			return 0;
		}
		return -ENOMEM;
	}
	phys = virt_to_phys(virt);

	for (p = phead, iova = 0; iova < max; iova += size, p = p->next) {
		p->addr = dma_map_single(dev, virt, size, DMA_TO_DEVICE);
		p->next = kmalloc(sizeof(struct dma_addr_list), GFP_KERNEL);
		memset(p->next, 0, sizeof(struct dma_addr_list));
		if (p->addr == 0 || p->addr == DMA_MAPPING_ERROR) {
			dev_err(dev, "Failed to map dma, out of iova space\n");
			return -ENOMEM;
		}
	}

	for (p = phead; p->addr != 0;) {
		dma_unmap_single(dev, p->addr, size, DMA_TO_DEVICE);
		ptmp = p;
		p = p->next;
		kfree(ptmp);
	}
	kfree(p);
	free_pages((unsigned long)virt, get_order(size));
	return 0;
}

#define ds_printf(d, s, fmt, ...) ({				\
			dev_err(d, fmt, ##__VA_ARGS__);		\
			seq_printf(s, fmt, ##__VA_ARGS__);	\
		})

static int __functional_dma_api_va_test(struct device *dev, struct seq_file *s,
				     struct iommu_domain *domain, void *priv)
{
	int i, j;
	int ret = 0;
	size_t *sz, *sizes = priv;

	for (j = 0; j < 1; ++j) {
		for (sz = sizes; *sz; ++sz) {
			for (i = 0; i < 2; ++i) {
				ds_printf(dev, s, "Full VA sweep @%s %d",
					       _size_to_string(*sz), i);
				if (__full_va_sweep(dev, s, *sz, domain)) {
					ds_printf(dev, s, "  -> FAILED\n");
					ret = -EINVAL;
					goto out;
				} else
					ds_printf(dev, s, "  -> SUCCEEDED\n");
			}
		}
	}

	ds_printf(dev, s, "bonus map:");
	if (__full_va_sweep(dev, s, SZ_4K, domain)) {
		ds_printf(dev, s, "  -> FAILED\n");
		ret = -EINVAL;
		goto out;
	} else
		ds_printf(dev, s, "  -> SUCCEEDED\n");

	for (sz = sizes; *sz; ++sz) {
		for (i = 0; i < 2; ++i) {
			ds_printf(dev, s, "Rand VA sweep @%s %d",
				   _size_to_string(*sz), i);
			if (__rand_va_sweep(dev, s, *sz)) {
				ds_printf(dev, s, "  -> FAILED\n");
				ret = -EINVAL;
				goto out;
			} else
				ds_printf(dev, s, "  -> SUCCEEDED\n");
		}
	}

	ds_printf(dev, s, "TLB stress sweep");
	if (__tlb_stress_sweep(dev, s)) {
		ds_printf(dev, s, "  -> FAILED\n");
		ret = -EINVAL;
		goto out;
	} else
		ds_printf(dev, s, "  -> SUCCEEDED\n");

	ds_printf(dev, s, "second bonus map:");
	if (__full_va_sweep(dev, s, SZ_4K, domain)) {
		ds_printf(dev, s, "  -> FAILED\n");
		ret = -EINVAL;
		goto out;
	} else
		ds_printf(dev, s, "  -> SUCCEEDED\n");
out:
	return ret;
}

/*iova alloc strategy stress test*/
static int iommu_iova_alloc_strategy_stress_show(struct seq_file *s,
						    void *ignored)
{
	size_t sizes[] = {SZ_4K, SZ_8K, SZ_16K, SZ_64K, 0};
	int ret = 0;

	ret = __apply_to_new_mapping(s, __functional_dma_api_va_test, sizes);
	return ret;
}

static int iommu_iova_alloc_strategy_stress_open(struct inode *inode,
						    struct file *file)
{
	return single_open(file, iommu_iova_alloc_strategy_stress_show,
			   inode->i_private);
}

static const struct file_operations iommu_iova_alloc_strategy_stress_fops = {
	.open	 = iommu_iova_alloc_strategy_stress_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};


static int __functional_dma_api_alloc_test(struct device *dev,
					   struct seq_file *s,
					   struct iommu_domain *domain,
					   void *ignored)
{
	size_t size = SZ_1K * 742;
	int ret = 0;
	u8 *data;
	dma_addr_t iova;

	/* Make sure we can allocate and use a buffer */
	ds_printf(dev, s, "Allocating coherent buffer");
	data = dma_alloc_coherent(dev, size, &iova, GFP_KERNEL);
	if (!data) {
		ds_printf(dev, s, "  -> FAILED\n");
		ret = -EINVAL;
	} else {
		int i;

		ds_printf(dev, s, "  -> SUCCEEDED\n");
		ds_printf(dev, s, "Using coherent buffer");
		for (i = 0; i < 742; ++i) {
			int ind = SZ_1K * i;
			u8 *p = data + ind;
			u8 val = i % 255;

			memset(data, 0xa5, size);
			*p = val;
			(*p)++;
			if ((*p) != val + 1) {
				ds_printf(dev, s,
					  "  -> FAILED on iter %d since %d != %d\n",
					  i, *p, val + 1);
				ret = -EINVAL;
				break;
			}
		}
		if (!ret)
			ds_printf(dev, s, "  -> SUCCEEDED\n");
		dma_free_coherent(dev, size, data, iova);
	}

	return ret;
}

/*iommu kernel virtual addr read/write*/
static int iommu_kvirtual_addr_rdwr_show(struct seq_file *s,
						   void *ignored)
{
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	int ret = -EINVAL;

	ret = __functional_dma_api_alloc_test(dev, s,
				global_iommu_domain, NULL);
	return ret;
}

static int iommu_kvirtual_addr_rdwr_open(struct inode *inode,
						   struct file *file)
{
	return single_open(file, iommu_kvirtual_addr_rdwr_show,
			   inode->i_private);
}

static const struct file_operations iommu_kvirtul_addr_rdwr_fops = {
	.open	 = iommu_kvirtual_addr_rdwr_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int __functional_dma_api_ion_test(struct device *dev,
					   struct seq_file *s,
					   struct iommu_domain *domain,
					   void *ignored)
{
	return 0;
}

/*iommu ion interface test*/
static int iommu_ion_interface_test_show(struct seq_file *s,
						   void *ignored)
{
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	int ret = -EINVAL;

	ret = __functional_dma_api_ion_test(dev, s,
				global_iommu_domain, NULL);
	return ret;
}

static int iommu_ion_interface_test_open(struct inode *inode,
						   struct file *file)
{
	return single_open(file, iommu_ion_interface_test_show,
			   inode->i_private);
}

static const struct file_operations iommu_ion_interface_test_fops = {
	.open	 = iommu_ion_interface_test_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int __functional_dma_api_iova_test(struct device *dev,
					   struct seq_file *s,
					   struct iommu_domain *domain,
					   void *ignored)
{
	size_t size = SZ_4K * 1024;
	int ret = 0;
	u32 *data;
	dma_addr_t iova;

	sunxi_set_debug_mode();

	/* Make sure we can allocate and use a buffer */
	ds_printf(dev, s, "Allocating coherent iova buffer");
	data = dma_alloc_coherent(dev, size, &iova, GFP_KERNEL);
	if (!data) {
		ds_printf(dev, s, "  -> FAILED\n");
		ret = -EINVAL;
	} else {
		int i;

		ds_printf(dev, s, "  -> SUCCEEDED\n");
		ds_printf(dev, s, "Using coherent buffer");
		for (i = 0; i < 1024; ++i) {
			int ind = (SZ_4K * i) / sizeof(u32);
			u32 *p = data + ind;
			u32 *p1 = (u32 *)iova + ind;
			u32 read_data;

			memset(data, 0xa5, size);
			*p = 0x5a5a5a5a;
			/**
			 * make sure that *p is written before
			 * the write operation of the debug mode of iommu
			 */
			wmb();
			sunxi_iova_test_write((dma_addr_t)p1, 0xdead);
			/**
			 * do the write operation of debug mode of iommu
			 * in order
			 */
			rmb();
			if ((*p) != 0xdead) {
				ds_printf(dev, s,
				 "-> FAILED on iova0 iter %x  %x\n", i, *p);
				ret = -EINVAL;
				goto out;
			}

			*p = 0xffffaaaa;
			/**
			 * make sure that *p is written before
			 * the read operation of the debug mode of iommu
			 */
			wmb();
			read_data = sunxi_iova_test_read((dma_addr_t)p1);
			if (read_data != 0xffffaaaa) {
				ds_printf(dev, s,
					"-> FAILED on iova1 iter %x  %x\n",
					i, read_data);
				ret = -EINVAL;
				goto out;
			}

		}
		if (!ret)
			ds_printf(dev, s, "  -> SUCCEEDED\n");
	}
out:
	dma_free_coherent(dev, size, data, iova);
	sunxi_set_prefetch_mode();
	return ret;
}

/*iommu test use debug interface*/
static int iommu_vir_devio_addr_rdwr_show(struct seq_file *s,
						    void *ignored)
{
	int ret = 0;

	ret = __apply_to_new_mapping(s, __functional_dma_api_iova_test, NULL);
	if (ret) {
		pr_err("the first iova test failed\n");
		return ret;
	}
	ret = 0;
	ret = __apply_to_new_mapping(s, __functional_dma_api_iova_test, NULL);
	if (ret) {
		pr_err("the second iova test failed\n");
		return ret;
	}
	ret = 0;
	ret = __apply_to_new_mapping(s, __functional_dma_api_iova_test, NULL);
	if (ret) {
		pr_err("the third iova test failed\n");
		return ret;
	}
	return 0;
}

static int iommu_vir_devio_addr_rdwr_open(struct inode *inode,
						    struct file *file)
{
	return single_open(file, iommu_vir_devio_addr_rdwr_show,
			   inode->i_private);
}

static const struct file_operations iommu_vir_devio_addr_rdwr_fops = {
	.open	 = iommu_vir_devio_addr_rdwr_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};


static int __functional_dma_api_basic_test(struct device *dev,
					   struct seq_file *s,
					   struct iommu_domain *domain,
					   void *ignored)
{
	size_t size = 1518;
	int i, j, ret = 0;
	u8 *data;
	dma_addr_t iova;
	phys_addr_t pa, pa2;

	ds_printf(dev, s, "Basic DMA API test");
	/* Make sure we can allocate and use a buffer */
	for (i = 0; i < 1000; ++i) {
		data = kmalloc(size, GFP_KERNEL);
		if (!data) {
			ds_printf(dev, s, "  -> FAILED\n");
			ret = -EINVAL;
			goto out;
		}
		memset(data, 0xa5, size);
		iova = dma_map_single(dev, data, size, DMA_TO_DEVICE);
		pa = iommu_iova_to_phys(domain, iova);
		pa2 = virt_to_phys(data);
		if (pa != pa2) {
			dev_err(dev,
				"iova_to_phys doesn't match virt_to_phys: %pa != %pa\n",
				&pa, &pa2);
			ret = -EINVAL;
			kfree(data);
			goto out;
		}
		dma_unmap_single(dev, iova, size, DMA_TO_DEVICE);
		for (j = 0; j < size; ++j) {
			if (data[j] != 0xa5) {
				dev_err(dev, "data[%d] != 0xa5\n", data[j]);
				ret = -EINVAL;
				kfree(data);
				goto out;
			}
		}
		kfree(data);
	}

out:
	if (ret)
		ds_printf(dev, s, "  -> FAILED\n");
	else
		ds_printf(dev, s, "  -> SUCCEEDED\n");

	return ret;
}

/*iommu basic test*/
static int iommu_debug_basic_test_show(struct seq_file *s,
						   void *ignored)
{
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	int ret = -EINVAL;

	ret = __functional_dma_api_basic_test(dev, s,
				global_iommu_domain, NULL);
	return ret;
}

static int iommu_debug_basic_test_open(struct inode *inode,
						   struct file *file)
{
	return single_open(file, iommu_debug_basic_test_show,
			   inode->i_private);
}

static const struct file_operations iommu_debug_basic_test_fops = {
	.open	 = iommu_debug_basic_test_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};


/*
 * The following will only work for drivers that implement the generic
 * device tree bindings described in
 * Documentation/devicetree/bindings/iommu/iommu.txt
 */
static int snarf_iommu_devices(struct device *dev, const char *name)
{
	struct iommu_debug_device *ddev;
	struct dentry *dir;

	if (IS_ERR_OR_NULL(dev))
		return -EINVAL;

	ddev = kzalloc(sizeof(*ddev), GFP_KERNEL);
	if (!ddev)
		return -ENODEV;
	ddev->dev = dev;
	ddev->domain = global_iommu_domain;
	dir = debugfs_create_dir(name, debugfs_tests_dir);
	if (!dir) {
		pr_err("Couldn't create iommu/devices/%s debugfs dir\n",
		       name);
		goto err;
	}

	if (!debugfs_create_file("profiling_fast_dma_api", 0400, dir, ddev,
				 &iommu_debug_profiling_fast_dma_api_fops)) {
		pr_err("Couldn't create iommu/devices/%s/profiling_fast_dma_api debugfs file\n",
		       name);
		goto err_rmdir;
	}

	if (!debugfs_create_file("iommu_basic_test", 0400, dir, ddev,
				 &iommu_debug_basic_test_fops)) {
		pr_err("Couldn't create iommu/devices/%s/iommu_basic_test debugfs file\n",
		       name);
		goto err_rmdir;
	}

	if (!debugfs_create_file("ion_interface_test", 0400, dir, ddev,
				 &iommu_ion_interface_test_fops)) {
		pr_err("Couldn't create iommu/devices/%s/ion_interface_test debugfs file\n",
		       name);
		goto err_rmdir;
	}

	if (!debugfs_create_file("iova_alloc_strategy_stress_test",
		0200, dir, ddev,
		&iommu_iova_alloc_strategy_stress_fops)) {
		pr_err("Couldn't create iommu/devices/%s/iova_alloc_strategy_stress_test debugfs file\n",
		       name);
		goto err_rmdir;
	}

	if (!debugfs_create_file("kvirtual_addr_rdwr_test", 0200, dir, ddev,
				 &iommu_kvirtul_addr_rdwr_fops)) {
		pr_err("Couldn't create iommu/devices/%s/kvirtual_addr_rdwr_test debugfs file\n",
		       name);
		goto err_rmdir;
	}

	if (!debugfs_create_file("vir_devio_addr_rdwr_test", 0200, dir, ddev,
				 &iommu_vir_devio_addr_rdwr_fops)) {
		pr_err("Couldn't create iommu/devices/%s/vir_devio_addr_rdwr_test debugfs file\n",
		       name);
		goto err_rmdir;
	}

	list_add(&ddev->list, &iommu_debug_devices);
	return 0;

err_rmdir:
	debugfs_remove_recursive(dir);
err:
	kfree(ddev);
	return 0;
}

static int pass_iommu_devices(struct device *dev, void *ignored)
{
	if (!of_find_property(dev->of_node, "iommus", NULL))
		return 0;

	return snarf_iommu_devices(dev, dev_name(dev));
}

static int iommu_debug_populate_devices(void)
{
	return bus_for_each_dev(&platform_bus_type, NULL, NULL,
			pass_iommu_devices);
}

static int iommu_debug_init_tests(void)
{
	iommu_debugfs_top = debugfs_create_dir("iommu", NULL);
	if (!iommu_debugfs_top) {
		pr_err("Couldn't create iommu debugfs directory\n");
		return -ENODEV;
	}
	debugfs_tests_dir = debugfs_create_dir("tests",
					       iommu_debugfs_top);
	if (!debugfs_tests_dir) {
		pr_err("Couldn't create iommu/tests debugfs directory\n");
		return -ENODEV;
	}

	return iommu_debug_populate_devices();
}

static void iommu_debug_destroy_tests(void)
{
	debugfs_remove_recursive(debugfs_tests_dir);
}
#else
static inline int iommu_debug_init_tests(void) { return 0; }
static inline void iommu_debug_destroy_tests(void) { }
#endif

static int __init iommu_debug_init(void)
{
	if (iommu_debug_init_tests())
		return -ENODEV;

	return 0;
}

static void __exit iommu_debug_exit(void)
{
	iommu_debug_destroy_tests();
}

module_init(iommu_debug_init);
module_exit(iommu_debug_exit);

MODULE_LICENSE("GPL v2");
