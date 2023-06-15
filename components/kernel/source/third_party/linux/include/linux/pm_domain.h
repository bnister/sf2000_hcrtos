#ifndef _LINUX_PM_DOMAIN_H
#define _LINUX_PM_DOMAIN_H

static inline int dev_pm_domain_attach(struct device *dev, bool power_on)
{
	return -ENODEV;
}

static inline void dev_pm_domain_detach(struct device *dev, bool power_off)
{
}

#endif

