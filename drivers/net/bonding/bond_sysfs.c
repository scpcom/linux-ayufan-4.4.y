#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/sysdev.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/in.h>
#include <linux/sysfs.h>
#include <linux/ctype.h>
#include <linux/inet.h>
#include <linux/rtnetlink.h>
#include <linux/etherdevice.h>
#include <net/net_namespace.h>

#include "bonding.h"

#define to_dev(obj)	container_of(obj, struct device, kobj)
#define to_bond(cd)	((struct bonding *)(netdev_priv(to_net_dev(cd))))

static ssize_t bonding_show_bonds(struct class *cls, char *buf)
{
	int res = 0;
	struct bonding *bond;

	rtnl_lock();

	list_for_each_entry(bond, &bond_dev_list, bond_list) {
		if (res > (PAGE_SIZE - IFNAMSIZ)) {
			 
			if ((PAGE_SIZE - res) > 10)
				res = PAGE_SIZE - 10;
			res += sprintf(buf + res, "++more++ ");
			break;
		}
		res += sprintf(buf + res, "%s ", bond->dev->name);
	}
	if (res)
		buf[res-1] = '\n';  

	rtnl_unlock();
	return res;
}

static struct net_device *bond_get_by_name(const char *ifname)
{
	struct bonding *bond;

	list_for_each_entry(bond, &bond_dev_list, bond_list) {
		if (strncmp(bond->dev->name, ifname, IFNAMSIZ) == 0)
			return bond->dev;
	}
	return NULL;
}

static ssize_t bonding_store_bonds(struct class *cls,
				   const char *buffer, size_t count)
{
	char command[IFNAMSIZ + 1] = {0, };
	char *ifname;
	int rv, res = count;

	sscanf(buffer, "%16s", command);  
	ifname = command + 1;
	if ((strlen(command) <= 1) ||
	    !dev_valid_name(ifname))
		goto err_no_cmd;

	if (command[0] == '+') {
		pr_info(DRV_NAME
			": %s is being created...\n", ifname);
		rv = bond_create(ifname);
		if (rv) {
			pr_info(DRV_NAME ": Bond creation failed.\n");
			res = rv;
		}
	} else if (command[0] == '-') {
		struct net_device *bond_dev;

		rtnl_lock();
		bond_dev = bond_get_by_name(ifname);
		if (bond_dev) {
			pr_info(DRV_NAME ": %s is being deleted...\n",
				ifname);
			unregister_netdevice(bond_dev);
		} else {
			pr_err(DRV_NAME ": unable to delete non-existent %s\n",
			       ifname);
			res = -ENODEV;
		}
		rtnl_unlock();
	} else
		goto err_no_cmd;

	return res;

err_no_cmd:
	pr_err(DRV_NAME ": no command found in bonding_masters."
	       " Use +ifname or -ifname.\n");
	return -EPERM;
}

static CLASS_ATTR(bonding_masters,  S_IWUSR | S_IRUGO,
		  bonding_show_bonds, bonding_store_bonds);

int bond_create_slave_symlinks(struct net_device *master,
			       struct net_device *slave)
{
	char linkname[IFNAMSIZ+7];
	int ret = 0;

	ret = sysfs_create_link(&(slave->dev.kobj), &(master->dev.kobj),
				"master");
	if (ret)
		return ret;
	 
	sprintf(linkname, "slave_%s", slave->name);
	ret = sysfs_create_link(&(master->dev.kobj), &(slave->dev.kobj),
				linkname);
	return ret;

}

void bond_destroy_slave_symlinks(struct net_device *master,
				 struct net_device *slave)
{
	char linkname[IFNAMSIZ+7];

	sysfs_remove_link(&(slave->dev.kobj), "master");
	sprintf(linkname, "slave_%s", slave->name);
	sysfs_remove_link(&(master->dev.kobj), linkname);
}

static ssize_t bonding_show_slaves(struct device *d,
				   struct device_attribute *attr, char *buf)
{
	struct slave *slave;
	int i, res = 0;
	struct bonding *bond = to_bond(d);

	read_lock(&bond->lock);
	bond_for_each_slave(bond, slave, i) {
		if (res > (PAGE_SIZE - IFNAMSIZ)) {
			 
			if ((PAGE_SIZE - res) > 10)
				res = PAGE_SIZE - 10;
			res += sprintf(buf + res, "++more++ ");
			break;
		}
		res += sprintf(buf + res, "%s ", slave->dev->name);
	}
	read_unlock(&bond->lock);
	if (res)
		buf[res-1] = '\n';  
	return res;
}

