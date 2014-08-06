
#include <net/arp.h>
#include <linux/ip.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/workqueue.h>

#include "ts27010.h"
#include "ts27010_net.h"
#include "ts27010_ringbuf.h"
#include "ts27010_mux.h"

#define NET_TX_TIMEOUT 10 
#define NET_DATA_MTU 1500 

struct ts27010_mux_net {
#ifdef ENABLE_MUX_NET_KREF_FEATURE
	struct kref					ref;
#endif
	struct dlci_struct*			dlci; 
	struct net_device_stats		stats;
	struct workqueue_struct*	net_wq;
	struct work_struct			net_work;
	struct net_device*			net;
	struct sk_buff_head			txhead;
};

#define STATS(net) (((struct ts27010_mux_net *)netdev_priv(net))->stats)


static int ts27010_mux_net_open(struct net_device *net)
{
	mux_print(MSG_MSGDUMP, "%s called\n", __func__);
	netif_start_queue(net);
	return 0;
}

static int ts27010_mux_net_close(struct net_device *net)
{
	netif_stop_queue(net);
	return 0;
}

static struct net_device_stats *ts27010_mux_net_get_stats(struct net_device *net)
{
	return &((struct ts27010_mux_net *)netdev_priv(net))->stats;
}

static void dlci_net_free(struct dlci_struct *dlci)
{
	if (!dlci->net) {
		WARN_ON(1);
		return;
	}
	free_netdev(dlci->net);
	dlci->net = NULL;
}

#ifdef ENABLE_MUX_NET_KREF_FEATURE
static void net_free(struct kref *ref)
{
	struct ts27010_mux_net *mux_net;
	struct dlci_struct *dlci;

	mux_net = container_of(ref, struct ts27010_mux_net, ref);
	dlci = mux_net->dlci;

	if (dlci->net) {
		unregister_netdev(dlci->net);
		dlci_net_free(dlci);
		mux_print(MSG_ERROR, "net was freed~\n");
	}
}

static inline void muxnet_get(struct ts27010_mux_net *mux_net)
{
	kref_get(&mux_net->ref);
}

static inline void muxnet_put(struct ts27010_mux_net *mux_net)
{
	kref_put(&mux_net->ref, net_free);
}
#endif

static void ts27010_mux_net_tx_work(struct work_struct * work)
{
	struct sk_buff *skb = NULL;
	struct net_device *net = NULL;
	struct ts27010_mux_net *mux_net = NULL;
	struct dlci_struct *dlci = NULL;
	unsigned char *data = NULL;
	int len = 0;
	int ret = 0;
	int err = 0;
	FUNC_ENTER();

	if ( work == NULL ) {
		mux_print(MSG_ERROR, "[WQ] work == NULL\n");
		goto net_tx_work_end;
	}

	mux_net = container_of(work, struct ts27010_mux_net, net_work);
	if ( mux_net == NULL ) {
		mux_print(MSG_ERROR, "[WQ] mux_net == NULL\n");
		goto net_tx_work_end;
	}

	dlci = mux_net->dlci;
	if ( dlci == NULL ) {
		mux_print(MSG_ERROR, "[WQ] dlci == NULL\n");
		goto net_tx_work_end;
	}

	net = mux_net->net;
	if ( net == NULL ) {
		mux_print(MSG_ERROR, "[WQ] net == NULL\n");
		goto net_tx_work_end;
	}

	while ((skb = skb_dequeue(&mux_net->txhead)) != NULL) {
		data = kzalloc(skb->len, GFP_ATOMIC);
		if (!data) {
			mux_print(MSG_ERROR, "[WQ] buffer kzalloc() failed\n");
			goto net_tx_work_end;
		}

		err = skb_copy_bits(skb, 0, data, skb->len);
		if (err < 0) {
			mux_print(MSG_ERROR, "[WQ] skb_copy_bits() failed - %d\n", err);
			kfree(data);
			goto net_tx_work_end;
		}

		len = skb->len;
		STATS(net).tx_packets++;
		STATS(net).tx_bytes += skb->len;
		if ( g_mux_uart_print_level != MSG_LIGHT && g_mux_uart_print_level != MSG_ERROR ) {
			mux_print(MSG_DEBUG, "[WQ] All tx %lu packets %lu bytes\n",
					STATS(net).tx_packets, STATS(net).tx_bytes);
		} else {
			mux_print(MSG_LIGHT, "[WQ] All tx %lu packets %lu bytes\n",
					STATS(net).tx_packets, STATS(net).tx_bytes);
		}

		mux_print(MSG_LIGHT, "[WQ] dlci=[%d], data=[%d] \n", dlci->line_no, len);

		if ( data != NULL && len > 0) {
			mux_print(MSG_LIGHT, "[WQ] ts27010_mux_uart_line_write+\n");
			ret = ts27010_mux_uart_line_write(dlci->line_no, data, len);
			mux_print(MSG_LIGHT, "[WQ] ts27010_mux_uart_line_write-, ret=[%d]\n", ret);
		}

		
		net->trans_start = jiffies;
		consume_skb(skb);
#ifdef ENABLE_MUX_NET_KREF_FEATURE
		muxnet_put(mux_net);
#endif
		kfree(data);
	}

net_tx_work_end:
	mux_print(MSG_LIGHT, "[WQ] net_tx_work_end\n");

	FUNC_EXIT();
	return;
}

