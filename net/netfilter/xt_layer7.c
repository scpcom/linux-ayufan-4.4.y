#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/spinlock.h>
#include <linux/version.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
#include <net/netfilter/nf_conntrack_extend.h>
#include <net/netfilter/nf_conntrack_acct.h>
#endif
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_layer7.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>

#include "regexp/regexp.c"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matthew Strait <quadong@users.sf.net>, Ethan Sommer <sommere@users.sf.net>");
MODULE_DESCRIPTION("iptables application layer match module");
MODULE_ALIAS("ipt_layer7");
MODULE_VERSION("2.21");

static int maxdatalen = 2048;  
module_param(maxdatalen, int, 0444);
MODULE_PARM_DESC(maxdatalen, "maximum bytes of data looked at by l7-filter");
#ifdef CONFIG_NETFILTER_XT_MATCH_LAYER7_DEBUG
	#define DPRINTK(format,args...) printk(format,##args)
#else
	#define DPRINTK(format,args...)
#endif

static int num_packets = 10;

static struct pattern_cache {
	char * regex_string;
	regexp * pattern;
	struct pattern_cache * next;
} * first_pattern_cache = NULL;

DEFINE_SPINLOCK(l7_lock);

static int total_acct_packets(struct nf_conn *ct)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 26)
	BUG_ON(ct == NULL);
	return (ct->counters[IP_CT_DIR_ORIGINAL].packets + ct->counters[IP_CT_DIR_REPLY].packets);
#else
	struct nf_conn_counter *acct;

	BUG_ON(ct == NULL);
	acct = nf_conn_acct_find(ct);
	if (!acct)
		return 0;
	return (acct[IP_CT_DIR_ORIGINAL].packets + acct[IP_CT_DIR_REPLY].packets);
#endif
}

#ifdef CONFIG_IP_NF_MATCH_LAYER7_DEBUG
 
static char * friendly_print(unsigned char * s)
{
	char * f = kmalloc(strlen(s) + 1, GFP_ATOMIC);
	int i;

	if(!f) {
		if (net_ratelimit())
			printk(KERN_ERR "layer7: out of memory in "
					"friendly_print, bailing.\n");
		return NULL;
	}

	for(i = 0; i < strlen(s); i++){
		if(isprint(s[i]) && s[i] < 128)	f[i] = s[i];
		else if(isspace(s[i]))		f[i] = ' ';
		else 				f[i] = '.';
	}
	f[i] = '\0';
	return f;
}

static char dec2hex(int i)
{
	switch (i) {
		case 0 ... 9:
			return (i + '0');
			break;
		case 10 ... 15:
			return (i - 10 + 'a');
			break;
		default:
			if (net_ratelimit())
				printk("layer7: Problem in dec2hex\n");
			return '\0';
	}
}

static char * hex_print(unsigned char * s)
{
	char * g = kmalloc(strlen(s)*3 + 1, GFP_ATOMIC);
	int i;

	if(!g) {
	       if (net_ratelimit())
			printk(KERN_ERR "layer7: out of memory in hex_print, "
					"bailing.\n");
	       return NULL;
	}

	for(i = 0; i < strlen(s); i++) {
		g[i*3    ] = dec2hex(s[i]/16);
		g[i*3 + 1] = dec2hex(s[i]%16);
		g[i*3 + 2] = ' ';
	}
	g[i*3] = '\0';

	return g;
}
#endif  