static ssize_t bonding_store_slaves(struct device *d,
				    struct device_attribute *attr,
				    const char *buffer, size_t count)
{
	char command[IFNAMSIZ + 1] = { 0, };
	char *ifname;
	int i, res, found, ret = count;
	u32 original_mtu;
	struct slave *slave;
	struct net_device *dev = NULL;
	struct bonding *bond = to_bond(d);

	if (!(bond->dev->flags & IFF_UP)) {
		pr_warning(DRV_NAME ": %s: doing slave updates when "
			   "interface is down.\n", bond->dev->name);
	}

	if (!rtnl_trylock())
		return restart_syscall();

	sscanf(buffer, "%16s", command);  
	ifname = command + 1;
	if ((strlen(command) <= 1) ||
	    !dev_valid_name(ifname))
		goto err_no_cmd;

	if (command[0] == '+') {

		found = 0;

		dev = __dev_get_by_name(&init_net, ifname);
		if (!dev) {
			pr_info(DRV_NAME
			       ": %s: Interface %s does not exist!\n",
			       bond->dev->name, ifname);
			ret = -ENODEV;
			goto out;
		}

		if (dev->flags & IFF_UP) {
			pr_err(DRV_NAME
			       ": %s: Error: Unable to enslave %s "
			       "because it is already up.\n",
			       bond->dev->name, dev->name);
			ret = -EPERM;
			goto out;
		}

		read_lock(&bond->lock);
		bond_for_each_slave(bond, slave, i)
			if (slave->dev == dev) {
				pr_err(DRV_NAME
				       ": %s: Interface %s is already enslaved!\n",
				       bond->dev->name, ifname);
				ret = -EPERM;
				read_unlock(&bond->lock);
				goto out;
			}
		read_unlock(&bond->lock);

		pr_info(DRV_NAME ": %s: Adding slave %s.\n",
			bond->dev->name, ifname);

		if (is_zero_ether_addr(bond->dev->dev_addr))
#ifdef MY_ABC_HERE
		{
			unsigned char szMac[MAX_ADDR_LEN];
			memset(szMac, 0, sizeof(szMac));

			if (syno_get_dev_vendor_mac(dev->name, szMac)){
				printk("%s:%s(%d) dev:[%s] get vendor mac fail\n", 
						__FILE__, __FUNCTION__, __LINE__, dev->name);
				 
				memcpy(bond->dev->dev_addr, dev->dev_addr,
						dev->addr_len);
			} else {
				 
				memcpy(bond->dev->dev_addr, szMac, dev->addr_len);
			}
		}
#else
			memcpy(bond->dev->dev_addr, dev->dev_addr,
			       dev->addr_len);
#endif

		original_mtu = dev->mtu;
		res = dev_set_mtu(dev, bond->dev->mtu);
		if (res) {
			ret = res;
			goto out;
		}

		res = bond_enslave(bond->dev, dev);
		bond_for_each_slave(bond, slave, i)
			if (strnicmp(slave->dev->name, ifname, IFNAMSIZ) == 0)
				slave->original_mtu = original_mtu;
		if (res)
			ret = res;

		goto out;
	}

	if (command[0] == '-') {
		dev = NULL;
		original_mtu = 0;
		bond_for_each_slave(bond, slave, i)
			if (strnicmp(slave->dev->name, ifname, IFNAMSIZ) == 0) {
				dev = slave->dev;
				original_mtu = slave->original_mtu;
				break;
			}
		if (dev) {
			pr_info(DRV_NAME ": %s: Removing slave %s\n",
				bond->dev->name, dev->name);
				res = bond_release(bond->dev, dev);
			if (res) {
				ret = res;
				goto out;
			}
			 
			dev_set_mtu(dev, original_mtu);
		} else {
			pr_err(DRV_NAME ": unable to remove non-existent"
			       " slave %s for bond %s.\n",
				ifname, bond->dev->name);
			ret = -ENODEV;
		}
		goto out;
	}

err_no_cmd:
	pr_err(DRV_NAME ": no command found in slaves file for bond %s. Use +ifname or -ifname.\n", bond->dev->name);
	ret = -EPERM;

out:
	rtnl_unlock();
	return ret;
}

static DEVICE_ATTR(slaves, S_IRUGO | S_IWUSR, bonding_show_slaves,
		   bonding_store_slaves);

static ssize_t bonding_show_mode(struct device *d,
				 struct device_attribute *attr, char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%s %d\n",
			bond_mode_tbl[bond->params.mode].modename,
			bond->params.mode);
}

static ssize_t bonding_store_mode(struct device *d,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int new_value, ret = count;
	struct bonding *bond = to_bond(d);

	if (bond->dev->flags & IFF_UP) {
		pr_err(DRV_NAME ": unable to update mode of %s"
		       " because interface is up.\n", bond->dev->name);
		ret = -EPERM;
		goto out;
	}

	new_value = bond_parse_parm(buf, bond_mode_tbl);
	if (new_value < 0)  {
		pr_err(DRV_NAME
		       ": %s: Ignoring invalid mode value %.*s.\n",
		       bond->dev->name,
		       (int)strlen(buf) - 1, buf);
		ret = -EINVAL;
		goto out;
	} else {
		if (bond->params.mode == BOND_MODE_8023AD)
			bond_unset_master_3ad_flags(bond);

		if (bond->params.mode == BOND_MODE_ALB)
			bond_unset_master_alb_flags(bond);

		bond->params.mode = new_value;
		bond_set_mode_ops(bond, bond->params.mode);
		pr_info(DRV_NAME ": %s: setting mode to %s (%d).\n",
		       bond->dev->name, bond_mode_tbl[new_value].modename,
		       new_value);
	}
out:
	return ret;
}
static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR,
		   bonding_show_mode, bonding_store_mode);

static ssize_t bonding_show_xmit_hash(struct device *d,
				      struct device_attribute *attr,
				      char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%s %d\n",
		       xmit_hashtype_tbl[bond->params.xmit_policy].modename,
		       bond->params.xmit_policy);
}