static int ts27010_mux_net_start_xmit(struct sk_buff *skb,
										struct net_device *net)
{
	struct ts27010_mux_net *mux_net = (struct ts27010_mux_net *)netdev_priv(net);
	struct dlci_struct *dlci = mux_net->dlci;

	FUNC_ENTER();

#ifdef ENABLE_MUX_NET_KREF_FEATURE
	muxnet_get(mux_net);
#endif

	if (!skb) {
		mux_print(MSG_ERROR, "no skb\n");
		return -EINVAL;
	}

	mux_print(MSG_MSGDUMP, "ttyline = %d \n", dlci->line_no);
	mux_print(MSG_DEBUG, "net name : %s \n", net->name);
	mux_print(MSG_MSGDUMP, "net device address: %p \n", net);
	mux_print(MSG_MSGDUMP, "net->type : %d \n", net->type);
	mux_print(MSG_MSGDUMP, "skb->protocol : %x \n" , skb->protocol);
	mux_print(MSG_DEBUG, "skb data length = %d bytes\n", skb->len);

#ifdef DEBUG
	if (MSG_MSGDUMP >= g_mux_uart_print_level)
	{
		int i = 0;
		int j = 15; 
		if (skb->len < j)
			j = (skb->len / 16) * 16 - 1;
		for (i = 0; i <= j; i = i + 16) {
			mux_print(MSG_MSGDUMP, "skb data %03d - %03d: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
				i, i + 15, *(skb->data + i + 0), *(skb->data + i + 1), *(skb->data + i + 2), *(skb->data + i + 3),
				*(skb->data + i + 4), *(skb->data + i + 5), *(skb->data + i + 6), *(skb->data + i + 7),
				*(skb->data + i + 8), *(skb->data + i + 9), *(skb->data + i + 10), *(skb->data + i + 11),
				*(skb->data + i + 12), *(skb->data + i +13), *(skb->data + i + 14), *(skb->data + i + 15));
		}
	}
#endif

	if ( mux_net->net_wq != NULL ) {
		mux_net->net = net;
		skb_queue_tail(&mux_net->txhead, skb);
		queue_work(mux_net->net_wq, &mux_net->net_work);
	} else {
		mux_print(MSG_ERROR, "[WQ] mux_net->net_wq = NULL\n");
		mux_net->net = NULL;
		kfree_skb(skb);
		return -1;
	}

	FUNC_EXIT();
	return NETDEV_TX_OK;
}

static void ts27010_mux_net_tx_timeout(struct net_device *net)
{
	
	dev_dbg(&net->dev, "Tx timed out.\n");

	
	STATS(net).tx_errors++;
}

