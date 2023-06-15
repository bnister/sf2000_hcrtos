#ifndef __HDCPD_SERVICE_H__
#define __HDCPD_SERVICE_H__

typedef void (*udhcpd_lease_cb)(unsigned int yiaddr);

int udhcpd_start(udhcpd_lease_cb lease_cb);
int udhcpd_stop();

#endif