static ssize_t bonding_store_xmit_hash(struct device *d,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int new_value, ret = count;
	struct bonding *bond = to_bond(d);

	if (bond->dev->flags & IFF_UP) {
		pr_err(DRV_NAME
		       "%s: Interface is up. Unable to update xmit policy.\n",
		       bond->dev->name);
		ret = -EPERM;
		goto out;
	}

	new_value = bond_parse_parm(buf, xmit_hashtype_tbl);
	if (new_value < 0)  {
		pr_err(DRV_NAME
		       ": %s: Ignoring invalid xmit hash policy value %.*s.\n",
		       bond->dev->name,
		       (int)strlen(buf) - 1, buf);
		ret = -EINVAL;
		goto out;
	} else {
		bond->params.xmit_policy = new_value;
		bond_set_mode_ops(bond, bond->params.mode);
		pr_info(DRV_NAME ": %s: setting xmit hash policy to %s (%d).\n",
			bond->dev->name,
			xmit_hashtype_tbl[new_value].modename, new_value);
	}
out:
	return ret;
}
static DEVICE_ATTR(xmit_hash_policy, S_IRUGO | S_IWUSR,
		   bonding_show_xmit_hash, bonding_store_xmit_hash);

static ssize_t bonding_show_arp_validate(struct device *d,
					 struct device_attribute *attr,
					 char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%s %d\n",
		       arp_validate_tbl[bond->params.arp_validate].modename,
		       bond->params.arp_validate);
}

static ssize_t bonding_store_arp_validate(struct device *d,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	int new_value;
	struct bonding *bond = to_bond(d);

	new_value = bond_parse_parm(buf, arp_validate_tbl);
	if (new_value < 0) {
		pr_err(DRV_NAME
		       ": %s: Ignoring invalid arp_validate value %s\n",
		       bond->dev->name, buf);
		return -EINVAL;
	}
	if (new_value && (bond->params.mode != BOND_MODE_ACTIVEBACKUP)) {
		pr_err(DRV_NAME
		       ": %s: arp_validate only supported in active-backup mode.\n",
		       bond->dev->name);
		return -EINVAL;
	}
	pr_info(DRV_NAME ": %s: setting arp_validate to %s (%d).\n",
	       bond->dev->name, arp_validate_tbl[new_value].modename,
	       new_value);

	if (!bond->params.arp_validate && new_value)
		bond_register_arp(bond);
	else if (bond->params.arp_validate && !new_value)
		bond_unregister_arp(bond);

	bond->params.arp_validate = new_value;

	return count;
}

static DEVICE_ATTR(arp_validate, S_IRUGO | S_IWUSR, bonding_show_arp_validate,
		   bonding_store_arp_validate);

static ssize_t bonding_show_fail_over_mac(struct device *d,
					  struct device_attribute *attr,
					  char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%s %d\n",
		       fail_over_mac_tbl[bond->params.fail_over_mac].modename,
		       bond->params.fail_over_mac);
}

static ssize_t bonding_store_fail_over_mac(struct device *d,
					   struct device_attribute *attr,
					   const char *buf, size_t count)
{
	int new_value;
	struct bonding *bond = to_bond(d);

	if (bond->slave_cnt != 0) {
		pr_err(DRV_NAME
		       ": %s: Can't alter fail_over_mac with slaves in bond.\n",
		       bond->dev->name);
		return -EPERM;
	}

	new_value = bond_parse_parm(buf, fail_over_mac_tbl);
	if (new_value < 0) {
		pr_err(DRV_NAME
		       ": %s: Ignoring invalid fail_over_mac value %s.\n",
		       bond->dev->name, buf);
		return -EINVAL;
	}

	bond->params.fail_over_mac = new_value;
	pr_info(DRV_NAME ": %s: Setting fail_over_mac to %s (%d).\n",
	       bond->dev->name, fail_over_mac_tbl[new_value].modename,
	       new_value);

	return count;
}

static DEVICE_ATTR(fail_over_mac, S_IRUGO | S_IWUSR,
		   bonding_show_fail_over_mac, bonding_store_fail_over_mac);

static ssize_t bonding_show_arp_interval(struct device *d,
					 struct device_attribute *attr,
					 char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.arp_interval);
}

static ssize_t bonding_store_arp_interval(struct device *d,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	int new_value, ret = count;
	struct bonding *bond = to_bond(d);

	if (sscanf(buf, "%d", &new_value) != 1) {
		pr_err(DRV_NAME
		       ": %s: no arp_interval value specified.\n",
		       bond->dev->name);
		ret = -EINVAL;
		goto out;
	}
	if (new_value < 0) {
		pr_err(DRV_NAME
		       ": %s: Invalid arp_interval value %d not in range 1-%d; rejected.\n",
		       bond->dev->name, new_value, INT_MAX);
		ret = -EINVAL;
		goto out;
	}

	pr_info(DRV_NAME
	       ": %s: Setting ARP monitoring interval to %d.\n",
	       bond->dev->name, new_value);
	bond->params.arp_interval = new_value;
	if (bond->params.arp_interval)
		bond->dev->priv_flags |= IFF_MASTER_ARPMON;
	if (bond->params.miimon) {
		pr_info(DRV_NAME
		       ": %s: ARP monitoring cannot be used with MII monitoring. "
		       "%s Disabling MII monitoring.\n",
		       bond->dev->name, bond->dev->name);
		bond->params.miimon = 0;
		if (delayed_work_pending(&bond->mii_work)) {
			cancel_delayed_work(&bond->mii_work);
			flush_workqueue(bond->wq);
		}
	}
	if (!bond->params.arp_targets[0]) {
		pr_info(DRV_NAME
		       ": %s: ARP monitoring has been set up, "
		       "but no ARP targets have been specified.\n",
		       bond->dev->name);
	}
	if (bond->dev->flags & IFF_UP) {
		 
		if (!delayed_work_pending(&bond->arp_work)) {
			if (bond->params.mode == BOND_MODE_ACTIVEBACKUP)
				INIT_DELAYED_WORK(&bond->arp_work,
						  bond_activebackup_arp_mon);
			else
				INIT_DELAYED_WORK(&bond->arp_work,
						  bond_loadbalance_arp_mon);

			queue_delayed_work(bond->wq, &bond->arp_work, 0);
		}
	}

out:
	return ret;
}
static DEVICE_ATTR(arp_interval, S_IRUGO | S_IWUSR,
		   bonding_show_arp_interval, bonding_store_arp_interval);

