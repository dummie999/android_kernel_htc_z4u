
#ifndef __LINUX_USB_OTG_H
#define __LINUX_USB_OTG_H

#include <linux/notifier.h>

enum usb_otg_state {
	OTG_STATE_UNDEFINED = 0,

	
	OTG_STATE_B_IDLE,
	OTG_STATE_B_SRP_INIT,
	OTG_STATE_B_PERIPHERAL,

	
	OTG_STATE_B_WAIT_ACON,
	OTG_STATE_B_HOST,

	
	OTG_STATE_A_IDLE,
	OTG_STATE_A_WAIT_VRISE,
	OTG_STATE_A_WAIT_BCON,
	OTG_STATE_A_HOST,
	OTG_STATE_A_SUSPEND,
	OTG_STATE_A_PERIPHERAL,
	OTG_STATE_A_WAIT_VFALL,
	OTG_STATE_A_VBUS_ERR,
};

enum usb_otg_event {
	OTG_EVENT_DEV_CONN_TMOUT,
	OTG_EVENT_NO_RESP_FOR_HNP_ENABLE,
	OTG_EVENT_HUB_NOT_SUPPORTED,
	OTG_EVENT_DEV_NOT_SUPPORTED,
	OTG_EVENT_HNP_FAILED,
	OTG_EVENT_NO_RESP_FOR_SRP,
};

enum usb_xceiv_events {
	USB_EVENT_NONE,         
	USB_EVENT_VBUS,         
	USB_EVENT_ID,           
	USB_EVENT_CHARGER,      
	USB_EVENT_ENUMERATED,   
};

struct otg_transceiver;

struct otg_io_access_ops {
	int (*read)(struct otg_transceiver *otg, u32 reg);
	int (*write)(struct otg_transceiver *otg, u32 val, u32 reg);
};

struct otg_transceiver {
	struct device		*dev;
	const char		*label;
	unsigned int		 flags;

	u8			default_a;
	enum usb_otg_state	state;
	enum usb_xceiv_events	last_event;

	struct usb_bus		*host;
	struct usb_gadget	*gadget;

	struct otg_io_access_ops	*io_ops;
	void __iomem			*io_priv;

	
	struct atomic_notifier_head	notifier;

	
	u16			port_status;
	u16			port_change;

	
	int	(*init)(struct otg_transceiver *otg);
	void	(*shutdown)(struct otg_transceiver *otg);

	
	int	(*set_host)(struct otg_transceiver *otg,
				struct usb_bus *host);

	
	int	(*set_peripheral)(struct otg_transceiver *otg,
				struct usb_gadget *gadget);

	
	int	(*set_power)(struct otg_transceiver *otg,
				unsigned mA);

	
	int	(*set_vbus)(struct otg_transceiver *otg,
				bool enabled);

	
	int	(*set_suspend)(struct otg_transceiver *otg,
				int suspend);

	
	int	(*start_srp)(struct otg_transceiver *otg);

	
	int	(*start_hnp)(struct otg_transceiver *otg);

	
	int	(*send_event)(struct otg_transceiver *otg,
			enum usb_otg_event event);

	void	(*notify_charger)(int connect_type);

};


extern int otg_set_transceiver(struct otg_transceiver *);

#if defined(CONFIG_NOP_USB_XCEIV) || (defined(CONFIG_NOP_USB_XCEIV_MODULE) && defined(MODULE))
extern void usb_nop_xceiv_register(void);
extern void usb_nop_xceiv_unregister(void);
#else
static inline void usb_nop_xceiv_register(void)
{
}

static inline void usb_nop_xceiv_unregister(void)
{
}
#endif

static inline int otg_io_read(struct otg_transceiver *otg, u32 reg)
{
	if (otg->io_ops && otg->io_ops->read)
		return otg->io_ops->read(otg, reg);

	return -EINVAL;
}

static inline int otg_io_write(struct otg_transceiver *otg, u32 val, u32 reg)
{
	if (otg->io_ops && otg->io_ops->write)
		return otg->io_ops->write(otg, val, reg);

	return -EINVAL;
}

static inline int
otg_init(struct otg_transceiver *otg)
{
	if (otg->init)
		return otg->init(otg);

	return 0;
}

static inline void
otg_shutdown(struct otg_transceiver *otg)
{
	if (otg->shutdown)
		otg->shutdown(otg);
}

extern int otg_send_event(enum usb_otg_event event);

#ifdef CONFIG_USB_OTG_UTILS
extern struct otg_transceiver *otg_get_transceiver(void);
extern void otg_put_transceiver(struct otg_transceiver *);
extern const char *otg_state_string(enum usb_otg_state state);
#else
static inline struct otg_transceiver *otg_get_transceiver(void)
{
	return NULL;
}

static inline void otg_put_transceiver(struct otg_transceiver *x)
{
}

static inline const char *otg_state_string(enum usb_otg_state state)
{
	return NULL;
}
#endif

static inline int
otg_start_hnp(struct otg_transceiver *otg)
{
	return otg->start_hnp(otg);
}

static inline int
otg_set_vbus(struct otg_transceiver *otg, bool enabled)
{
	return otg->set_vbus(otg, enabled);
}

static inline int
otg_set_host(struct otg_transceiver *otg, struct usb_bus *host)
{
	return otg->set_host(otg, host);
}


static inline int
otg_set_peripheral(struct otg_transceiver *otg, struct usb_gadget *periph)
{
	return otg->set_peripheral(otg, periph);
}

static inline int
otg_set_power(struct otg_transceiver *otg, unsigned mA)
{
	return otg->set_power(otg, mA);
}

static inline int
otg_set_suspend(struct otg_transceiver *otg, int suspend)
{
	if (otg->set_suspend != NULL)
		return otg->set_suspend(otg, suspend);
	else
		return 0;
}

static inline int
otg_start_srp(struct otg_transceiver *otg)
{
	return otg->start_srp(otg);
}

static inline int
otg_register_notifier(struct otg_transceiver *otg, struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&otg->notifier, nb);
}

static inline void
otg_unregister_notifier(struct otg_transceiver *otg, struct notifier_block *nb)
{
	atomic_notifier_chain_unregister(&otg->notifier, nb);
}

extern int usb_bus_start_enum(struct usb_bus *bus, unsigned port_num);

#endif 
