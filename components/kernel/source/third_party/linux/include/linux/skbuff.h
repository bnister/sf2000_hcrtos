#ifndef _LINUX_SKBUFF_H
#define _LINUX_SKBUFF_H

#include <linux/gfp.h>
#include <linux/bug.h>
#include <linux/spinlock.h>
#include <linux/netdev_features.h>

/* Don't change this without changing skb_csum_unnecessary! */
#define CHECKSUM_NONE		0
#define CHECKSUM_UNNECESSARY	1
#define CHECKSUM_COMPLETE	2
#define CHECKSUM_PARTIAL	3

struct net_device;

typedef unsigned char *sk_buff_data_t;

enum {
	SKB_GSO_TCPV4 = 1 << 0,
	SKB_GSO_UDP = 1 << 1,

	/* This indicates the skb is from an untrusted source. */
	SKB_GSO_DODGY = 1 << 2,

	/* This indicates the tcp segment has CWR set. */
	SKB_GSO_TCP_ECN = 1 << 3,

	SKB_GSO_TCPV6 = 1 << 4,

	SKB_GSO_FCOE = 1 << 5,

	SKB_GSO_GRE = 1 << 6,

	SKB_GSO_GRE_CSUM = 1 << 7,

	SKB_GSO_IPIP = 1 << 8,

	SKB_GSO_SIT = 1 << 9,

	SKB_GSO_UDP_TUNNEL = 1 << 10,

	SKB_GSO_UDP_TUNNEL_CSUM = 1 << 11,

	SKB_GSO_TUNNEL_REMCSUM = 1 << 12,
};

struct skb_shared_info {
	unsigned char   nr_frags;
	unsigned short  gso_type;
};

struct sk_buff;
struct sk_buff_head {
	/* These two members must be first. */
	struct sk_buff *next;
	struct sk_buff *prev;

	__u32 qlen;
	spinlock_t lock;
};

struct sk_buff {
	struct sk_buff		*next;
	struct sk_buff		*prev;

	struct net_device	*dev;

	struct skb_shared_info	shared_info;
	__be16			protocol;

	__u8			ip_summed:2;
	__u8			ooo_okay:1;
	__u8			l4_hash:1;
	__u8			sw_hash:1;
	__u8			wifi_acked_valid:1;
	__u8			wifi_acked:1;
	__u8			no_fcs:1;

	unsigned int		len,
				data_len;

	sk_buff_data_t		mac_header;
	sk_buff_data_t		tail;
	sk_buff_data_t		end;
	unsigned char		*head, *data;
	unsigned int		truesize;

	unsigned char		reserved[2]; /* for ETH_PAD_SIZE */
};

struct sk_buff *alloc_skb(unsigned int size, gfp_t priority);
void __kfree_skb(struct sk_buff *skb);
void dev_kfree_skb_any(struct sk_buff *skb);
void consume_skb(struct sk_buff *skb);
#define dev_kfree_skb(a)        consume_skb(a)
#define kfree_skb(skb)		__kfree_skb(skb)

#define skb_shinfo(SKB) ((struct skb_shared_info *)(&(SKB)->shared_info))

#define SMP_CACHE_BYTES		32
#define SKB_DATA_ALIGN(X)	ALIGN(X, SMP_CACHE_BYTES)
#define SKB_WITH_OVERHEAD(X)	\
	((X) - SKB_DATA_ALIGN(sizeof(struct skb_shared_info)))

static inline unsigned char *skb_tail_pointer(const struct sk_buff *skb)
{
	return skb->tail;
}

static inline void skb_reset_tail_pointer(struct sk_buff *skb)
{
	skb->tail = skb->data;
}

static inline void skb_set_tail_pointer(struct sk_buff *skb, const int offset)
{
	skb->tail = skb->data + offset;
}

static inline unsigned char *skb_end_pointer(const struct sk_buff *skb)
{
	return skb->end;
}

static inline void skb_reset_mac_header(struct sk_buff *skb)
{
	skb->mac_header = skb->data;
}

static inline bool skb_is_nonlinear(const struct sk_buff *skb)
{
	return skb->data_len;
}

static inline int skb_tailroom(const struct sk_buff *skb)
{
	return skb_is_nonlinear(skb) ? 0 : skb->end - skb->tail;
}

static inline unsigned char *skb_mac_header(const struct sk_buff *skb)
{
	return skb->mac_header;
}

struct sk_buff *__netdev_alloc_skb(struct net_device *dev, unsigned int length,
				   gfp_t gfp_mask);
static inline struct sk_buff *netdev_alloc_skb(struct net_device *dev,
					       unsigned int length)
{
	return __netdev_alloc_skb(dev, length, GFP_ATOMIC);
}



unsigned char *skb_put(struct sk_buff *skb, unsigned int len);
#define skb_is_gso(skb) (0)

static inline struct sk_buff *skb_peek(const struct sk_buff_head *list_)
{
	struct sk_buff *list = ((const struct sk_buff *)list_)->next;

	if (list == (struct sk_buff *)list_)
		list = NULL;
	return list;
}

static inline void __skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
	struct sk_buff *next, *prev;

	list->qlen--;
	next = skb->next;
	prev = skb->prev;
	skb->next = skb->prev = NULL;
	next->prev = prev;
	prev->next = next;
}