static ssize_t bonding_show_arp_targets(struct device *d,
					struct device_attribute *attr,
					char *buf)
{
	int i, res = 0;
	struct bonding *bond = to_bond(d);

	for (i = 0; i < BOND_MAX_ARP_TARGETS; i++) {
		if (bond->params.arp_targets[i])
			res += sprintf(buf + res, "%pI4 ",
				       &bond->params.arp_targets[i]);
	}
	if (res)
		buf[res-1] = '\n';  
	return res;
}

static ssize_t bonding_store_arp_targets(struct device *d,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	__be32 newtarget;
	int i = 0, done = 0, ret = count;
	struct bonding *bond = to_bond(d);
	__be32 *targets;

	targets = bond->params.arp_targets;
	newtarget = in_aton(buf + 1);
	 
	if (buf[0] == '+') {
		if ((newtarget == 0) || (newtarget == htonl(INADDR_BROADCAST))) {
			pr_err(DRV_NAME
			       ": %s: invalid ARP target %pI4 specified for addition\n",
			       bond->dev->name, &newtarget);
			ret = -EINVAL;
			goto out;
		}
		 
		for (i = 0; (i < BOND_MAX_ARP_TARGETS) && !done; i++) {
			if (targets[i] == newtarget) {  
				pr_err(DRV_NAME
				       ": %s: ARP target %pI4 is already present\n",
				       bond->dev->name, &newtarget);
				ret = -EINVAL;
				goto out;
			}
			if (targets[i] == 0) {
				pr_info(DRV_NAME
				       ": %s: adding ARP target %pI4.\n",
				       bond->dev->name, &newtarget);
				done = 1;
				targets[i] = newtarget;
			}
		}
		if (!done) {
			pr_err(DRV_NAME
			       ": %s: ARP target table is full!\n",
			       bond->dev->name);
			ret = -EINVAL;
			goto out;
		}

	} else if (buf[0] == '-')	{
		if ((newtarget == 0) || (newtarget == htonl(INADDR_BROADCAST))) {
			pr_err(DRV_NAME
			       ": %s: invalid ARP target %pI4 specified for removal\n",
			       bond->dev->name, &newtarget);
			ret = -EINVAL;
			goto out;
		}

		for (i = 0; (i < BOND_MAX_ARP_TARGETS) && !done; i++) {
			if (targets[i] == newtarget) {
				int j;
				pr_info(DRV_NAME
				       ": %s: removing ARP target %pI4.\n",
				       bond->dev->name, &newtarget);
				for (j = i; (j < (BOND_MAX_ARP_TARGETS-1)) && targets[j+1]; j++)
					targets[j] = targets[j+1];

				targets[j] = 0;
				done = 1;
			}
		}
		if (!done) {
			pr_info(DRV_NAME
			       ": %s: unable to remove nonexistent ARP target %pI4.\n",
			       bond->dev->name, &newtarget);
			ret = -EINVAL;
			goto out;
		}
	} else {
		pr_err(DRV_NAME ": no command found in arp_ip_targets file"
		       " for bond %s. Use +<addr> or -<addr>.\n",
			bond->dev->name);
		ret = -EPERM;
		goto out;
	}

out:
	return ret;
}
static DEVICE_ATTR(arp_ip_target, S_IRUGO | S_IWUSR , bonding_show_arp_targets, bonding_store_arp_targets);

static ssize_t bonding_show_downdelay(struct device *d,
				      struct device_attribute *attr,
				      char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.downdelay * bond->params.miimon);
}

static ssize_t bonding_store_downdelay(struct device *d,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int new_value, ret = count;
	struct bonding *bond = to_bond(d);

	if (!(bond->params.miimon)) {
		pr_err(DRV_NAME
		       ": %s: Unable to set down delay as MII monitoring is disabled\n",
		       bond->dev->name);
		ret = -EPERM;
		goto out;
	}

	if (sscanf(buf, "%d", &new_value) != 1) {
		pr_err(DRV_NAME
		       ": %s: no down delay value specified.\n",
		       bond->dev->name);
		ret = -EINVAL;
		goto out;
	}
	if (new_value < 0) {
		pr_err(DRV_NAME
		       ": %s: Invalid down delay value %d not in range %d-%d; rejected.\n",
		       bond->dev->name, new_value, 1, INT_MAX);
		ret = -EINVAL;
		goto out;
	} else {
		if ((new_value % bond->params.miimon) != 0) {
			pr_warning(DRV_NAME
				   ": %s: Warning: down delay (%d) is not a "
				   "multiple of miimon (%d), delay rounded "
				   "to %d ms\n",
				   bond->dev->name, new_value,
				   bond->params.miimon,
				   (new_value / bond->params.miimon) *
				   bond->params.miimon);
		}
		bond->params.downdelay = new_value / bond->params.miimon;
		pr_info(DRV_NAME ": %s: Setting down delay to %d.\n",
		       bond->dev->name,
		       bond->params.downdelay * bond->params.miimon);

	}

out:
	return ret;
}
static DEVICE_ATTR(downdelay, S_IRUGO | S_IWUSR,
		   bonding_show_downdelay, bonding_store_downdelay);

