#ifndef __AF_PACKET_H
#define __AF_PACKET_H

#include <kernel/list.h>
#include <kernel/wait.h>
#include <netpacket/packet.h>
#include <lwip/opt.h>
#include <lwip/netif.h>
#include <lwip/etharp.h>
#include <lwip/netifapi.h>
#include <lwip/pbuf.h>

struct af_packet_node {					/*  一个消息节点                */
	struct list_head	head;				/*  待接收数据链                */
	struct pbuf		*pbuf;
	u8_t			forme;				/*  发送给本机的数据包          */
	u8_t			index;				/*  接收网络接口                */
	u8_t                    outgo;				/*  是否为发送截获              */
};

struct af_packet_queue {
	struct list_head	nodes;
	size_t			total;				/*  总有效数据字节数            */
};

struct af_packet {
	struct list_head	head;
	int			flag;				/*  NONBLOCK or NOT             */
	int			type;				/*  RAW / DGRAM                 */
	int			protocol;			/*  协议                        */

#define __AF_PACKET_SHUTD_R 0x01
#define __AF_PACKET_SHUTD_W 0x02
	int			shutdflag;			/*  当前 shutdown 状态          */

	bool			recvout;			/*  是否接受输出数据包          */
	int			ifindex;			/*  绑定的接收                  */
	struct af_packet_queue	pktq;				/*  接收缓冲                    */
	size_t			maxbufsize;			/*  最大接收缓冲大小            */
	struct sockaddr_ll	saddrll;			/*  connect 信息                */
	unsigned long		recvtimeout;			/*  读取超时 tick               */
	wait_queue_head_t	wait;
};

struct af_packet *packet_socket(int domain, int type, int protocol);
int packet_close(struct af_packet *pafpacket, wait_queue_head_t *wq);
int packet_shutdown(struct af_packet *pafpacket, int how);
int packet_bind(struct af_packet *pafpacket, const struct sockaddr *name, socklen_t namelen);
int packet_listen(struct af_packet *pafpacket, int backlog);
struct af_packet *packet_accept(struct af_packet *pafpacket, struct sockaddr *addr, socklen_t *addrlen);
int packet_connect(struct af_packet *pafpacket, const struct sockaddr *name, socklen_t namelen);
ssize_t packet_recvfrom(struct af_packet *pafpacket, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
ssize_t packet_recv(struct af_packet *pafpacket, void *mem, size_t len, int flags);
ssize_t  packet_recvmsg (struct af_packet *pafpacket, struct msghdr *msg, int flags);
ssize_t packet_sendto(struct af_packet *pafpacket, const void *data, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);
ssize_t packet_sendmsg(struct af_packet *pafpacket, const struct msghdr *msg, int flags);
ssize_t packet_send(struct af_packet *pafpacket, const void *data, size_t size, int flags);
int packet_getsockname(struct af_packet *pafpacket, struct sockaddr *name, socklen_t *namelen);
int packet_getpeername(struct af_packet *pafpacket, struct sockaddr *name, socklen_t *namelen);
int packet_setsockopt(struct af_packet *pafpacket, int level, int optname, const void *optval, socklen_t optlen);
int packet_getsockopt(struct af_packet *pafpacket, int level, int optname, void *optval, socklen_t *optlen);
int packet_ioctl(struct af_packet *pafpacket, int iCmd, void *pvArg);
int packet_poll(struct af_packet *pafpacket, struct file *filep, poll_table *wait);

#endif
