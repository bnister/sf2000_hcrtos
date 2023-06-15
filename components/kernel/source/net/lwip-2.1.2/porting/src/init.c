#include <lwip/sys.h>
#include <lwip/opt.h>
#include <lwip/stats.h>
#include <lwip/err.h>
#include <lwip/debug.h>
#include <lwip/netif.h>
#include <lwip/netifapi.h>
#include <lwip/tcpip.h>
#include <lwip/sio.h>
#include <lwip/init.h>
#include <lwip/dhcp.h>
#include <lwip/inet.h>
#include <netif/etharp.h>
#include <kernel/completion.h>
#include <kernel/module.h>

/**
 * LwIP system initialization
 */
int lwip_system_init(void)
{
	tcpip_init(NULL, NULL);
	return 0;
}

int hcrtos_get_wifi_bw_mode(void)
{
#ifdef	CONFIG_SOC_HC15XX
	//default 5G 20MHz, 2.4G 20MHz
	return 0x00;
#else
	//default 5G 80MHz, 2.4G 20MHz
	return 0x20;
#endif
}

module_system(lwip, lwip_system_init, NULL, 0)