static __be16 ts27010net_ip_type_trans(struct sk_buff *skb, struct net_device *dev)
{
	__be16 protocol = 0;

	skb->dev = dev;

	
	switch (skb->data[0] & 0xf0) {
	case 0x40:
		protocol = htons(ETH_P_IP);
		break;
	case 0x60:
		protocol = htons(ETH_P_IPV6);
		break;
	default:
		mux_print(MSG_ERROR, "[%s] L3 protocol decode error: 0x%02x \n",
					dev->name, skb->data[0] & 0xf0);
		
	}
	return protocol;
}

void ts27010_mux_rx_netchar(struct dlci_struct *dlci,
							unsigned char *in_buf, int size)
{
	struct sk_buff *skb;
	struct net_device *net = dlci->net;
#ifdef ENABLE_MUX_NET_KREF_FEATURE
	struct ts27010_mux_net *mux_net = (struct ts27010_mux_net *)netdev_priv(net);
#endif
	unsigned char *dst;

	FUNC_ENTER();

#ifdef ENABLE_MUX_NET_KREF_FEATURE
	muxnet_get(mux_net);
#endif

	mux_print(MSG_DEBUG, "ttyline = %d \n", dlci->line_no);
	mux_print(MSG_DEBUG, "net name : %s \n", net->name);
	mux_print(MSG_MSGDUMP, "net device address: %p \n", net);
	mux_print(MSG_MSGDUMP, "net->type : %d \n", net->type);
	mux_print(MSG_DEBUG, "Data length = %d byte\n", size);

#ifdef DEBUG
	if (MSG_MSGDUMP >= g_mux_uart_print_level)
	{
		int i = 0;
		int j = 15;
		if (size < j)
			j = (size / 16) * 16 -1;
		for (i = 0; i <= j; i = i + 16) {
			mux_print(MSG_MSGDUMP, "Data %03d - %03d: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
				i, i + 15, *(in_buf + i + 0), *(in_buf + i + 1), *(in_buf + i + 2), *(in_buf + i + 3),
				*(in_buf + i + 4), *(in_buf + i + 5), *(in_buf + i + 6), *(in_buf + i + 7),
				*(in_buf + i + 8), *(in_buf + i + 9), *(in_buf + i + 10), *(in_buf + i + 11),
				*(in_buf + i + 12), *(in_buf + i +13), *(in_buf + i + 14), *(in_buf + i + 15));
		}
	}
#endif

	skb = dev_alloc_skb(size + NET_IP_ALIGN);

	if (skb == NULL) {
		mux_print(MSG_ERROR, "[%s] cannot allocate skb\n", net->name);
		
		STATS(net).rx_dropped++;
	} else {
		skb->dev = net;
		skb_reserve(skb, NET_IP_ALIGN);
		dst  = skb_put(skb, size);
		memcpy(dst, in_buf, size);

		
		skb->protocol = ts27010net_ip_type_trans(skb, net);
		mux_print(MSG_MSGDUMP, "skb->protocol : %x \n" , skb->protocol);
		
		netif_rx(skb);
	}

	
	STATS(net).rx_packets++;
	STATS(net).rx_bytes += size;
	if ( g_mux_uart_print_level != MSG_LIGHT && g_mux_uart_print_level != MSG_ERROR ) {
		mux_print(MSG_DEBUG, "All rx %lu packets %lu bytes\n", STATS(net).rx_packets, STATS(net).rx_bytes);
	} else {
		mux_print(MSG_LIGHT, "All rx %lu packets %lu bytes\n", STATS(net).rx_packets, STATS(net).rx_bytes);
	}
#ifdef ENABLE_MUX_NET_KREF_FEATURE
	muxnet_put(mux_net);
#endif

	FUNC_EXIT();
	return;
}

int ts27010_change_mtu(struct net_device *net, int new_mtu)
{
	if ((new_mtu < 8) || (new_mtu > NET_DATA_MTU))
		return -EINVAL;
	net->mtu = new_mtu;
	return 0;
}

