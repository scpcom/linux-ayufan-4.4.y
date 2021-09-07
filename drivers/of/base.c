#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/of.h>
#include <linux/spinlock.h>

struct device_node *allnodes;

DEFINE_RWLOCK(devtree_lock);

int of_n_addr_cells(struct device_node *np)
{
	const int *ip;

	do {
		if (np->parent)
			np = np->parent;
		ip = of_get_property(np, "#address-cells", NULL);
		if (ip)
			return *ip;
	} while (np->parent);
	 
	return OF_ROOT_NODE_ADDR_CELLS_DEFAULT;
}
EXPORT_SYMBOL(of_n_addr_cells);

int of_n_size_cells(struct device_node *np)
{
	const int *ip;

	do {
		if (np->parent)
			np = np->parent;
		ip = of_get_property(np, "#size-cells", NULL);
		if (ip)
			return *ip;
	} while (np->parent);
	 
	return OF_ROOT_NODE_SIZE_CELLS_DEFAULT;
}
EXPORT_SYMBOL(of_n_size_cells);

struct property *of_find_property(const struct device_node *np,
				  const char *name,
				  int *lenp)
{
	struct property *pp;

	if (!np)
		return NULL;

	read_lock(&devtree_lock);
	for (pp = np->properties; pp != 0; pp = pp->next) {
		if (of_prop_cmp(pp->name, name) == 0) {
			if (lenp != 0)
				*lenp = pp->length;
			break;
		}
	}
	read_unlock(&devtree_lock);

	return pp;
}
EXPORT_SYMBOL(of_find_property);

const void *of_get_property(const struct device_node *np, const char *name,
			 int *lenp)
{
	struct property *pp = of_find_property(np, name, lenp);

	return pp ? pp->value : NULL;
}
EXPORT_SYMBOL(of_get_property);

int of_device_is_compatible(const struct device_node *device,
		const char *compat)
{
	const char* cp;
	int cplen, l;

	cp = of_get_property(device, "compatible", &cplen);
	if (cp == NULL)
		return 0;
	while (cplen > 0) {
		if (of_compat_cmp(cp, compat, strlen(compat)) == 0)
			return 1;
		l = strlen(cp) + 1;
		cp += l;
		cplen -= l;
	}

	return 0;
}
EXPORT_SYMBOL(of_device_is_compatible);

int of_device_is_available(const struct device_node *device)
{
	const char *status;
	int statlen;

	status = of_get_property(device, "status", &statlen);
	if (status == NULL)
		return 1;

	if (statlen > 0) {
		if (!strcmp(status, "okay") || !strcmp(status, "ok"))
			return 1;
	}

	return 0;
}
EXPORT_SYMBOL(of_device_is_available);

struct device_node *of_get_parent(const struct device_node *node)
{
	struct device_node *np;

	if (!node)
		return NULL;

	read_lock(&devtree_lock);
	np = of_node_get(node->parent);
	read_unlock(&devtree_lock);
	return np;
}
EXPORT_SYMBOL(of_get_parent);

struct device_node *of_get_next_parent(struct device_node *node)
{
	struct device_node *parent;

	if (!node)
		return NULL;

	read_lock(&devtree_lock);
	parent = of_node_get(node->parent);
	of_node_put(node);
	read_unlock(&devtree_lock);
	return parent;
}

struct device_node *of_get_next_child(const struct device_node *node,
	struct device_node *prev)
{
	struct device_node *next;

	read_lock(&devtree_lock);
	next = prev ? prev->sibling : node->child;
	for (; next; next = next->sibling)
		if (of_node_get(next))
			break;
	of_node_put(prev);
	read_unlock(&devtree_lock);
	return next;
}
EXPORT_SYMBOL(of_get_next_child);

struct device_node *of_find_node_by_path(const char *path)
{
	struct device_node *np = allnodes;

	read_lock(&devtree_lock);
	for (; np; np = np->allnext) {
		if (np->full_name && (of_node_cmp(np->full_name, path) == 0)
		    && of_node_get(np))
			break;
	}
	read_unlock(&devtree_lock);
	return np;
}
EXPORT_SYMBOL(of_find_node_by_path);

struct device_node *of_find_node_by_name(struct device_node *from,
	const char *name)
{
	struct device_node *np;

	read_lock(&devtree_lock);
	np = from ? from->allnext : allnodes;
	for (; np; np = np->allnext)
		if (np->name && (of_node_cmp(np->name, name) == 0)
		    && of_node_get(np))
			break;
	of_node_put(from);
	read_unlock(&devtree_lock);
	return np;
}
EXPORT_SYMBOL(of_find_node_by_name);

struct device_node *of_find_node_by_type(struct device_node *from,
	const char *type)
{
	struct device_node *np;

	read_lock(&devtree_lock);
	np = from ? from->allnext : allnodes;
	for (; np; np = np->allnext)
		if (np->type && (of_node_cmp(np->type, type) == 0)
		    && of_node_get(np))
			break;
	of_node_put(from);
	read_unlock(&devtree_lock);
	return np;
}
EXPORT_SYMBOL(of_find_node_by_type);