static regexp * compile_and_cache(const char * regex_string,
                                  const char * protocol)
{
	struct pattern_cache * node               = first_pattern_cache;
	struct pattern_cache * last_pattern_cache = first_pattern_cache;
	struct pattern_cache * tmp;
	unsigned int len;

	while (node != NULL) {
		if (!strcmp(node->regex_string, regex_string))
		return node->pattern;

		last_pattern_cache = node; 
		node = node->next;
	}

	tmp = kmalloc(sizeof(struct pattern_cache), GFP_ATOMIC);

	if(!tmp) {
		if (net_ratelimit())
			printk(KERN_ERR "layer7: out of memory in "
					"compile_and_cache, bailing.\n");
		return NULL;
	}

	tmp->regex_string  = kmalloc(strlen(regex_string) + 1, GFP_ATOMIC);
	tmp->pattern       = kmalloc(sizeof(struct regexp),    GFP_ATOMIC);
	tmp->next = NULL;

	if(!tmp->regex_string || !tmp->pattern) {
		if (net_ratelimit())
			printk(KERN_ERR "layer7: out of memory in "
					"compile_and_cache, bailing.\n");
		kfree(tmp->regex_string);
		kfree(tmp->pattern);
		kfree(tmp);
		return NULL;
	}

	node = tmp;

	if(first_pattern_cache == NULL)  
		first_pattern_cache = node;  
	else
		last_pattern_cache->next = node;  

	len = strlen(regex_string);
	DPRINTK("About to compile this: \"%s\"\n", regex_string);
	node->pattern = regcomp((char *)regex_string, &len);
	if ( !node->pattern ) {
		if (net_ratelimit())
			printk(KERN_ERR "layer7: Error compiling regexp "
					"\"%s\" (%s)\n",
					regex_string, protocol);
		 
	}

	strcpy(node->regex_string, regex_string);
	return node->pattern;
}

static int can_handle(const struct sk_buff *skb)
{
	if(!ip_hdr(skb))  
		return 0;
	if(ip_hdr(skb)->protocol != IPPROTO_TCP &&
	   ip_hdr(skb)->protocol != IPPROTO_UDP &&
	   ip_hdr(skb)->protocol != IPPROTO_ICMP)
		return 0;
	return 1;
}

static int app_data_offset(const struct sk_buff *skb)
{
	 
	int ip_hl = 4*ip_hdr(skb)->ihl;

	if( ip_hdr(skb)->protocol == IPPROTO_TCP ) {
		 
		int tcp_hl = 4*(skb->data[ip_hl + 12] >> 4);

		return ip_hl + tcp_hl;
	} else if( ip_hdr(skb)->protocol == IPPROTO_UDP  ) {
		return ip_hl + 8;  
	} else if( ip_hdr(skb)->protocol == IPPROTO_ICMP ) {
		return ip_hl + 8;  
	} else {
		if (net_ratelimit())
			printk(KERN_ERR "layer7: tried to handle unknown "
					"protocol!\n");
		return ip_hl + 8;  
	}
}

static int match_no_append(struct nf_conn * conntrack,
                           struct nf_conn * master_conntrack,
                           enum ip_conntrack_info ctinfo,
                           enum ip_conntrack_info master_ctinfo,
                           const struct xt_layer7_info * info)
{
	 
	if(master_conntrack->layer7.app_data != NULL) {

	#ifdef CONFIG_IP_NF_MATCH_LAYER7_DEBUG
		if(!master_conntrack->layer7.app_proto) {
			char * f =
			  friendly_print(master_conntrack->layer7.app_data);
			char * g =
			  hex_print(master_conntrack->layer7.app_data);
			DPRINTK("\nl7-filter gave up after %d bytes "
				"(%d packets):\n%s\n",
				strlen(f), total_acct_packets(master_conntrack), f);
			kfree(f);
			DPRINTK("In hex: %s\n", g);
			kfree(g);
		}
	#endif

		kfree(master_conntrack->layer7.app_data);
		master_conntrack->layer7.app_data = NULL;  
	}

	if(master_conntrack->layer7.app_proto){
		 
		if(!conntrack->layer7.app_proto) {
			conntrack->layer7.app_proto =
			  kmalloc(strlen(master_conntrack->layer7.app_proto)+1,
			    GFP_ATOMIC);
			if(!conntrack->layer7.app_proto){
				if (net_ratelimit())
					printk(KERN_ERR "layer7: out of memory "
							"in match_no_append, "
							"bailing.\n");
				return 1;
			}
			strcpy(conntrack->layer7.app_proto,
				master_conntrack->layer7.app_proto);
		}

		return (!strcmp(master_conntrack->layer7.app_proto,
				info->protocol));
	}
	else {
		 
		master_conntrack->layer7.app_proto =
			kmalloc(strlen("unknown")+1, GFP_ATOMIC);
		if(!master_conntrack->layer7.app_proto){
			if (net_ratelimit())
				printk(KERN_ERR "layer7: out of memory in "
						"match_no_append, bailing.\n");
			return 1;
		}
		strcpy(master_conntrack->layer7.app_proto, "unknown");
		return 0;
	}
}

