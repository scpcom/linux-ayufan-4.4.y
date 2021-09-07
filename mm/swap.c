 
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>
#include <linux/swap.h>
#include <linux/mman.h>
#include <linux/pagemap.h>
#include <linux/pagevec.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm_inline.h>
#include <linux/buffer_head.h>	 
#include <linux/percpu_counter.h>
#include <linux/percpu.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/backing-dev.h>
#include <linux/memcontrol.h>

#include "internal.h"

int page_cluster;

static DEFINE_PER_CPU(struct pagevec[NR_LRU_LISTS], lru_add_pvecs);
static DEFINE_PER_CPU(struct pagevec, lru_rotate_pvecs);

static void __page_cache_release(struct page *page)
{
	if (PageLRU(page)) {
		unsigned long flags;
		struct zone *zone = page_zone(page);

		spin_lock_irqsave(&zone->lru_lock, flags);
		VM_BUG_ON(!PageLRU(page));
		__ClearPageLRU(page);
		del_page_from_lru(zone, page);
		spin_unlock_irqrestore(&zone->lru_lock, flags);
	}
	free_hot_page(page);
}

static void put_compound_page(struct page *page)
{
	page = compound_head(page);
	if (put_page_testzero(page)) {
		compound_page_dtor *dtor;

		dtor = get_compound_page_dtor(page);
		(*dtor)(page);
	}
}

void put_page(struct page *page)
{
#ifndef CONFIG_SYNO_PLX_PORTING
	if (unlikely(PageCompound(page)))
		put_compound_page(page);
	else if (put_page_testzero(page))
 		__page_cache_release(page);

#else
#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
	if (PageIncoherentSendfile(page)) {
		fast_put(page);
	} else {
#endif  
		if (unlikely(PageCompound(page))) {
		put_compound_page(page);
		} else if (put_page_testzero(page)) {
		__page_cache_release(page);
		}
#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
	}
#endif  
#endif
}
EXPORT_SYMBOL(put_page);

void put_pages_list(struct list_head *pages)
{
	while (!list_empty(pages)) {
		struct page *victim;

		victim = list_entry(pages->prev, struct page, lru);
		list_del(&victim->lru);
		page_cache_release(victim);
	}
}
EXPORT_SYMBOL(put_pages_list);

static void pagevec_move_tail(struct pagevec *pvec)
{
	int i;
	int pgmoved = 0;
	struct zone *zone = NULL;

	for (i = 0; i < pagevec_count(pvec); i++) {
		struct page *page = pvec->pages[i];
		struct zone *pagezone = page_zone(page);

		if (pagezone != zone) {
			if (zone)
				spin_unlock(&zone->lru_lock);
			zone = pagezone;
			spin_lock(&zone->lru_lock);
		}
		if (PageLRU(page) && !PageActive(page) && !PageUnevictable(page)) {
			int lru = page_lru_base_type(page);
			list_move_tail(&page->lru, &zone->lru[lru].list);
			pgmoved++;
		}
	}
	if (zone)
		spin_unlock(&zone->lru_lock);
	__count_vm_events(PGROTATED, pgmoved);
	release_pages(pvec->pages, pvec->nr, pvec->cold);
	pagevec_reinit(pvec);
}

void  rotate_reclaimable_page(struct page *page)
{
	if (!PageLocked(page) && !PageDirty(page) && !PageActive(page) &&
	    !PageUnevictable(page) && PageLRU(page)) {
		struct pagevec *pvec;
		unsigned long flags;

		page_cache_get(page);
		local_irq_save(flags);
		pvec = &__get_cpu_var(lru_rotate_pvecs);
		if (!pagevec_add(pvec, page))
			pagevec_move_tail(pvec);
		local_irq_restore(flags);
	}
}