static ssize_t bonding_show_updelay(struct device *d,
				    struct device_attribute *attr,
				    char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.updelay * bond->params.miimon);

}

static ssize_t bonding_store_updelay(struct device *d,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int new_value, ret = count;
	struct bonding *bond = to_bond(d);

	if (!(bond->params.miimon)) {
		pr_err(DRV_NAME
		       ": %s: Unable to set up delay as MII monitoring is disabled\n",
		       bond->dev->name);
		ret = -EPERM;
		goto out;
	}

	if (sscanf(buf, "%d", &new_value) != 1) {
		pr_err(DRV_NAME
		       ": %s: no up delay value specified.\n",
		       bond->dev->name);
		ret = -EINVAL;
		goto out;
	}
	if (new_value < 0) {
		pr_err(DRV_NAME
		       ": %s: Invalid down delay value %d not in range %d-%d; rejected.\n",
		       bond->dev->name, new_value, 1, INT_MAX);
		ret = -EINVAL;
		goto out;
	} else {
		if ((new_value % bond->params.miimon) != 0) {
			pr_warning(DRV_NAME
				   ": %s: Warning: up delay (%d) is not a "
				   "multiple of miimon (%d), updelay rounded "
				   "to %d ms\n",
				   bond->dev->name, new_value,
				   bond->params.miimon,
				   (new_value / bond->params.miimon) *
				   bond->params.miimon);
		}
		bond->params.updelay = new_value / bond->params.miimon;
		pr_info(DRV_NAME ": %s: Setting up delay to %d.\n",
		       bond->dev->name, bond->params.updelay * bond->params.miimon);

	}

out:
	return ret;
}
static DEVICE_ATTR(updelay, S_IRUGO | S_IWUSR,
		   bonding_show_updelay, bonding_store_updelay);

static ssize_t bonding_show_lacp(struct device *d,
				 struct device_attribute *attr,
				 char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%s %d\n",
		bond_lacp_tbl[bond->params.lacp_fast].modename,
		bond->params.lacp_fast);
}

static ssize_t bonding_store_lacp(struct device *d,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int new_value, ret = count;
	struct bonding *bond = to_bond(d);

	if (bond->dev->flags & IFF_UP) {
		pr_err(DRV_NAME
		       ": %s: Unable to update LACP rate because interface is up.\n",
		       bond->dev->name);
		ret = -EPERM;
		goto out;
	}

	if (bond->params.mode != BOND_MODE_8023AD) {
		pr_err(DRV_NAME
		       ": %s: Unable to update LACP rate because bond is not in 802.3ad mode.\n",
		       bond->dev->name);
		ret = -EPERM;
		goto out;
	}

	new_value = bond_parse_parm(buf, bond_lacp_tbl);

	if ((new_value == 1) || (new_value == 0)) {
		bond->params.lacp_fast = new_value;
		pr_info(DRV_NAME ": %s: Setting LACP rate to %s (%d).\n",
			bond->dev->name, bond_lacp_tbl[new_value].modename,
			new_value);
	} else {
		pr_err(DRV_NAME
		       ": %s: Ignoring invalid LACP rate value %.*s.\n",
		       bond->dev->name, (int)strlen(buf) - 1, buf);
		ret = -EINVAL;
	}
out:
	return ret;
}
static DEVICE_ATTR(lacp_rate, S_IRUGO | S_IWUSR,
		   bonding_show_lacp, bonding_store_lacp);

static ssize_t bonding_show_ad_select(struct device *d,
				      struct device_attribute *attr,
				      char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%s %d\n",
		ad_select_tbl[bond->params.ad_select].modename,
		bond->params.ad_select);
}

static ssize_t bonding_store_ad_select(struct device *d,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int new_value, ret = count;
	struct bonding *bond = to_bond(d);

	if (bond->dev->flags & IFF_UP) {
		pr_err(DRV_NAME
		       ": %s: Unable to update ad_select because interface "
		       "is up.\n", bond->dev->name);
		ret = -EPERM;
		goto out;
	}

	new_value = bond_parse_parm(buf, ad_select_tbl);

	if (new_value != -1) {
		bond->params.ad_select = new_value;
		pr_info(DRV_NAME
		       ": %s: Setting ad_select to %s (%d).\n",
		       bond->dev->name, ad_select_tbl[new_value].modename,
		       new_value);
	} else {
		pr_err(DRV_NAME
		       ": %s: Ignoring invalid ad_select value %.*s.\n",
		       bond->dev->name, (int)strlen(buf) - 1, buf);
		ret = -EINVAL;
	}
out:
	return ret;
}
static DEVICE_ATTR(ad_select, S_IRUGO | S_IWUSR,
		   bonding_show_ad_select, bonding_store_ad_select);

static ssize_t bonding_show_n_grat_arp(struct device *d,
				   struct device_attribute *attr,
				   char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.num_grat_arp);
}