struct sk_buff *skb_dequeue(struct sk_buff_head *list);
static inline struct sk_buff *__skb_dequeue(struct sk_buff_head *list)
{
	struct sk_buff *skb = skb_peek(list);

	if (skb)
		__skb_unlink(skb, list);
	return skb;
}

static inline struct sk_buff *__dev_alloc_skb(unsigned int length,
					      gfp_t gfp_mask)
{
	return __netdev_alloc_skb(NULL, length, gfp_mask);
}

static inline void skb_reserve(struct sk_buff *skb, int len)
{
	skb->data += len;
	skb->tail += len;
}


/**
 *	__skb_queue_purge - empty a list
 *	@list: list to empty
 *
 *	Delete all buffers on an &sk_buff list. Each buffer is removed from
 *	the list and one reference dropped. This function does not take the
 *	list lock and the caller must hold the relevant locks to use it.
 */
// void skb_queue_purge(struct sk_buff_head *list);
static inline void skb_queue_purge(struct sk_buff_head *list)
{
	struct sk_buff *skb;
	while ((skb = __skb_dequeue(list)) != NULL)
		kfree_skb(skb);
}


/**
 *	__skb_queue_head_init - initialize non-spinlock portions of sk_buff_head
 *	@list: queue to initialize
 *
 *	This initializes only the list and queue length aspects of
 *	an sk_buff_head object.  This allows to initialize the list
 *	aspects of an sk_buff_head without reinitializing things like
 *	the spinlock.  It can also be used for on-stack sk_buff_head
 *	objects where the spinlock is known to not be used.
 */
static inline void __skb_queue_head_init(struct sk_buff_head *list)
{
	list->prev = list->next = (struct sk_buff *)list;
	list->qlen = 0;
}

/*
 * This function creates a split out lock class for each invocation;
 * this is needed for now since a whole lot of users of the skb-queue
 * infrastructure in drivers have different locking usage (in hardirq)
 * than the networking core (in softirq only). In the long run either the
 * network layer or drivers should need annotation to consolidate the
 * main types of usage into 3 classes.
 */
static inline void skb_queue_head_init(struct sk_buff_head *list)
{
	spin_lock_init(&list->lock);
	__skb_queue_head_init(list);
}

static inline int skb_queue_empty(const struct sk_buff_head *list)
{
	return list->next == (const struct sk_buff *)list;
}

static inline u16 skb_get_queue_mapping(const struct sk_buff *skb)
{
	return 0;
}

static inline __u32 skb_queue_len(const struct sk_buff_head *list_)
{
	return list_->qlen;
}

static inline void __skb_insert(struct sk_buff *newsk,
				struct sk_buff *prev, struct sk_buff *next,
				struct sk_buff_head *list)
{
	newsk->next = next;
	newsk->prev = prev;
	next->prev  = prev->next = newsk;
	list->qlen++;
}

static inline void __skb_queue_before(struct sk_buff_head *list,
				      struct sk_buff *next,
				      struct sk_buff *newsk)
{
	__skb_insert(newsk, next->prev, next, list);
}

extern void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk);
static inline void __skb_queue_tail(struct sk_buff_head *list,
				   struct sk_buff *newsk)
{
	__skb_queue_before(list, (struct sk_buff *)list, newsk);
}

unsigned char *skb_push(struct sk_buff *skb, unsigned int len);
struct sk_buff *skb_copy(const struct sk_buff *skb, gfp_t priority);
struct sk_buff *skb_clone(struct sk_buff *skb, gfp_t priority);
struct sk_buff *pskb_copy(struct sk_buff *skb, gfp_t gfp_mask);
unsigned char *skb_pull(struct sk_buff *skb, unsigned int len);
static inline unsigned char *__skb_pull(struct sk_buff *skb, unsigned int len)
{
	skb->len -= len;
	BUG_ON(skb->len < skb->data_len);
	return skb->data += len;
}

static inline unsigned char *skb_pull_inline(struct sk_buff *skb, unsigned int len)
{
	return unlikely(len > skb->len) ? NULL : __skb_pull(skb, len);
}

/**
 *	skb_headroom - bytes at buffer head
 *	@skb: buffer to check
 *
 *	Return the number of bytes of free space at the head of an &sk_buff.
 */
static inline unsigned int skb_headroom(const struct sk_buff *skb)
{  
	return skb->data - skb->head;
}
static inline int skb_cloned(const struct sk_buff *skb)
{
	return 0;
}


int skb_copy_bits(const struct sk_buff *skb, int offset, void *to, int len);



#ifndef NET_IP_ALIGN
#define NET_IP_ALIGN	2
#endif


static inline struct sk_buff *__netdev_alloc_skb_ip_align(struct net_device *dev,
		unsigned int length, gfp_t gfp)
{
	struct sk_buff *skb = __netdev_alloc_skb(dev, length + NET_IP_ALIGN, gfp);

	if (NET_IP_ALIGN && skb)
		skb_reserve(skb, NET_IP_ALIGN);
	return skb;
}

static inline struct sk_buff *netdev_alloc_skb_ip_align(struct net_device *dev,
		unsigned int length)
{
	return __netdev_alloc_skb_ip_align(dev, length, GFP_ATOMIC);
}


struct sk_buff *skb_realloc_headroom(struct sk_buff *skb,
				     unsigned int headroom);

int pskb_expand_head(struct sk_buff *skb, int nhead, int ntail, gfp_t gfp_mask);


#endif
