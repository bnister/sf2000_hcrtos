#include <malloc.h>
#include <linux/compiler.h>
#include <linux/printk.h>
#include <linux/bug.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include <kernel/module.h>


#define FREE_SPACE_SIZE 2

struct softnet_data softnet_data;
#if CONFIG_NETDEV_BUDGET
int netdev_budget = CONFIG_NETDEV_BUDGET;
#else
int netdev_budget = 300;
#endif

/* Called with irq disabled */
static inline void ____napi_schedule(struct softnet_data *sd,
				     struct napi_struct *napi)
{
	list_add_tail(&napi->poll_list, &sd->poll_list);
	raise_softirq_irqoff(NET_RX_SOFTIRQ);
}

void __napi_schedule(struct napi_struct *n)
{
	taskENTER_CRITICAL();
	____napi_schedule(&softnet_data, n);
	taskEXIT_CRITICAL();
}

void __napi_complete(struct napi_struct *n)
{
	BUG_ON(!test_bit(NAPI_STATE_SCHED, &n->state));

	list_del_init(&n->poll_list);
	smp_llsc_mb();
	clear_bit(NAPI_STATE_SCHED, &n->state);
}

void napi_disable(struct napi_struct *n)
{
	might_sleep();
	set_bit(NAPI_STATE_DISABLE, &n->state);

	while (test_and_set_bit(NAPI_STATE_SCHED, &n->state))
		msleep(1);
	while (test_and_set_bit(NAPI_STATE_NPSVC, &n->state))
		msleep(1);

	clear_bit(NAPI_STATE_DISABLE, &n->state);
}

void netif_napi_del(struct napi_struct *napi)
{
	list_del_init(&napi->dev_list);
}

void napi_complete_done(struct napi_struct *n, int work_done)
{
	/*
	 * don't let napi dequeue from the cpu poll list
	 * just in case its running on a different cpu
	 */
	if (unlikely(test_bit(NAPI_STATE_NPSVC, &n->state)))
		return;

	if (likely(list_empty(&n->poll_list))) {
		WARN_ON_ONCE(!test_and_clear_bit(NAPI_STATE_SCHED, &n->state));
	} else {
		/* If n->poll_list is not empty, we need to mask irqs */
		taskENTER_CRITICAL();
		__napi_complete(n);
		taskEXIT_CRITICAL();
	}
}

static int napi_poll(struct napi_struct *n, struct list_head *repoll)
{
	int work, weight;

	list_del_init(&n->poll_list);

	weight = n->weight;

	/* This NAPI_STATE_SCHED test is for avoiding a race
	 * with netpoll's poll_napi().  Only the entity which
	 * obtains the lock and sees NAPI_STATE_SCHED set will
	 * actually make the ->poll() call.  Therefore we avoid
	 * accidentally calling ->poll() when NAPI is not scheduled.
	 */
	work = 0;
	if (test_bit(NAPI_STATE_SCHED, &n->state)) {
		work = n->poll(n, weight);
	}

	if (likely(work < weight))
		goto out_unlock;

	/* Drivers must not modify the NAPI state if they
	 * consume the entire weight.  In such cases this code
	 * still "owns" the NAPI instance and therefore can
	 * move the instance around on the list at-will.
	 */
	if (unlikely(napi_disable_pending(n))) {
		napi_complete(n);
		goto out_unlock;
	}

	/* Some drivers may have called napi_schedule
	 * prior to exhausting their budget.
	 */
	if (unlikely(!list_empty(&n->poll_list))) {
		pr_warn_once("%s: Budget exhausted after napi rescheduled\n",
			     n->dev ? n->dev->name : "backlog");
		goto out_unlock;
	}

	list_add_tail(&n->poll_list, repoll);

out_unlock:

	return work;
}