static ssize_t bonding_store_n_grat_arp(struct device *d,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int new_value, ret = count;
	struct bonding *bond = to_bond(d);

	if (sscanf(buf, "%d", &new_value) != 1) {
		pr_err(DRV_NAME
		       ": %s: no num_grat_arp value specified.\n",
		       bond->dev->name);
		ret = -EINVAL;
		goto out;
	}
	if (new_value < 0 || new_value > 255) {
		pr_err(DRV_NAME
		       ": %s: Invalid num_grat_arp value %d not in range 0-255; rejected.\n",
		       bond->dev->name, new_value);
		ret = -EINVAL;
		goto out;
	} else {
		bond->params.num_grat_arp = new_value;
	}
out:
	return ret;
}
static DEVICE_ATTR(num_grat_arp, S_IRUGO | S_IWUSR,
		   bonding_show_n_grat_arp, bonding_store_n_grat_arp);

static ssize_t bonding_show_n_unsol_na(struct device *d,
				       struct device_attribute *attr,
				       char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.num_unsol_na);
}

static ssize_t bonding_store_n_unsol_na(struct device *d,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int new_value, ret = count;
	struct bonding *bond = to_bond(d);

	if (sscanf(buf, "%d", &new_value) != 1) {
		pr_err(DRV_NAME
		       ": %s: no num_unsol_na value specified.\n",
		       bond->dev->name);
		ret = -EINVAL;
		goto out;
	}

	if (new_value < 0 || new_value > 255) {
		pr_err(DRV_NAME
		       ": %s: Invalid num_unsol_na value %d not in range 0-255; rejected.\n",
		       bond->dev->name, new_value);
		ret = -EINVAL;
		goto out;
	} else
		bond->params.num_unsol_na = new_value;
out:
	return ret;
}
static DEVICE_ATTR(num_unsol_na, S_IRUGO | S_IWUSR,
		   bonding_show_n_unsol_na, bonding_store_n_unsol_na);

static ssize_t bonding_show_miimon(struct device *d,
				   struct device_attribute *attr,
				   char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.miimon);
}

static ssize_t bonding_store_miimon(struct device *d,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int new_value, ret = count;
	struct bonding *bond = to_bond(d);

	if (sscanf(buf, "%d", &new_value) != 1) {
		pr_err(DRV_NAME
		       ": %s: no miimon value specified.\n",
		       bond->dev->name);
		ret = -EINVAL;
		goto out;
	}
	if (new_value < 0) {
		pr_err(DRV_NAME
		       ": %s: Invalid miimon value %d not in range %d-%d; rejected.\n",
		       bond->dev->name, new_value, 1, INT_MAX);
		ret = -EINVAL;
		goto out;
	} else {
		pr_info(DRV_NAME
		       ": %s: Setting MII monitoring interval to %d.\n",
		       bond->dev->name, new_value);
		bond->params.miimon = new_value;
		if (bond->params.updelay)
			pr_info(DRV_NAME
			      ": %s: Note: Updating updelay (to %d) "
			      "since it is a multiple of the miimon value.\n",
			      bond->dev->name,
			      bond->params.updelay * bond->params.miimon);
		if (bond->params.downdelay)
			pr_info(DRV_NAME
			      ": %s: Note: Updating downdelay (to %d) "
			      "since it is a multiple of the miimon value.\n",
			      bond->dev->name,
			      bond->params.downdelay * bond->params.miimon);
		if (bond->params.arp_interval) {
			pr_info(DRV_NAME
			       ": %s: MII monitoring cannot be used with "
			       "ARP monitoring. Disabling ARP monitoring...\n",
			       bond->dev->name);
			bond->params.arp_interval = 0;
			bond->dev->priv_flags &= ~IFF_MASTER_ARPMON;
			if (bond->params.arp_validate) {
				bond_unregister_arp(bond);
				bond->params.arp_validate =
					BOND_ARP_VALIDATE_NONE;
			}
			if (delayed_work_pending(&bond->arp_work)) {
				cancel_delayed_work(&bond->arp_work);
				flush_workqueue(bond->wq);
			}
		}

		if (bond->dev->flags & IFF_UP) {
			 
			if (!delayed_work_pending(&bond->mii_work)) {
				INIT_DELAYED_WORK(&bond->mii_work,
						  bond_mii_monitor);
				queue_delayed_work(bond->wq,
						   &bond->mii_work, 0);
			}
		}
	}
out:
	return ret;
}
static DEVICE_ATTR(miimon, S_IRUGO | S_IWUSR,
		   bonding_show_miimon, bonding_store_miimon);

static ssize_t bonding_show_primary(struct device *d,
				    struct device_attribute *attr,
				    char *buf)
{
	int count = 0;
	struct bonding *bond = to_bond(d);

	if (bond->primary_slave)
		count = sprintf(buf, "%s\n", bond->primary_slave->dev->name);

	return count;
}

