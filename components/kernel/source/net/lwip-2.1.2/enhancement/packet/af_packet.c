#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/kmalloc.h>
#include <linux/mutex.h>
#include <lwip/opt.h>
#include <lwip/netif.h>
#include <lwip/etharp.h>
#include <lwip/netifapi.h>
#include <lwip/pbuf.h>
#include <netif/ethernet.h>
#include <kernel/list.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>

#include <string.h>
#include <nuttx/errno.h>
#include <packet/af_packet.h>

#define __AF_PACKET_BUF_DEF		(1024 * 64)			/*  默认为 64K 接收缓冲         */

static DEFINE_MUTEX(__afpacket_mutex);

#define __AF_PACKET_LOCK()          mutex_lock(&__afpacket_mutex)
#define __AF_PACKET_UNLOCK()        mutex_unlock(&__afpacket_mutex);

#define __AF_PACKET_IS_NBIO(pafpacket, flags)                                  \
	((pafpacket->flag & O_NONBLOCK) || (flags & MSG_DONTWAIT))

struct list_head __afpackets = LIST_HEAD_INIT(__afpackets);

static struct af_packet_node  *__packetBufAlloc (size_t  stLen)
{
	struct af_packet_node *pktm;

	pktm = (struct af_packet_node *)malloc(sizeof(*pktm));
	if (pktm == NULL) {
		return (NULL);
	}

	pktm->pbuf = pbuf_alloc(PBUF_RAW, (u16_t)stLen, PBUF_RAM);
	if (pktm->pbuf == NULL) {
		free(pktm);
		return (NULL);
	}

	return (pktm);
}

static void __packetBufFree(struct af_packet_node *pktn)
{
	if (pktn->pbuf)
		pbuf_free(pktn->pbuf);

	free(pktn);
}

static void __packetBufFreeAll(struct af_packet *pafpacket)
{
	struct af_packet_node *pktn = NULL;
	struct af_packet_queue *pktq = &pafpacket->pktq;

	while (!list_empty(&pktq->nodes)) {
		pktn = list_first_entry(&pktq->nodes, typeof(*pktn), head);
		list_del_init(&pktn->head);
		__packetBufFree(pktn);
	}

	pktq->total = 0;
}

static struct af_packet *__packetCreate(int type, int protocol)
{
	struct af_packet *pafpacket;

	pafpacket = (struct af_packet *)zalloc(sizeof(struct af_packet));
	if (pafpacket == NULL)
		return NULL;

	pafpacket->flag			= O_RDWR;
	pafpacket->type			= type;
	pafpacket->protocol		= protocol;
	pafpacket->shutdflag		= 0;
	pafpacket->recvout		= true;
	pafpacket->ifindex		= 0;				/*  未绑定任何网络接口          */
	pafpacket->maxbufsize		= __AF_PACKET_BUF_DEF;
	pafpacket->recvtimeout		= -1;
	INIT_LIST_HEAD(&pafpacket->pktq.nodes);
	init_waitqueue_head(&pafpacket->wait);

	taskENTER_CRITICAL();
	list_add_tail(&pafpacket->head, &__afpackets);
	taskEXIT_CRITICAL();

	return pafpacket;
}

static void __packetDelete(struct af_packet *pafpacket)
{
	__AF_PACKET_LOCK();
	__packetBufFreeAll(pafpacket);
	list_del(&pafpacket->head);
	__AF_PACKET_UNLOCK();

	free(pafpacket);
}

struct af_packet *packet_socket(int domain, int type, int protocol)
{
	if (domain != AF_PACKET)
		return NULL;

	if ((type != SOCK_DGRAM) && (type != SOCK_RAW))
		return NULL;

	return __packetCreate(type, protocol);
}

int packet_close(struct af_packet *pafpacket, wait_queue_head_t *wq)
{
	list_splice_init(&pafpacket->wait.task_list, &wq->task_list);
	__packetDelete(pafpacket);
	return 0;
}