#if defined(MY_ABC_HERE)
static int add_datastr(char *target, int offset, char *app_data, int len)
{
	int length = 0, i;
	if (!target) return 0;

 	for(i = 0; i < maxdatalen-offset-1 && i < len; i++) {
		if(app_data[i] != '\0') {
			 
			target[length+offset] =
				isascii(app_data[i])? 
					tolower(app_data[i]) : app_data[i];
			length++;
		}
	}
	target[length+offset] = '\0';

	return length;
}

static int add_data(struct nf_conn * master_conntrack,
                    char * app_data, int appdatalen)
{
	int length;

	length = add_datastr(master_conntrack->layer7.app_data, master_conntrack->layer7.app_data_len, app_data, appdatalen);
	master_conntrack->layer7.app_data_len += length;

	return length;
}
#else
static int add_data(struct nf_conn * master_conntrack,
                    char * app_data, int appdatalen)
{
	int length = 0, i;
	int oldlength = master_conntrack->layer7.app_data_len;

	if(!master_conntrack->layer7.app_data) return 0;

	for(i = 0; i < maxdatalen-oldlength-1 &&
		   i < appdatalen; i++) {
		if(app_data[i] != '\0') {
			 
			master_conntrack->layer7.app_data[length+oldlength] =
				isascii(app_data[i])?
					tolower(app_data[i]) : app_data[i];
			length++;
		}
	}

	master_conntrack->layer7.app_data[length+oldlength] = '\0';
	master_conntrack->layer7.app_data_len = length + oldlength;

	return length;
}
#endif

static int my_atoi(const char *s)
{
	int val = 0;

	for (;; s++) {
		switch (*s) {
			case '0'...'9':
			val = 10*val+(*s-'0');
			break;
		default:
			return val;
		}
	}
}

static int layer7_read_proc(char* page, char ** start, off_t off, int count,
                            int* eof, void * data)
{
	if(num_packets > 99 && net_ratelimit())
		printk(KERN_ERR "layer7: NOT REACHED. num_packets too big\n");

	page[0] = num_packets/10 + '0';
	page[1] = num_packets%10 + '0';
	page[2] = '\n';
	page[3] = '\0';

	*eof=1;

	return 3;
}

static int layer7_write_proc(struct file* file, const char* buffer,
                             unsigned long count, void *data)
{
	char * foo = kmalloc(count, GFP_ATOMIC);

	if(!foo){
		if (net_ratelimit())
			printk(KERN_ERR "layer7: out of memory, bailing. "
					"num_packets unchanged.\n");
		return count;
	}

	if(copy_from_user(foo, buffer, count)) {
		return -EFAULT;
	}

	num_packets = my_atoi(foo);
	kfree (foo);

	if(num_packets > 99) {
		printk(KERN_WARNING "layer7: num_packets can't be > 99.\n");
		num_packets = 99;
	} else if(num_packets < 1) {
		printk(KERN_WARNING "layer7: num_packets can't be < 1.\n");
		num_packets = 1;
	}

	return count;
}

static bool
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
match(const struct sk_buff *skbin, struct xt_action_param *par)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
match(const struct sk_buff *skbin, const struct xt_match_param *par)
#else
match(const struct sk_buff *skbin,
      const struct net_device *in,
      const struct net_device *out,
      const struct xt_match *match,
      const void *matchinfo,
      int offset,
      unsigned int protoff,
      bool *hotdrop)