static void ts27010_mux_net_init(struct net_device *net)
{
	static const struct net_device_ops ts27010_netdev_ops = {
	.ndo_open	 = ts27010_mux_net_open,
	.ndo_stop	 = ts27010_mux_net_close,
	.ndo_start_xmit	 = ts27010_mux_net_start_xmit,
	.ndo_tx_timeout	 = ts27010_mux_net_tx_timeout,
	.ndo_get_stats	 = ts27010_mux_net_get_stats,
	.ndo_change_mtu	 = ts27010_change_mtu,
	};
	net->netdev_ops = &ts27010_netdev_ops;

	
	ether_setup(net);

	
	random_ether_addr(net->dev_addr);

	net->watchdog_timeo = 1000; 

	
	net->header_ops= 0;  
	net->type		= ARPHRD_RAWIP;
	net->hard_header_len	= 0;
	net->mtu		= NET_DATA_MTU;
	net->addr_len		= 0;
	net->flags		&= ~(IFF_BROADCAST|
						IFF_MULTICAST);
}

void ts27010_destroy_network(struct dlci_struct *dlci)
{
	struct ts27010_mux_net *mux_net;

	mux_print(MSG_INFO, "destroy network interface[%d]\n", dlci->line_no);
	if (dlci->net) {
		mux_net = (struct ts27010_mux_net *)netdev_priv(dlci->net);

		if ( mux_net->net_wq != NULL ) {
			struct sk_buff *skb;

			mux_print(MSG_LIGHT, "[WQ] cancel_work_sync\n");
			cancel_work_sync(&mux_net->net_work);
			mux_print(MSG_LIGHT, "[WQ] destroy_workqueue, wq=[0x%p]\n", mux_net->net_wq);
			destroy_workqueue(mux_net->net_wq);
			mux_net->net_wq = NULL;

			while ((skb = skb_dequeue(&mux_net->txhead)) != NULL) {
				consume_skb(skb);
			}
		}

		mux_print(MSG_LIGHT, "unregister_netdev\n");
		unregister_netdev(dlci->net);
		mux_print(MSG_LIGHT, "dlci_net_free\n");
		dlci_net_free(dlci);
#ifdef ENABLE_MUX_NET_KREF_FEATURE
		kref_put(&mux_net->ref, net_free);
#endif
	}
}

int ts27010_create_network(struct dlci_struct *dlci, struct ts27010_netconfig *nc)
{
	char *netname;
	int retval = 0;
	struct net_device *net;
	struct ts27010_mux_net *mux_net;

	mux_print(MSG_DEBUG, "create network interface\n");

	netname = "pdp_ip%d";
	if (nc->if_name[0] != '\0')
		netname = nc->if_name;
	net = alloc_netdev(sizeof(struct ts27010_mux_net),
		netname,
		ts27010_mux_net_init);
	if (!net) {
		mux_print(MSG_ERROR, "alloc_netdev failed\n");
		retval = -ENOMEM;
		goto error_ret;
	}
	mux_print(MSG_DEBUG, "register netdev\n");
	mux_print(MSG_MSGDUMP, "net device address %p \n", net);
	mux_print(MSG_DEBUG, "net device mtu %d \n", net->mtu);

	retval = register_netdev(net);
	if (retval) {
		mux_print(MSG_ERROR, "network register fail %d\n", retval);
		free_netdev(net);
		goto error_ret;
	}
	dlci->net = net;
	mux_net = (struct ts27010_mux_net *)netdev_priv(net);
	mux_net->dlci = dlci;
#ifdef ENABLE_MUX_NET_KREF_FEATURE
	kref_init(&mux_net->ref);
#endif
	strncpy(nc->if_name, net->name, IFNAMSIZ); 

	skb_queue_head_init(&mux_net->txhead);

	mux_print(MSG_LIGHT, "[WQ] create_workqueue\n");
	mux_net->net_wq = create_singlethread_workqueue(net->name);
	if (!mux_net->net_wq) {
		mux_print(MSG_ERROR, "[WQ] Fail to open WQ\n");
		unregister_netdev(net);
		free_netdev(net);
		retval = -1;
		goto error_ret;
	}
	mux_print(MSG_LIGHT, "[WQ] INIT_WORK\n");
	INIT_WORK(&mux_net->net_work, ts27010_mux_net_tx_work);

	return net->ifindex;	

error_ret:
	return retval;
}