static ssize_t bonding_store_primary(struct device *d,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int i;
	struct slave *slave;
	struct bonding *bond = to_bond(d);

	if (!rtnl_trylock())
		return restart_syscall();
	read_lock(&bond->lock);
	write_lock_bh(&bond->curr_slave_lock);

	if (!USES_PRIMARY(bond->params.mode)) {
		pr_info(DRV_NAME
		       ": %s: Unable to set primary slave; %s is in mode %d\n",
		       bond->dev->name, bond->dev->name, bond->params.mode);
	} else {
		bond_for_each_slave(bond, slave, i) {
			if (strnicmp
			    (slave->dev->name, buf,
			     strlen(slave->dev->name)) == 0) {
				pr_info(DRV_NAME
				       ": %s: Setting %s as primary slave.\n",
				       bond->dev->name, slave->dev->name);
				bond->primary_slave = slave;
				strcpy(bond->params.primary, slave->dev->name);
				bond_select_active_slave(bond);
				goto out;
			}
		}

		if (strlen(buf) == 0 || buf[0] == '\n') {
			pr_info(DRV_NAME
			       ": %s: Setting primary slave to None.\n",
			       bond->dev->name);
			bond->primary_slave = NULL;
				bond_select_active_slave(bond);
		} else {
			pr_info(DRV_NAME
			       ": %s: Unable to set %.*s as primary slave as it is not a slave.\n",
			       bond->dev->name, (int)strlen(buf) - 1, buf);
		}
	}
out:
	write_unlock_bh(&bond->curr_slave_lock);
	read_unlock(&bond->lock);
	rtnl_unlock();

	return count;
}
static DEVICE_ATTR(primary, S_IRUGO | S_IWUSR,
		   bonding_show_primary, bonding_store_primary);

static ssize_t bonding_show_carrier(struct device *d,
				    struct device_attribute *attr,
				    char *buf)
{
	struct bonding *bond = to_bond(d);

	return sprintf(buf, "%d\n", bond->params.use_carrier);
}

static ssize_t bonding_store_carrier(struct device *d,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int new_value, ret = count;
	struct bonding *bond = to_bond(d);

	if (sscanf(buf, "%d", &new_value) != 1) {
		pr_err(DRV_NAME
		       ": %s: no use_carrier value specified.\n",
		       bond->dev->name);
		ret = -EINVAL;
		goto out;
	}
	if ((new_value == 0) || (new_value == 1)) {
		bond->params.use_carrier = new_value;
		pr_info(DRV_NAME ": %s: Setting use_carrier to %d.\n",
		       bond->dev->name, new_value);
	} else {
		pr_info(DRV_NAME
		       ": %s: Ignoring invalid use_carrier value %d.\n",
		       bond->dev->name, new_value);
	}
out:
	return count;
}
static DEVICE_ATTR(use_carrier, S_IRUGO | S_IWUSR,
		   bonding_show_carrier, bonding_store_carrier);

static ssize_t bonding_show_active_slave(struct device *d,
					 struct device_attribute *attr,
					 char *buf)
{
	struct slave *curr;
	struct bonding *bond = to_bond(d);
	int count = 0;

	read_lock(&bond->curr_slave_lock);
	curr = bond->curr_active_slave;
	read_unlock(&bond->curr_slave_lock);

	if (USES_PRIMARY(bond->params.mode) && curr)
		count = sprintf(buf, "%s\n", curr->dev->name);
	return count;
}

static ssize_t bonding_store_active_slave(struct device *d,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	int i;
	struct slave *slave;
	struct slave *old_active = NULL;
	struct slave *new_active = NULL;
	struct bonding *bond = to_bond(d);

	if (!rtnl_trylock())
		return restart_syscall();
	read_lock(&bond->lock);
	write_lock_bh(&bond->curr_slave_lock);

	if (!USES_PRIMARY(bond->params.mode))
		pr_info(DRV_NAME ": %s: Unable to change active slave;"
			" %s is in mode %d\n",
			bond->dev->name, bond->dev->name, bond->params.mode);
	else {
		bond_for_each_slave(bond, slave, i) {
			if (strnicmp
			    (slave->dev->name, buf,
			     strlen(slave->dev->name)) == 0) {
        			old_active = bond->curr_active_slave;
        			new_active = slave;
        			if (new_active == old_active) {
					 
					pr_info(DRV_NAME
						": %s: %s is already the current active slave.\n",
						bond->dev->name, slave->dev->name);
					goto out;
				}
				else {
        				if ((new_active) &&
            				    (old_active) &&
				            (new_active->link == BOND_LINK_UP) &&
				            IS_UP(new_active->dev)) {
						pr_info(DRV_NAME
							": %s: Setting %s as active slave.\n",
							bond->dev->name, slave->dev->name);
							bond_change_active_slave(bond, new_active);
        				}
					else {
						pr_info(DRV_NAME
							": %s: Could not set %s as active slave; "
							"either %s is down or the link is down.\n",
							bond->dev->name, slave->dev->name,
							slave->dev->name);
					}
					goto out;
				}
			}
		}

		if (strlen(buf) == 0 || buf[0] == '\n') {
			pr_info(DRV_NAME
				": %s: Setting active slave to None.\n",
				bond->dev->name);
			bond->primary_slave = NULL;
			bond_select_active_slave(bond);
		} else {
			pr_info(DRV_NAME ": %s: Unable to set %.*s"
				" as active slave as it is not a slave.\n",
				bond->dev->name, (int)strlen(buf) - 1, buf);
		}
	}
 out:
	write_unlock_bh(&bond->curr_slave_lock);
	read_unlock(&bond->lock);
	rtnl_unlock();

	return count;

}
static DEVICE_ATTR(active_slave, S_IRUGO | S_IWUSR,
		   bonding_show_active_slave, bonding_store_active_slave);

static ssize_t bonding_show_mii_status(struct device *d,
				       struct device_attribute *attr,
				       char *buf)
{
	struct slave *curr;
	struct bonding *bond = to_bond(d);

	read_lock(&bond->curr_slave_lock);
	curr = bond->curr_active_slave;
	read_unlock(&bond->curr_slave_lock);

	return sprintf(buf, "%s\n", curr ? "up" : "down");
}
static DEVICE_ATTR(mii_status, S_IRUGO, bonding_show_mii_status, NULL);