#endif
{
	 
	struct sk_buff * skb = (struct sk_buff *)skbin;

	const struct xt_layer7_info * info =
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
		par->matchinfo;
	#else
		matchinfo;
	#endif

	enum ip_conntrack_info master_ctinfo, ctinfo;
	struct nf_conn *master_conntrack, *conntrack;
	unsigned char * app_data;
#if defined(MY_ABC_HERE)
	unsigned char *tmp_data;
#endif
	unsigned int pattern_result, appdatalen;
	regexp * comppattern;

	spin_lock_bh(&l7_lock);

	if(!can_handle(skb)){
		DPRINTK("layer7: This is some protocol I can't handle.\n");
		spin_unlock_bh(&l7_lock);
		return info->invert;
	}

	if(!(conntrack = nf_ct_get(skb, &ctinfo)) ||
	   !(master_conntrack=nf_ct_get(skb,&master_ctinfo))){
		DPRINTK("layer7: couldn't get conntrack.\n");
		spin_unlock_bh(&l7_lock);
		return info->invert;
	}

	while (master_ct(master_conntrack) != NULL)
		master_conntrack = master_ct(master_conntrack);

#if defined(MY_ABC_HERE)
	if(!info->pkt && (total_acct_packets(master_conntrack) > num_packets ||
	   master_conntrack->layer7.app_proto)) {
#else
	if(total_acct_packets(master_conntrack) > num_packets ||
	   master_conntrack->layer7.app_proto) {
#endif

		pattern_result = match_no_append(conntrack, master_conntrack,
						 ctinfo, master_ctinfo, info);

		skb->cb[0] = 1;  

		spin_unlock_bh(&l7_lock);
		return (pattern_result ^ info->invert);
	}

	if(skb_is_nonlinear(skb)){
		if(skb_linearize(skb) != 0){
			if (net_ratelimit())
				printk(KERN_ERR "layer7: failed to linearize "
						"packet, bailing.\n");
			spin_unlock_bh(&l7_lock);
			return info->invert;
		}
	}

	app_data = skb->data + app_data_offset(skb);
	appdatalen = skb_tail_pointer(skb) - app_data;

	comppattern = compile_and_cache(info->pattern, info->protocol);

#if defined(MY_ABC_HERE)
	if (info->pkt) {
		tmp_data = kmalloc(maxdatalen, GFP_ATOMIC);
		if(!tmp_data){
			if (net_ratelimit())
				printk(KERN_ERR "layer7: out of memory in match, bailing.\n");
			return info->invert;
		}

		tmp_data[0] = '\0';
		add_datastr(tmp_data, 0, app_data, appdatalen);
		pattern_result = ((comppattern && regexec(comppattern, tmp_data)) ? 1 : 0);

		kfree(tmp_data);
		tmp_data = NULL;
		spin_unlock_bh(&l7_lock);

		return (pattern_result ^ info->invert);
	}
#endif

	if(total_acct_packets(master_conntrack) == 1 && !skb->cb[0] &&
	   !master_conntrack->layer7.app_data){
		master_conntrack->layer7.app_data =
			kmalloc(maxdatalen, GFP_ATOMIC);
		if(!master_conntrack->layer7.app_data){
			if (net_ratelimit())
				printk(KERN_ERR "layer7: out of memory in "
						"match, bailing.\n");
			spin_unlock_bh(&l7_lock);
			return info->invert;
		}

		master_conntrack->layer7.app_data[0] = '\0';
	}

	if(master_conntrack->layer7.app_data == NULL){
		spin_unlock_bh(&l7_lock);
		return info->invert;  
	}

	if(!skb->cb[0]){
		int newbytes;
		newbytes = add_data(master_conntrack, app_data, appdatalen);

		if(newbytes == 0) {  
			skb->cb[0] = 1;
			 
			spin_unlock_bh(&l7_lock);
			return info->invert;
		}
	}

	if(!strcmp(info->protocol, "unknown")) {
		pattern_result = 0;
	 
	} else if(!strcmp(info->protocol, "unset")) {
		pattern_result = 2;
		DPRINTK("layer7: matched unset: not yet classified "
			"(%d/%d packets)\n",
                        total_acct_packets(master_conntrack), num_packets);
	 
	} else if(comppattern &&
		  regexec(comppattern, master_conntrack->layer7.app_data)){
		DPRINTK("layer7: matched %s\n", info->protocol);
		pattern_result = 1;
	} else pattern_result = 0;

	if(pattern_result == 1) {
		master_conntrack->layer7.app_proto =
			kmalloc(strlen(info->protocol)+1, GFP_ATOMIC);
		if(!master_conntrack->layer7.app_proto){
			if (net_ratelimit())
				printk(KERN_ERR "layer7: out of memory in "
						"match, bailing.\n");
			spin_unlock_bh(&l7_lock);
			return (pattern_result ^ info->invert);
		}
		strcpy(master_conntrack->layer7.app_proto, info->protocol);
	} else if(pattern_result > 1) {  
		pattern_result = 1;
	}

	skb->cb[0] = 1;

	spin_unlock_bh(&l7_lock);
	return (pattern_result ^ info->invert);
}

#if defined(MY_ABC_HERE)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
static int
#else
static bool
#endif
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
#if defined(MY_ABC_HERE)
check(const struct xt_mtchk_param *par)
#else
static bool check(const struct xt_mtchk_param *par)
#endif
{
        if (nf_ct_l3proto_try_module_get(par->match->family) < 0) {
                printk(KERN_WARNING "can't load conntrack support for "
                                    "proto=%d\n", par->match->family);
#else
#if defined(MY_ABC_HERE)
check(const char *tablename, const void *inf,
#else
static bool check(const char *tablename, const void *inf,
#endif
		 const struct xt_match *match, void *matchinfo,
		 unsigned int hook_mask)
{
        if (nf_ct_l3proto_try_module_get(match->family) < 0) {
                printk(KERN_WARNING "can't load conntrack support for "
                                    "proto=%d\n", match->family);
#endif
#if defined(MY_ABC_HERE) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
		return -EINVAL;
	}
	return 0;
#else
                return 0;
        }
	return 1;
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
	static void destroy(const struct xt_mtdtor_param *par)
	{
		nf_ct_l3proto_module_put(par->match->family);
	}
#else
	static void destroy(const struct xt_match *match, void *matchinfo)
	{
		nf_ct_l3proto_module_put(match->family);
	}
#endif

static struct xt_match xt_layer7_match[] __read_mostly = {
{
	.name		= "layer7",
	.family		= AF_INET,
	.checkentry	= check,
	.match		= match,
	.destroy	= destroy,
	.matchsize	= sizeof(struct xt_layer7_info),
	.me		= THIS_MODULE
}
};

static void layer7_cleanup_proc(void)
{
	remove_proc_entry("layer7_numpackets", init_net.proc_net);
}

static void layer7_init_proc(void)
{
	struct proc_dir_entry* entry;
	entry = create_proc_entry("layer7_numpackets", 0644, init_net.proc_net);
	entry->read_proc = layer7_read_proc;
	entry->write_proc = layer7_write_proc;
}

static int __init xt_layer7_init(void)
{
	need_conntrack();

	layer7_init_proc();
	if(maxdatalen < 1) {
		printk(KERN_WARNING "layer7: maxdatalen can't be < 1, "
			"using 1\n");
		maxdatalen = 1;
	}
	 
	else if(maxdatalen > 65536) {
		printk(KERN_WARNING "layer7: maxdatalen can't be > 65536, "
			"using 65536\n");
		maxdatalen = 65536;
	}
	return xt_register_matches(xt_layer7_match,
				   ARRAY_SIZE(xt_layer7_match));
}

static void __exit xt_layer7_fini(void)
{
	layer7_cleanup_proc();
	xt_unregister_matches(xt_layer7_match, ARRAY_SIZE(xt_layer7_match));
}

module_init(xt_layer7_init);
module_exit(xt_layer7_fini);