int packet_shutdown(struct af_packet *pafpacket, int how)
{
	if ((how != SHUT_RD) && (how != SHUT_WR) && (how != SHUT_RDWR)) {
		set_errno(EINVAL);
		return -1;
	}

	__AF_PACKET_LOCK();
	if (how == SHUT_RD) {
		pafpacket->shutdflag |= __AF_PACKET_SHUTD_R;
		__packetBufFreeAll(pafpacket);
		wake_up(&pafpacket->wait);
	} else if (how == SHUT_WR) { /*  关闭本地写                  */
		pafpacket->shutdflag |= __AF_PACKET_SHUTD_W;
	} else { /*  关闭本地读写                */
		pafpacket->shutdflag |= (__AF_PACKET_SHUTD_R | __AF_PACKET_SHUTD_W);
		__packetBufFreeAll(pafpacket);
		wake_up(&pafpacket->wait);
	}
	__AF_PACKET_UNLOCK();

	return 0;
}

int packet_bind(struct af_packet *pafpacket, const struct sockaddr *name, socklen_t namelen)
{
	struct netif *pnetif;
	struct sockaddr_ll *paddrll = (struct sockaddr_ll *)name;

	if (!name || (namelen < sizeof(struct sockaddr_ll))) {
		set_errno(EINVAL);
		return -1;
	}

	if (paddrll->sll_ifindex) {
		pnetif = netifapi_netif_find_by_index(paddrll->sll_ifindex);
		if (!pnetif) {
			set_errno(ENODEV);
			return -1;
		}
		if (!(pnetif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
			set_errno(ENXIO);
			return -1;
		}
	}

	pafpacket->ifindex = paddrll->sll_ifindex;

	return 0;
}

int packet_listen(struct af_packet *pafpacket, int backlog)
{
	set_errno(EOPNOTSUPP);
	return -1;
}

struct af_packet *packet_accept(struct af_packet *pafpacket, struct sockaddr *addr, socklen_t *addrlen)
{
	set_errno(EOPNOTSUPP);
	return NULL;
}

int packet_connect(struct af_packet *pafpacket, const struct sockaddr *name, socklen_t namelen)
{
	struct netif *pnetif;
	struct sockaddr_ll *paddrll = (struct sockaddr_ll *)name;

	if (!name || (namelen < sizeof(struct sockaddr_ll))) {
		set_errno(EINVAL);
		return -1;
	}

	if (paddrll->sll_hatype != ARPHRD_ETHER) {
		set_errno(EINVAL);
		return -1;
	}

	if (paddrll->sll_ifindex) {
		pnetif = netif_get_by_index(paddrll->sll_ifindex);
		if (!pnetif) {
			set_errno(ENODEV);
			return -1;
		}
		if (!(pnetif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
			set_errno(ENXIO);
			return -1;
		}
	}

	__AF_PACKET_LOCK();
	memcpy(&pafpacket->saddrll, paddrll, sizeof(struct sockaddr_ll));
	if (pafpacket->saddrll.sll_ifindex == 0)
		pafpacket->saddrll.sll_ifindex = pafpacket->ifindex;
	__AF_PACKET_UNLOCK();

	return 0;
}

static bool __packetCanRead(struct af_packet *pafpacket, int flags, size_t stLen)
{
	struct af_packet_queue *pktq = &pafpacket->pktq;

	if (list_empty(&pktq->nodes))
		return false;
	if ((flags & MSG_WAITALL) && (pktq->total < stLen))
		return false;
	return true;
}

static size_t __packetEthHeaderInfo(struct af_packet_node *pktm, struct sockaddr_ll *paddrll)
{
	struct eth_hdr *pethhdr = (struct eth_hdr *)pktm->pbuf->payload;

	if (paddrll) {
		paddrll->sll_family = AF_PACKET;
		paddrll->sll_protocol = pethhdr->type;
		paddrll->sll_ifindex = pktm->index;
		paddrll->sll_hatype = ARPHRD_ETHER;
		paddrll->sll_halen = ETHARP_HWADDR_LEN;

		if (pktm->outgo) {
			paddrll->sll_addr[0] = pethhdr->dest.addr[0];
			paddrll->sll_addr[1] = pethhdr->dest.addr[1];
			paddrll->sll_addr[2] = pethhdr->dest.addr[2];
			paddrll->sll_addr[3] = pethhdr->dest.addr[3];
			paddrll->sll_addr[4] = pethhdr->dest.addr[4];
			paddrll->sll_addr[5] = pethhdr->dest.addr[5];

		} else {
			paddrll->sll_addr[0] = pethhdr->src.addr[0];
			paddrll->sll_addr[1] = pethhdr->src.addr[1];
			paddrll->sll_addr[2] = pethhdr->src.addr[2];
			paddrll->sll_addr[3] = pethhdr->src.addr[3];
			paddrll->sll_addr[4] = pethhdr->src.addr[4];
			paddrll->sll_addr[5] = pethhdr->src.addr[5];
		}

		if (pktm->outgo) {
			paddrll->sll_pkttype = PACKET_OUTGOING;

		} else if (pethhdr->dest.addr[0] & 0x01) {
			if (eth_addr_cmp(&pethhdr->dest, &ethbroadcast)) {
				paddrll->sll_pkttype = PACKET_BROADCAST;

			} else {
				paddrll->sll_pkttype = PACKET_MULTICAST;
			}

		} else if (pktm->forme) {
			paddrll->sll_pkttype = PACKET_HOST;

		} else {
			paddrll->sll_pkttype = PACKET_OUTGOING;
		}
	}

	if (pethhdr->type == PP_HTONS(ETHTYPE_VLAN)) {
		return (ETH_HLEN + 4);
	} else {
		return (ETH_HLEN);
	}
}

static ssize_t __packetBufRecv(struct af_packet *pafpacket, void *pvBuffer,
			       size_t stMaxBytes, struct sockaddr_ll *paddrll,
			       int flags, int *msg_flags)
{
	struct af_packet_node *pktm;
	struct af_packet_queue *pktq = &pafpacket->pktq;
	u16_t usCpyOft;
	size_t stHdrLen;
	size_t stMsgLen;
	size_t stRetLen;

	if (list_empty(&pktq->nodes))
		return 0;

	pktm = list_first_entry(&pktq->nodes, typeof(*pktm), head);
	stHdrLen = __packetEthHeaderInfo(pktm, paddrll);

	if (pafpacket->type == SOCK_DGRAM) {
		stMsgLen = pktm->pbuf->tot_len - stHdrLen - ETH_PAD_SIZE;
		usCpyOft = (u16_t)(stHdrLen + ETH_PAD_SIZE);
	} else {
		stMsgLen = pktm->pbuf->tot_len - ETH_PAD_SIZE;
		usCpyOft = ETH_PAD_SIZE;
	}

	if (stMsgLen > stMaxBytes) {
		pbuf_copy_partial(pktm->pbuf, pvBuffer, (u16_t)stMaxBytes, usCpyOft);
		if (flags & MSG_TRUNC) {
			stRetLen = stMsgLen;
		} else {
			stRetLen = stMaxBytes;
		}
		if (msg_flags) {
			(*msg_flags) |= MSG_TRUNC;
		}

	} else {
		pbuf_copy_partial(pktm->pbuf, pvBuffer, (u16_t)stMsgLen, usCpyOft);
		stRetLen = stMsgLen;
	}

	if ((flags & MSG_PEEK) == 0) {
		list_del_init(&pktm->head);
		__packetBufFree(pktm);
		pktq->total -= stMsgLen; /*  更新缓冲区中的总数据        */
	}

	return ((ssize_t)stRetLen);
}

static ssize_t packet_recvfrom2(struct af_packet *pafpacket, void *mem,
				size_t len, int flags, struct sockaddr *from,
				socklen_t *fromlen, int *msg_flags)
{
	ssize_t sstTotal = 0;

	if (!mem || !len) {
		set_errno(EINVAL);
		return -1;
	}

	if (from && fromlen && (*fromlen < sizeof(struct sockaddr_ll))) {
		set_errno(EINVAL);
		return -1;
	}

	__AF_PACKET_LOCK();
	do {
		if (pafpacket->shutdflag == __AF_PACKET_SHUTD_R) {
			__AF_PACKET_UNLOCK();
			set_errno(ENOTCONN); /*  本地已经关闭                */
			return (sstTotal);
		}

		if (__packetCanRead(pafpacket, flags, len)) { /*  可以接收                    */
			sstTotal = __packetBufRecv(pafpacket, mem, len, (struct sockaddr_ll *)from, flags, msg_flags);
			if (sstTotal > 0) {
				if (fromlen) {
					*fromlen = sizeof(struct sockaddr_ll);
				}
				break;
			}
		}

		if (__AF_PACKET_IS_NBIO(pafpacket, flags)) { /*  非阻塞 IO                   */
			__AF_PACKET_UNLOCK();
			set_errno(EWOULDBLOCK); /*  需要重新读                  */
			return (sstTotal);
		}

		__AF_PACKET_UNLOCK();
		wait_event_timeout(pafpacket->wait, pafpacket->pktq.total > 0, pafpacket->recvtimeout);
		__AF_PACKET_LOCK();
	} while (1);
	__AF_PACKET_UNLOCK();

	return (sstTotal);
}

ssize_t packet_recvfrom(struct af_packet *pafpacket, void *mem, size_t len,
			int flags, struct sockaddr *from, socklen_t *fromlen)
{
	return packet_recvfrom2(pafpacket, mem, len, flags, from, fromlen, NULL);
}

ssize_t packet_recv(struct af_packet *pafpacket, void *mem, size_t len, int flags)
{
	return packet_recvfrom2(pafpacket, mem, len, flags, NULL, NULL, NULL);
}

ssize_t packet_recvmsg(struct af_packet *pafpacket, struct msghdr *msg, int flags)
{
	msg->msg_flags = 0;
	msg->msg_controllen = 0;

	if (msg->msg_iovlen == 1) {
		return packet_recvfrom2(pafpacket, msg->msg_iov->iov_base, msg->msg_iov->iov_len, flags,
					 (struct sockaddr *)msg->msg_name, &msg->msg_namelen, &msg->msg_flags);

	} else {
		set_errno(EOPNOTSUPP);
		return -1;
	}

	return -1;
}

static error_t __packetEthRawSendto(struct af_packet *pafpacket,
				    const void *pvPacket, size_t stBytes, struct sockaddr_ll *psockaddrll)
{
	struct netif *pnetif;
	struct pbuf *pbuf_hdr;
	struct pbuf *pbuf_dat;
	err_t err;

	if (stBytes < ETH_HLEN || stBytes > (UINT16_MAX - ETH_PAD_SIZE)) {
		return (EMSGSIZE);
	}

	LOCK_TCPIP_CORE();
	if (psockaddrll) {
		pnetif = netif_get_by_index(psockaddrll->sll_ifindex);
	} else {
		pnetif = netif_get_by_index(pafpacket->ifindex);
	}
	if (pnetif == NULL) {
		UNLOCK_TCPIP_CORE();
		return (ENODEV);
	}
	if (!(pnetif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
		UNLOCK_TCPIP_CORE();
		return (ENXIO);
	}
	if (pnetif->mtu && (stBytes > (size_t)pnetif->mtu + ETH_HLEN)) {
		UNLOCK_TCPIP_CORE();
		return (EMSGSIZE);
	}
	if (!netif_is_up(pnetif) || !netif_is_link_up(pnetif)) {
		UNLOCK_TCPIP_CORE();
		return (ENETDOWN);
	}

#if ETH_PAD_SIZE
	pbuf_hdr = pbuf_alloc(PBUF_RAW, ETH_HLEN + ETH_PAD_SIZE, PBUF_RAM); /*  分配带有 PAD 的报头         */
	if (pbuf_hdr == NULL) {
		UNLOCK_TCPIP_CORE();
		return (ENOMEM);
	}
	memcpy(((u8_t *)pbuf_hdr->payload) + ETH_PAD_SIZE, pvPacket, ETH_HLEN);
	pbuf_dat = pbuf_alloc(PBUF_RAW, (u16_t)(stBytes - ETH_HLEN), PBUF_REF);
	if (pbuf_dat == NULL) { /*  数据引用 pbuf               */
		pbuf_free(pbuf_hdr);
		UNLOCK_TCPIP_CORE();
		return (ENOMEM);
	}
	pbuf_dat->payload = (void *)((u8_t *)pvPacket + ETH_HLEN);
	pbuf_cat(pbuf_hdr, pbuf_dat);
#else
	(void) pbuf_dat;
	pbuf_hdr = pbuf_alloc(PBUF_RAW, (u16_t)stBytes, PBUF_REF); /*  直接引用即可                */
	if (pbuf_hdr == NULL) {
		UNLOCK_TCPIP_CORE();
		return (ENOMEM);
	}
	pbuf_hdr->payload = (void *)pvPacket;
#endif
	err = pnetif->linkoutput(pnetif, pbuf_hdr);
	UNLOCK_TCPIP_CORE();

	pbuf_free(pbuf_hdr);

	if (err) {
		return (EIO);
	} else {
		return 0;
	}
}

static error_t __packetEthDgramSendto(struct af_packet *pafpacket,
				      const void *pvPacket, size_t stBytes, struct sockaddr_ll *psockaddrll)
{
	struct netif *pnetif;
	struct pbuf *pbuf_hdr;
	struct pbuf *pbuf_dat;
	struct eth_hdr *pethhdr;
	err_t err;

	if (stBytes > (UINT16_MAX - ETH_HLEN - ETH_PAD_SIZE)) {
		return (EMSGSIZE);
	}

	LOCK_TCPIP_CORE();
	pnetif = netif_get_by_index(psockaddrll->sll_ifindex);
	if (pnetif == NULL) {
		UNLOCK_TCPIP_CORE();
		return (ENODEV);
	}
	if (!(pnetif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
		UNLOCK_TCPIP_CORE();
		return (ENXIO);
	}
	if (!netif_is_up(pnetif) || !netif_is_link_up(pnetif)) {
		UNLOCK_TCPIP_CORE();
		return (ENETDOWN);
	}
	if (pnetif->mtu && (stBytes > (size_t)pnetif->mtu)) {
		UNLOCK_TCPIP_CORE();
		return (EMSGSIZE);
	}
	pbuf_hdr = pbuf_alloc(PBUF_RAW, ETH_HLEN + ETH_PAD_SIZE, PBUF_RAM); /*  分配带有 PAD 的报头         */
	if (pbuf_hdr == NULL) {
		UNLOCK_TCPIP_CORE();
		return (ENOMEM);
	}

	pethhdr = (struct eth_hdr *)pbuf_hdr->payload;
	pethhdr->dest.addr[0] = psockaddrll->sll_addr[0];
	pethhdr->dest.addr[1] = psockaddrll->sll_addr[1];
	pethhdr->dest.addr[2] = psockaddrll->sll_addr[2];
	pethhdr->dest.addr[3] = psockaddrll->sll_addr[3];
	pethhdr->dest.addr[4] = psockaddrll->sll_addr[4];
	pethhdr->dest.addr[5] = psockaddrll->sll_addr[5];

	pethhdr->src.addr[0] = pnetif->hwaddr[0];
	pethhdr->src.addr[1] = pnetif->hwaddr[1];
	pethhdr->src.addr[2] = pnetif->hwaddr[2];
	pethhdr->src.addr[3] = pnetif->hwaddr[3];
	pethhdr->src.addr[4] = pnetif->hwaddr[4];
	pethhdr->src.addr[5] = pnetif->hwaddr[5];

	pethhdr->type = psockaddrll->sll_protocol;

	pbuf_dat = pbuf_alloc(PBUF_RAW, (u16_t)stBytes, PBUF_REF);
	if (pbuf_dat == NULL) { /*  数据引用 pbuf               */
		pbuf_free(pbuf_hdr);
		UNLOCK_TCPIP_CORE();
		return (ENOMEM);
	}

	pbuf_dat->payload = (void *)pvPacket;
	pbuf_cat(pbuf_hdr, pbuf_dat);

	err = pnetif->linkoutput(pnetif, pbuf_hdr);
	UNLOCK_TCPIP_CORE();

	pbuf_free(pbuf_hdr);

	if (err) {
		return (EIO);
	} else {
		return 0;
	}
}

ssize_t packet_sendto(struct af_packet *pafpacket, const void *data,
		      size_t size, int flags, const struct sockaddr *to, socklen_t tolen)
{
	struct sockaddr_ll *psockaddrll;
	error_t err = 0;

	if (!data || !size) {
		set_errno(EINVAL);
		return -1;
	}

	if (to && (tolen >= sizeof(struct sockaddr_ll))) {
		psockaddrll = (struct sockaddr_ll *)to;

	} else {
		if ((pafpacket->type != SOCK_RAW) || (pafpacket->ifindex <= 0)) {
			set_errno(ENOTCONN);
			return -1;
		}
		psockaddrll = NULL;
	}

	if (pafpacket->shutdflag & __AF_PACKET_SHUTD_W) {
		set_errno(ESHUTDOWN);
		return -1;
	}

	if (pafpacket->type == SOCK_RAW) { /*  SOCK_RAW                    */
		if (psockaddrll && (psockaddrll->sll_hatype != ARPHRD_ETHER)) {
			set_errno(ENOTSUP);
			return -1;
		}
		err = __packetEthRawSendto(pafpacket, data, size, psockaddrll);
	} else { /*  SOCK_DGRAM                  */
//		if (psockaddrll->sll_hatype != ARPHRD_ETHER) {
//			set_errno(ENOTSUP);
//			return -1;
//		}
		err = __packetEthDgramSendto(pafpacket, data, size, psockaddrll);
	}

	if (err) {
		set_errno(err);
		return (0);
	} else {
		return ((ssize_t)size);
	}
}

ssize_t packet_sendmsg(struct af_packet *pafpacket, const struct msghdr *msg, int flags)
{
	if (msg->msg_iovlen == 1) {
		return packet_sendto(pafpacket, msg->msg_iov->iov_base, msg->msg_iov->iov_len, flags,
				     (const struct sockaddr *)msg->msg_name, msg->msg_namelen);
	} else {
		set_errno(EOPNOTSUPP);
		return -1;
	}
}

ssize_t packet_send(struct af_packet *pafpacket, const void *data, size_t size, int flags)
{
	struct sockaddr_ll *psockaddrll = &pafpacket->saddrll;
	socklen_t tolen = sizeof(struct sockaddr_ll);

	if (psockaddrll->sll_hatype != ARPHRD_ETHER) {
		psockaddrll = NULL; /*  没有设置目标                */
		tolen = 0;
	}

	return packet_sendto(pafpacket, data, size, flags, (struct sockaddr *)psockaddrll, tolen);
}

static error_t __packetEthIfInfo(int iIndex, struct sockaddr_ll *paddrll)
{
	struct netif *pnetif;

	if (iIndex < 0) {
		return (ENODEV);
	}

	LOCK_TCPIP_CORE();
	pnetif = netif_get_by_index(iIndex);
	if (pnetif == NULL) {
		UNLOCK_TCPIP_CORE();
		return (ENODEV);
	}
	if (!(pnetif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
		UNLOCK_TCPIP_CORE();
		return (ENXIO);
	}

	paddrll->sll_family = AF_PACKET;
	paddrll->sll_protocol = ETH_P_ALL;
	paddrll->sll_ifindex = iIndex;
	paddrll->sll_hatype = ARPHRD_ETHER;
	paddrll->sll_halen = ETHARP_HWADDR_LEN;
	paddrll->sll_pkttype = PACKET_HOST;

	paddrll->sll_addr[0] = pnetif->hwaddr[0];
	paddrll->sll_addr[1] = pnetif->hwaddr[1];
	paddrll->sll_addr[2] = pnetif->hwaddr[2];
	paddrll->sll_addr[3] = pnetif->hwaddr[3];
	paddrll->sll_addr[4] = pnetif->hwaddr[4];
	paddrll->sll_addr[5] = pnetif->hwaddr[5];
	UNLOCK_TCPIP_CORE();

	return 0;
}

int packet_getsockname(struct af_packet *pafpacket, struct sockaddr *name, socklen_t *namelen)
{
	error_t err;

	if (!name || !namelen || (*namelen < sizeof(struct sockaddr_ll))) {
		set_errno(EINVAL);
		return -1;
	}

	__AF_PACKET_LOCK();
	err = __packetEthIfInfo(pafpacket->ifindex, (struct sockaddr_ll *)name);
	__AF_PACKET_UNLOCK();

	if (err) {
		set_errno(err);
		return -1;
	} else {
		if (namelen) {
			*namelen = sizeof(struct sockaddr_ll);
		}
		return 0;
	}
}

int packet_getpeername(struct af_packet *pafpacket, struct sockaddr *name, socklen_t *namelen)
{
	set_errno(EOPNOTSUPP);
	return -1;
}

int packet_setsockopt(struct af_packet *pafpacket, int level, int optname, const void *optval, socklen_t optlen)
{
	set_errno(EOPNOTSUPP);
	return -1;
}

int packet_getsockopt(struct af_packet *pafpacket, int level, int optname, void *optval, socklen_t *optlen)
{
	set_errno(EOPNOTSUPP);
	return -1;
}

int packet_ioctl(struct af_packet *pafpacket, int iCmd, void *pvArg)
{
	int iRet = 0;

	switch (iCmd) {
	case F_GETFL:
		if (pvArg) {
			*(int *)pvArg = pafpacket->flag;
		}
		break;

	case F_SETFL:
		if ((int)(long)pvArg & O_NONBLOCK) {
			pafpacket->flag |= O_NONBLOCK;
		} else {
			pafpacket->flag &= ~O_NONBLOCK;
		}
		break;

	case FIONREAD:
		if (pvArg) {
			*(int *)pvArg = (int)pafpacket->pktq.total;
		}
		break;

	case FIONBIO:
		if (pvArg && *(int *)pvArg) {
			pafpacket->flag |= O_NONBLOCK;
		} else {
			pafpacket->flag &= ~O_NONBLOCK;
		}
		break;

	case SIOCGIFINDEX: {
		extern u8_t lwip_ioctl_internal_SIOCGIFINDEX(struct ifreq *ifr);
		iRet = lwip_ioctl_internal_SIOCGIFINDEX((struct ifreq *)pvArg);
		break;
	}

        case SIOCGIFHWADDR: {
		extern u8_t lwip_ioctl_internal_SIOCGIFHWADDR(struct ifreq *ifr);
		iRet = lwip_ioctl_internal_SIOCGIFHWADDR((struct ifreq *)pvArg);
		break;
	}
	default:
		set_errno(ENOSYS);
		iRet = -1;
		break;
	}

	return (iRet);
}

static void __packetBufInput(struct af_packet *pafpacket, struct pbuf *p, struct netif *netif, bool bOutgo)
{
	struct af_packet_node *pktm;
	struct af_packet_queue *pktq;
	size_t stMsgLen;
	struct eth_hdr *pethhdr;

	pktq = &pafpacket->pktq;
	pethhdr = (struct eth_hdr *)p->payload;

	if (pafpacket->type == SOCK_RAW) {
		stMsgLen = p->tot_len - ETH_PAD_SIZE;
	} else {
		if (pethhdr->type == PP_HTONS(ETHTYPE_VLAN)) {
			stMsgLen = p->tot_len - ETH_HLEN - SIZEOF_VLAN_HDR - ETH_PAD_SIZE;
		} else {
			stMsgLen = p->tot_len - ETH_HLEN - ETH_PAD_SIZE;
		}
	}

	if ((stMsgLen + pktq->total) > (pafpacket->maxbufsize)) { /*  缓冲区不够了, 丢弃          */
		return;
	}

	pktm = __packetBufAlloc((size_t)p->tot_len);
	if (pktm == NULL) {
		return; /*  内存不足                    */
	}
	pbuf_copy(pktm->pbuf, p); /*  拷贝接收数据                */

	if (memcmp(pethhdr->dest.addr, netif->hwaddr, ETHARP_HWADDR_LEN) == 0) {
		pktm->forme = 1;
	} else {
		pktm->forme = 0;
	}

	pktm->index = netif_get_index(netif);
	pktm->outgo = (u8_t)bOutgo;
	list_add_tail(&pktm->head, &pktq->nodes);
	pktq->total += stMsgLen; /*  更新缓冲区中的总数据        */
	wake_up(&pafpacket->wait);
}

static int packet_link_input(struct pbuf *p, struct netif *netif, bool bOutgo)
{
	static uint16_t usAll = 0x0300; /*  ETH_P_ALL                   */
	struct af_packet *pafpacket = NULL, *next = NULL;
	struct eth_hdr *pethhdr;
	struct eth_vlan_hdr *pethvlanhdr;
	bool bRecv;

	pethhdr = (struct eth_hdr *)p->payload;
#if 0
	if (pethhdr->type != PP_HTONS(ETHTYPE_EAPOL))
		return 0;
#endif

	if (list_empty(&__afpackets) ||
	    (p->tot_len < (ETH_HLEN + ETH_PAD_SIZE)) ||
	    !(netif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
		return (0);
	}

	if (pethhdr->type == PP_HTONS(ETHTYPE_VLAN)) {
		pethvlanhdr = (struct eth_vlan_hdr *)(((char *)pethhdr) + SIZEOF_ETH_HDR);
	} else {
		pethvlanhdr = NULL;
	}

	__AF_PACKET_LOCK();
	list_for_each_entry_safe (pafpacket, next, &__afpackets, head) {
		if ((pafpacket->ifindex > 0) &&
		    (pafpacket->ifindex != netif_get_index(netif))) { /*  不是绑定的网卡              */
			continue;
		}

		if (bOutgo && (pafpacket->recvout == false)) {
			continue;
		}

		bRecv = false;
		if (pethvlanhdr) {
			if ((pethhdr->type == (uint16_t)pafpacket->protocol) ||
			    (pethvlanhdr->tpid == (uint16_t)pafpacket->protocol) ||
			    (usAll == (uint16_t)pafpacket->protocol)) {
				bRecv = true;
			}

		} else {
			if ((pethhdr->type == (uint16_t)pafpacket->protocol) ||
			    (usAll == (uint16_t)pafpacket->protocol)) {
				bRecv = true;
			}
		}

		if (bRecv) { /*  协议匹配                    */
			__packetBufInput(pafpacket, p, netif, bOutgo);
		}
	}
	__AF_PACKET_UNLOCK();

	return (0);
}

int lwip_link_input_hook (struct pbuf *p, struct netif *pnetif)
{
	if (!netif_is_up(pnetif)
#if PPP_SUPPORT
	    && !pnetif->ppp_ref
#endif /*  PPP_SUPPORT                 */
	) {
		return (1); /*  没有使能的网卡不接收        */
	}

	return packet_link_input(p, pnetif, false);
}

int lwip_link_output_hook(struct pbuf *p, struct netif *pnetif)
{
	return packet_link_input(p, pnetif, true);
}

int packet_poll(struct af_packet *pafpacket, struct file *filep, poll_table *wait)
{
	struct af_packet_queue *pktq = &pafpacket->pktq;
	int mask = 0;

	poll_wait(filep, &pafpacket->wait, wait);

	if (pktq->total > 0)
		mask |= (POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND);

	return mask;
}