static void update_page_reclaim_stat(struct zone *zone, struct page *page,
				     int file, int rotated)
{
	struct zone_reclaim_stat *reclaim_stat = &zone->reclaim_stat;
	struct zone_reclaim_stat *memcg_reclaim_stat;

	memcg_reclaim_stat = mem_cgroup_get_reclaim_stat_from_page(page);

	reclaim_stat->recent_scanned[file]++;
	if (rotated)
		reclaim_stat->recent_rotated[file]++;

	if (!memcg_reclaim_stat)
		return;

	memcg_reclaim_stat->recent_scanned[file]++;
	if (rotated)
		memcg_reclaim_stat->recent_rotated[file]++;
}

void activate_page(struct page *page)
{
	struct zone *zone = page_zone(page);

	spin_lock_irq(&zone->lru_lock);
	if (PageLRU(page) && !PageActive(page) && !PageUnevictable(page)) {
		int file = page_is_file_cache(page);
		int lru = page_lru_base_type(page);
		del_page_from_lru_list(zone, page, lru);

		SetPageActive(page);
		lru += LRU_ACTIVE;
		add_page_to_lru_list(zone, page, lru);
		__count_vm_event(PGACTIVATE);

		update_page_reclaim_stat(zone, page, file, 1);
	}
	spin_unlock_irq(&zone->lru_lock);
}

void mark_page_accessed(struct page *page)
{
	if (!PageActive(page) && !PageUnevictable(page) &&
			PageReferenced(page) && PageLRU(page)) {
		activate_page(page);
		ClearPageReferenced(page);
	} else if (!PageReferenced(page)) {
		SetPageReferenced(page);
	}
}

EXPORT_SYMBOL(mark_page_accessed);

void __lru_cache_add(struct page *page, enum lru_list lru)
{
	struct pagevec *pvec = &get_cpu_var(lru_add_pvecs)[lru];

	page_cache_get(page);
	if (!pagevec_add(pvec, page))
		____pagevec_lru_add(pvec, lru);
	put_cpu_var(lru_add_pvecs);
}

void lru_cache_add_lru(struct page *page, enum lru_list lru)
{
	if (PageActive(page)) {
		VM_BUG_ON(PageUnevictable(page));
		ClearPageActive(page);
	} else if (PageUnevictable(page)) {
		VM_BUG_ON(PageActive(page));
		ClearPageUnevictable(page);
	}

	VM_BUG_ON(PageLRU(page) || PageActive(page) || PageUnevictable(page));
	__lru_cache_add(page, lru);
}

void add_page_to_unevictable_list(struct page *page)
{
	struct zone *zone = page_zone(page);

	spin_lock_irq(&zone->lru_lock);
	SetPageUnevictable(page);
	SetPageLRU(page);
	add_page_to_lru_list(zone, page, LRU_UNEVICTABLE);
	spin_unlock_irq(&zone->lru_lock);
}

static void drain_cpu_pagevecs(int cpu)
{
	struct pagevec *pvecs = per_cpu(lru_add_pvecs, cpu);
	struct pagevec *pvec;
	int lru;

	for_each_lru(lru) {
		pvec = &pvecs[lru - LRU_BASE];
		if (pagevec_count(pvec))
			____pagevec_lru_add(pvec, lru);
	}

	pvec = &per_cpu(lru_rotate_pvecs, cpu);
	if (pagevec_count(pvec)) {
		unsigned long flags;

		local_irq_save(flags);
		pagevec_move_tail(pvec);
		local_irq_restore(flags);
	}
}

void lru_add_drain(void)
{
	drain_cpu_pagevecs(get_cpu());
	put_cpu();
}

static void lru_add_drain_per_cpu(struct work_struct *dummy)
{
	lru_add_drain();
}

int lru_add_drain_all(void)
{
	return schedule_on_each_cpu(lru_add_drain_per_cpu);
}

