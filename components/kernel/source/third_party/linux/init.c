#include <generated/br2_autoconf.h>
#include <stdlib.h>
#include <kernel/module.h>
#include <linux/kconfig.h>

extern int devices_init(void);
extern int buses_init(void);
extern int platform_bus_init(void);
static int driver_init(void)
{
	/* These are the core pieces */
	devices_init();
	buses_init();

	platform_bus_init();

	return 0;
}
extern int rcu_init(void);
extern int workqueue_init(void);
extern int softirq_init(void);
extern int spawn_ksoftirqd(void);
extern int idr_init_cache(void);

#if IS_ENABLED(CONFIG_DRV_LNXDRIVER)

int get_config_sched_lnxworkstacksize(void)
{
	return CONFIG_SCHED_LNXWORKSTACKSIZE;
}

int get_config_softirqd_hi_priority(void)
{
	return CONFIG_SCHED_SOFTIRQD_HI_PRIORITY;
}

int get_config_softirqd_hi_resched_priority(void)
{
#if IS_ENABLED(CONFIG_SCHED_SOFTIRQD_HI_RESCHED_PRIORITY)
	return CONFIG_SCHED_SOFTIRQD_HI_RESCHED_PRIORITY;
#else
	return CONFIG_SCHED_SOFTIRQD_HI_PRIORITY;
#endif
}

int get_config_softirqd_hi_resched(void)
{
#if IS_ENABLED(CONFIG_SCHED_SOFTIRQD_HI_RESCHED)
	return 1;
#else
	return 0;
#endif
}

int get_config_softirqd_priority(void)
{
	return CONFIG_SCHED_SOFTIRQD_PRIORITY;
}

int get_config_softirqd_resched_priority(void)
{
#if IS_ENABLED(CONFIG_SCHED_SOFTIRQD_RESCHED_PRIORITY)
	return CONFIG_SCHED_SOFTIRQD_RESCHED_PRIORITY;
#else
	return CONFIG_SCHED_SOFTIRQD_PRIORITY;
#endif
}

int get_config_softirqd_resched(void)
{
#if IS_ENABLED(CONFIG_SCHED_SOFTIRQD_RESCHED)
	return 1;
#else
	return 0;
#endif
}

int get_config_net_rx_softirq_in_task_only(void)
{
#if IS_ENABLED(CONFIG_NET_RX_SOFTIRQ_IN_TASK_ONLY)
	return 1;
#else
	return 0;
#endif
}

int get_config_net_tx_softirq_in_task_only(void)
{
#if IS_ENABLED(CONFIG_NET_TX_SOFTIRQ_IN_TASK_ONLY)
	return 1;
#else
	return 0;
#endif
}

int get_config_tasklet_softirq2_in_task_only(void)
{
#if IS_ENABLED(CONFIG_TASKLET_SOFTIRQ2_IN_TASK_ONLY)
	return 1;
#else
	return 0;
#endif
}

module_system(linux_driver_init, driver_init, NULL, 1)
module_system(rcu, rcu_init, NULL, 2)
module_system(idr, idr_init_cache, NULL, 2)
module_system(workqueue, workqueue_init, NULL, 0)
#endif

#if IS_ENABLED(CONFIG_DRV_SOFTIRQ)
module_system(softirqd, spawn_ksoftirqd, NULL, 0)
module_system(softirq, softirq_init, NULL, 1)
#endif