static void net_rx_action(struct softirq_action *h)
{
	struct softnet_data *sd = &softnet_data;
	unsigned long time_limit = jiffies + 2;
	int budget = netdev_budget;
	LIST_HEAD(list);
	LIST_HEAD(repoll);

	taskENTER_CRITICAL();
	list_splice_init(&sd->poll_list, &list);
	taskEXIT_CRITICAL();

	for (;;) {
		struct napi_struct *n;

		if (list_empty(&list)) {
			if (list_empty(&repoll))
				return;
			break;
		}

		n = list_first_entry(&list, struct napi_struct, poll_list);
		budget -= napi_poll(n, &repoll);

		/* If softirq window is exhausted then punt.
		 * Allow this to run for 2 jiffies since which will allow
		 * an average latency of 1.5/HZ.
		 */
		if (unlikely(budget <= 0 ||
			     time_after_eq(jiffies, time_limit))) {
			break;
		}
	}

	taskENTER_CRITICAL();

	list_splice_tail_init(&sd->poll_list, &list);
	list_splice_tail(&repoll, &list);
	list_splice(&list, &sd->poll_list);
	if (!list_empty(&sd->poll_list))
		raise_softirq_irqoff(NET_RX_SOFTIRQ);

	taskEXIT_CRITICAL();
}

struct net_device *alloc_etherdev(int sizeof_priv)
{
	struct net_device *p;
	size_t alloc_size;

	alloc_size = sizeof(struct net_device);
	if (sizeof_priv) {
		/* ensure 32-byte alignment of private area */
		alloc_size = ALIGN(alloc_size, NETDEV_ALIGN);
		alloc_size += sizeof_priv;
	}
	p = memalign(NETDEV_ALIGN, alloc_size);
	if (p) {
		memset(p, 0, alloc_size);
		INIT_LIST_HEAD(&p->napi_list);
	}

	return p;
}

void netif_napi_add(struct net_device *dev, struct napi_struct *napi,
		    int (*poll)(struct napi_struct *, int), int weight)
{
	INIT_LIST_HEAD(&napi->poll_list);
	napi->poll = poll;
	napi->weight = weight;
	list_add(&napi->dev_list, &dev->napi_list);
	napi->dev = dev;
	set_bit(NAPI_STATE_SCHED, &napi->state);
}

int dev_alloc_name(struct net_device *dev, const char *name)
{
	strncpy(dev->name,  name, IFNAMSIZ - FREE_SPACE_SIZE);
	dev->name[IFNAMSIZ - FREE_SPACE_SIZE] = '\0';
	return 0;
}



#include <stdlib.h>
__attribute__((weak)) void add_device_randomness(const void *buf, unsigned int size)
{
	unsigned int *ptr = (unsigned int *)buf;
	for(unsigned int index = 0; index < size; index++)
		ptr[index] = random() & 0xff;
}


/**
 *	dev_set_mac_address - Change Media Access Control Address
 *	@dev: device
 *	@sa: new address
 *
 *	Change the hardware (MAC) address of the device
 */
int dev_set_mac_address(struct net_device *dev, struct sockaddr *sa)
{
	const struct net_device_ops *ops = dev->netdev_ops;
	int err;

	if (!ops->ndo_set_mac_address)
		return -EOPNOTSUPP;
	if (sa->sa_family != dev->type)
		return -EINVAL;
	if (!netif_device_present(dev))
		return -ENODEV;
	err = ops->ndo_set_mac_address(dev, sa);
	if (err)
		return err;

	// dev->addr_assign_type = NET_ADDR_SET;
	// call_netdevice_notifiers(NETDEV_CHANGEADDR, dev);
	add_device_randomness(dev->dev_addr, dev->addr_len);
	return 0;
}


static int net_dev_init(void)
{
	struct softnet_data *sd = &softnet_data;

	INIT_LIST_HEAD(&sd->poll_list);

	open_softirq(NET_RX_SOFTIRQ, net_rx_action);

	return 0;
}



module_system(netsubsys, net_dev_init, NULL, 1);