void release_pages(struct page **pages, int nr, int cold)
{
	int i;
	struct pagevec pages_to_free;
	struct zone *zone = NULL;
	unsigned long uninitialized_var(flags);

	pagevec_init(&pages_to_free, cold);
	for (i = 0; i < nr; i++) {
		struct page *page = pages[i];

#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
		WARN_ON(PageIncoherentSendfile(page));
#endif  
		if (unlikely(PageCompound(page))) {
			if (zone) {
				spin_unlock_irqrestore(&zone->lru_lock, flags);
				zone = NULL;
			}
			put_compound_page(page);
			continue;
		}

		if (!put_page_testzero(page))
			continue;

		if (PageLRU(page)) {
			struct zone *pagezone = page_zone(page);

			if (pagezone != zone) {
				if (zone)
					spin_unlock_irqrestore(&zone->lru_lock,
									flags);
				zone = pagezone;
				spin_lock_irqsave(&zone->lru_lock, flags);
			}
			VM_BUG_ON(!PageLRU(page));
			__ClearPageLRU(page);
			del_page_from_lru(zone, page);
		}

		if (!pagevec_add(&pages_to_free, page)) {
			if (zone) {
				spin_unlock_irqrestore(&zone->lru_lock, flags);
				zone = NULL;
			}
			__pagevec_free(&pages_to_free);
			pagevec_reinit(&pages_to_free);
  		}
	}
	if (zone)
		spin_unlock_irqrestore(&zone->lru_lock, flags);

	pagevec_free(&pages_to_free);
}

void __pagevec_release(struct pagevec *pvec)
{
	lru_add_drain();
	release_pages(pvec->pages, pagevec_count(pvec), pvec->cold);
	pagevec_reinit(pvec);
}

EXPORT_SYMBOL(__pagevec_release);

void ____pagevec_lru_add(struct pagevec *pvec, enum lru_list lru)
{
	int i;
	struct zone *zone = NULL;

	VM_BUG_ON(is_unevictable_lru(lru));

	for (i = 0; i < pagevec_count(pvec); i++) {
		struct page *page = pvec->pages[i];
		struct zone *pagezone = page_zone(page);
		int file;
		int active;

		if (pagezone != zone) {
			if (zone)
				spin_unlock_irq(&zone->lru_lock);
			zone = pagezone;
			spin_lock_irq(&zone->lru_lock);
		}
		VM_BUG_ON(PageActive(page));
		VM_BUG_ON(PageUnevictable(page));
		VM_BUG_ON(PageLRU(page));
		SetPageLRU(page);
		active = is_active_lru(lru);
		file = is_file_lru(lru);
		if (active)
			SetPageActive(page);
		update_page_reclaim_stat(zone, page, file, active);
		add_page_to_lru_list(zone, page, lru);
	}
	if (zone)
		spin_unlock_irq(&zone->lru_lock);
	release_pages(pvec->pages, pvec->nr, pvec->cold);
	pagevec_reinit(pvec);
}

EXPORT_SYMBOL(____pagevec_lru_add);

void pagevec_strip(struct pagevec *pvec)
{
	int i;

	for (i = 0; i < pagevec_count(pvec); i++) {
		struct page *page = pvec->pages[i];

		if (page_has_private(page) && trylock_page(page)) {
			if (page_has_private(page))
				try_to_release_page(page, 0);
			unlock_page(page);
		}
	}
}

unsigned pagevec_lookup(struct pagevec *pvec, struct address_space *mapping,
		pgoff_t start, unsigned nr_pages)
{
	pvec->nr = find_get_pages(mapping, start, nr_pages, pvec->pages);
	return pagevec_count(pvec);
}

EXPORT_SYMBOL(pagevec_lookup);

unsigned pagevec_lookup_tag(struct pagevec *pvec, struct address_space *mapping,
		pgoff_t *index, int tag, unsigned nr_pages)
{
	pvec->nr = find_get_pages_tag(mapping, index, tag,
					nr_pages, pvec->pages);
	return pagevec_count(pvec);
}

EXPORT_SYMBOL(pagevec_lookup_tag);

void __init swap_setup(void)
{
	unsigned long megs = totalram_pages >> (20 - PAGE_SHIFT);

#ifdef CONFIG_SWAP
	bdi_init(swapper_space.backing_dev_info);
#endif

	if (megs < 16)
		page_cluster = 2;
	else
		page_cluster = 3;
	 
}
