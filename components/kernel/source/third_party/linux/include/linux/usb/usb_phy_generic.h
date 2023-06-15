#ifndef __LINUX_USB_NOP_XCEIV_H
#define __LINUX_USB_NOP_XCEIV_H

static inline struct platform_device *usb_phy_generic_register(void)
{
	return NULL;
}

static inline void usb_phy_generic_unregister(struct platform_device *pdev)
{
}

#endif /* __LINUX_USB_NOP_XCEIV_H */