struct device_node *of_find_compatible_node(struct device_node *from,
	const char *type, const char *compatible)
{
	struct device_node *np;

	read_lock(&devtree_lock);
	np = from ? from->allnext : allnodes;
	for (; np; np = np->allnext) {
		if (type
		    && !(np->type && (of_node_cmp(np->type, type) == 0)))
			continue;
		if (of_device_is_compatible(np, compatible) && of_node_get(np))
			break;
	}
	of_node_put(from);
	read_unlock(&devtree_lock);
	return np;
}
EXPORT_SYMBOL(of_find_compatible_node);

struct device_node *of_find_node_with_property(struct device_node *from,
	const char *prop_name)
{
	struct device_node *np;
	struct property *pp;

	read_lock(&devtree_lock);
	np = from ? from->allnext : allnodes;
	for (; np; np = np->allnext) {
		for (pp = np->properties; pp != 0; pp = pp->next) {
			if (of_prop_cmp(pp->name, prop_name) == 0) {
				of_node_get(np);
				goto out;
			}
		}
	}
out:
	of_node_put(from);
	read_unlock(&devtree_lock);
	return np;
}
EXPORT_SYMBOL(of_find_node_with_property);

const struct of_device_id *of_match_node(const struct of_device_id *matches,
					 const struct device_node *node)
{
	while (matches->name[0] || matches->type[0] || matches->compatible[0]) {
		int match = 1;
		if (matches->name[0])
			match &= node->name
				&& !strcmp(matches->name, node->name);
		if (matches->type[0])
			match &= node->type
				&& !strcmp(matches->type, node->type);
		if (matches->compatible[0])
			match &= of_device_is_compatible(node,
						matches->compatible);
		if (match)
			return matches;
		matches++;
	}
	return NULL;
}
EXPORT_SYMBOL(of_match_node);

struct device_node *of_find_matching_node(struct device_node *from,
					  const struct of_device_id *matches)
{
	struct device_node *np;

	read_lock(&devtree_lock);
	np = from ? from->allnext : allnodes;
	for (; np; np = np->allnext) {
		if (of_match_node(matches, np) && of_node_get(np))
			break;
	}
	of_node_put(from);
	read_unlock(&devtree_lock);
	return np;
}
EXPORT_SYMBOL(of_find_matching_node);

struct of_modalias_table {
	char *of_device;
	char *modalias;
};
static struct of_modalias_table of_modalias_table[] = {
	{ "fsl,mcu-mpc8349emitx", "mcu-mpc8349emitx" },
	{ "mmc-spi-slot", "mmc_spi" },
#ifdef MY_DEF_HERE
	{ "fsl,espi-flash", "fsl_m25p80"},
#endif
};

int of_modalias_node(struct device_node *node, char *modalias, int len)
{
	int i, cplen;
	const char *compatible;
	const char *p;

	for (i = 0; i < ARRAY_SIZE(of_modalias_table); i++) {
		compatible = of_modalias_table[i].of_device;
		if (!of_device_is_compatible(node, compatible))
			continue;
		strlcpy(modalias, of_modalias_table[i].modalias, len);
		return 0;
	}

	compatible = of_get_property(node, "compatible", &cplen);
	if (!compatible)
		return -ENODEV;

	p = strchr(compatible, ',');
	if (!p)
		return -ENODEV;
	p++;
	strlcpy(modalias, p, len);
	return 0;
}
EXPORT_SYMBOL_GPL(of_modalias_node);

struct device_node *
of_parse_phandle(struct device_node *np, const char *phandle_name, int index)
{
	const phandle *phandle;
	int size;

	phandle = of_get_property(np, phandle_name, &size);
	if ((!phandle) || (size < sizeof(*phandle) * (index + 1)))
		return NULL;

	return of_find_node_by_phandle(phandle[index]);
}
EXPORT_SYMBOL(of_parse_phandle);

int of_parse_phandles_with_args(struct device_node *np, const char *list_name,
				const char *cells_name, int index,
				struct device_node **out_node,
				const void **out_args)
{
	int ret = -EINVAL;
	const u32 *list;
	const u32 *list_end;
	int size;
	int cur_index = 0;
	struct device_node *node = NULL;
	const void *args = NULL;

	list = of_get_property(np, list_name, &size);
	if (!list) {
		ret = -ENOENT;
		goto err0;
	}
	list_end = list + size / sizeof(*list);

	while (list < list_end) {
		const u32 *cells;
		const phandle *phandle;

		phandle = list++;
		args = list;

		if (!*phandle)
			goto next;

		node = of_find_node_by_phandle(*phandle);
		if (!node) {
			pr_debug("%s: could not find phandle\n",
				 np->full_name);
			goto err0;
		}

		cells = of_get_property(node, cells_name, &size);
		if (!cells || size != sizeof(*cells)) {
			pr_debug("%s: could not get %s for %s\n",
				 np->full_name, cells_name, node->full_name);
			goto err1;
		}

		list += *cells;
		if (list > list_end) {
			pr_debug("%s: insufficient arguments length\n",
				 np->full_name);
			goto err1;
		}
next:
		if (cur_index == index)
			break;

		of_node_put(node);
		node = NULL;
		args = NULL;
		cur_index++;
	}

	if (!node) {
		 
		if (args)
			ret = -EEXIST;
		else
			ret = -ENOENT;
		goto err0;
	}

	if (out_node)
		*out_node = node;
	if (out_args)
		*out_args = args;

	return 0;
err1:
	of_node_put(node);
err0:
	pr_debug("%s failed with status %d\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL(of_parse_phandles_with_args);
