#include <string.h>
#include <errno.h>
#include <net/netlink.h>

struct nlmsghdr *
__nlmsg_put(struct sk_buff *skb, u32 portid, u32 seq, int type, int len, int flags)
{
	struct nlmsghdr *nlh;
	int size = nlmsg_msg_size(len);

	nlh = (struct nlmsghdr *)skb_put(skb, NLMSG_ALIGN(size));
	nlh->nlmsg_type = type;
	nlh->nlmsg_len = size;
	nlh->nlmsg_flags = flags;
	nlh->nlmsg_pid = portid;
	nlh->nlmsg_seq = seq;
	if (!__builtin_constant_p(size) || NLMSG_ALIGN(size) - size != 0)
		memset(nlmsg_data(nlh) + len, 0, NLMSG_ALIGN(size) - size);
	return nlh;
}

struct nlattr *__nla_reserve(struct sk_buff *skb, int attrtype, int attrlen)
{
	struct nlattr *nla;

	nla = (struct nlattr *) skb_put(skb, nla_total_size(attrlen));
	nla->nla_type = attrtype;
	nla->nla_len = nla_attr_size(attrlen);

	memset((unsigned char *) nla + nla->nla_len, 0, nla_padlen(attrlen));

	return nla;
}

void __nla_put(struct sk_buff *skb, int attrtype, int attrlen,
			     const void *data)
{
	struct nlattr *nla;

	nla = __nla_reserve(skb, attrtype, attrlen);
	memcpy(nla_data(nla), data, attrlen);
}

int nla_put(struct sk_buff *skb, int attrtype, int attrlen, const void *data)
{
	if (unlikely(skb_tailroom(skb) < nla_total_size(attrlen)))
		return -EMSGSIZE;

	__nla_put(skb, attrtype, attrlen, data);
	return 0;
}

struct nlattr *nla_reserve(struct sk_buff *skb, int attrtype, int attrlen)
{
	if (unlikely(skb_tailroom(skb) < nla_total_size(attrlen)))
		return NULL;

	return __nla_reserve(skb, attrtype, attrlen);
}