static ssize_t bonding_show_ad_aggregator(struct device *d,
					  struct device_attribute *attr,
					  char *buf)
{
	int count = 0;
	struct bonding *bond = to_bond(d);

	if (bond->params.mode == BOND_MODE_8023AD) {
		struct ad_info ad_info;
		count = sprintf(buf, "%d\n",
				(bond_3ad_get_active_agg_info(bond, &ad_info))
				?  0 : ad_info.aggregator_id);
	}

	return count;
}
static DEVICE_ATTR(ad_aggregator, S_IRUGO, bonding_show_ad_aggregator, NULL);

static ssize_t bonding_show_ad_num_ports(struct device *d,
					 struct device_attribute *attr,
					 char *buf)
{
	int count = 0;
	struct bonding *bond = to_bond(d);

	if (bond->params.mode == BOND_MODE_8023AD) {
		struct ad_info ad_info;
		count = sprintf(buf, "%d\n",
				(bond_3ad_get_active_agg_info(bond, &ad_info))
				?  0 : ad_info.ports);
	}

	return count;
}
static DEVICE_ATTR(ad_num_ports, S_IRUGO, bonding_show_ad_num_ports, NULL);

static ssize_t bonding_show_ad_actor_key(struct device *d,
					 struct device_attribute *attr,
					 char *buf)
{
	int count = 0;
	struct bonding *bond = to_bond(d);

	if (bond->params.mode == BOND_MODE_8023AD) {
		struct ad_info ad_info;
		count = sprintf(buf, "%d\n",
				(bond_3ad_get_active_agg_info(bond, &ad_info))
				?  0 : ad_info.actor_key);
	}

	return count;
}
static DEVICE_ATTR(ad_actor_key, S_IRUGO, bonding_show_ad_actor_key, NULL);

static ssize_t bonding_show_ad_partner_key(struct device *d,
					   struct device_attribute *attr,
					   char *buf)
{
	int count = 0;
	struct bonding *bond = to_bond(d);

	if (bond->params.mode == BOND_MODE_8023AD) {
		struct ad_info ad_info;
		count = sprintf(buf, "%d\n",
				(bond_3ad_get_active_agg_info(bond, &ad_info))
				?  0 : ad_info.partner_key);
	}

	return count;
}
static DEVICE_ATTR(ad_partner_key, S_IRUGO, bonding_show_ad_partner_key, NULL);

static ssize_t bonding_show_ad_partner_mac(struct device *d,
					   struct device_attribute *attr,
					   char *buf)
{
	int count = 0;
	struct bonding *bond = to_bond(d);

	if (bond->params.mode == BOND_MODE_8023AD) {
		struct ad_info ad_info;
		if (!bond_3ad_get_active_agg_info(bond, &ad_info))
			count = sprintf(buf, "%pM\n", ad_info.partner_system);
	}

	return count;
}
static DEVICE_ATTR(ad_partner_mac, S_IRUGO, bonding_show_ad_partner_mac, NULL);

static struct attribute *per_bond_attrs[] = {
	&dev_attr_slaves.attr,
	&dev_attr_mode.attr,
	&dev_attr_fail_over_mac.attr,
	&dev_attr_arp_validate.attr,
	&dev_attr_arp_interval.attr,
	&dev_attr_arp_ip_target.attr,
	&dev_attr_downdelay.attr,
	&dev_attr_updelay.attr,
	&dev_attr_lacp_rate.attr,
	&dev_attr_ad_select.attr,
	&dev_attr_xmit_hash_policy.attr,
	&dev_attr_num_grat_arp.attr,
	&dev_attr_num_unsol_na.attr,
	&dev_attr_miimon.attr,
	&dev_attr_primary.attr,
	&dev_attr_use_carrier.attr,
	&dev_attr_active_slave.attr,
	&dev_attr_mii_status.attr,
	&dev_attr_ad_aggregator.attr,
	&dev_attr_ad_num_ports.attr,
	&dev_attr_ad_actor_key.attr,
	&dev_attr_ad_partner_key.attr,
	&dev_attr_ad_partner_mac.attr,
	NULL,
};

static struct attribute_group bonding_group = {
	.name = "bonding",
	.attrs = per_bond_attrs,
};

int bond_create_sysfs(void)
{
	int ret;

	ret = netdev_class_create_file(&class_attr_bonding_masters);
	 
	if (ret == -EEXIST) {
		 
		if (__dev_get_by_name(&init_net,
				      class_attr_bonding_masters.attr.name))
			pr_err("network device named %s already "
			       "exists in sysfs",
			       class_attr_bonding_masters.attr.name);
		ret = 0;
	}

	return ret;

}

void bond_destroy_sysfs(void)
{
	netdev_class_remove_file(&class_attr_bonding_masters);
}

int bond_create_sysfs_entry(struct bonding *bond)
{
	struct net_device *dev = bond->dev;
	int err;

	err = sysfs_create_group(&(dev->dev.kobj), &bonding_group);
	if (err)
		pr_emerg("eek! didn't create group!\n");

	return err;
}
 
void bond_destroy_sysfs_entry(struct bonding *bond)
{
	struct net_device *dev = bond->dev;

	sysfs_remove_group(&(dev->dev.kobj), &bonding_group);
}
