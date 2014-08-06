#include <linux/module.h>
#include <linux/types.h>
#include <linux/tty.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>

#ifdef CONFIG_MOT_FEAT_TD_BP_CTRL
#include <linux/td_bp_ctrl.h>
#endif

#include "ts27010.h"
#include "ts27010_mux.h"
#include "ts27010_ringbuf.h"
#include "ts27010_misc.h"

struct ts27010_ldisc_data {
	struct ts27010_ringbuf		*rbuf;
	struct delayed_work		recv_work;
	spinlock_t			recv_lock; 
	struct mutex			send_lock; 
	atomic_t 			ref_count;
};

#ifdef PROC_DEBUG_MUX_STAT
int g_nStatUARTDrvIO;
static int s_nDrvRecved;
static int s_nUARTRecved;
static int s_nDrvSent;

void ts27010_ldisc_uart_drv_stat_clear(void)
{
	s_nDrvSent = 0;
	s_nDrvRecved = 0;
	s_nUARTRecved = 0;
}

void  ts27010_ldisc_uart_drv_stat(void)
{
	mux_print(MSG_WARNING, "Drv Sent: %d, Drv Recved: %d, UART Recved: %d\n",
		s_nDrvSent, s_nDrvRecved, s_nUARTRecved);
}
#endif

#define RECV_RUNNING 0
#define MUX_UART_RESERVE_BUFFERSIZE (4 * DEF_TS0710_MTU)

static unsigned long s_mux_recv_flags;
static int s_bFC_UART;
static struct ts27010_ldisc_data *s_ld;

static void ts27010_ldisc_recv_worker(struct work_struct *work)
{
	int left;
	unsigned long flags;
	FUNC_ENTER();

	if (test_and_set_bit(RECV_RUNNING, &s_mux_recv_flags)) {
		mux_print(MSG_WARNING, "another recv_work is running\n");
		queue_delayed_work(g_mux_uart_queue, &s_ld->recv_work,
			msecs_to_jiffies(20));
		return;
	}
	
	ts27010_mux_uart_recv(s_ld->rbuf);

	spin_lock_irqsave(&s_ld->recv_lock, flags);
	left = ts27010_ringbuf_room(s_ld->rbuf);
	spin_unlock_irqrestore(&s_ld->recv_lock, flags);
	if (s_bFC_UART && left > MUX_UART_RESERVE_BUFFERSIZE) {
		mux_print(MSG_WARNING, "MUX stop flow control UART\n");
		s_bFC_UART = 0;
		if (ts27010mux_uart_tty && ts27010mux_uart_tty->driver
			&& ts27010mux_uart_tty->driver->ops
			&& ts27010mux_uart_tty->driver->ops->unthrottle)
			ts27010mux_uart_tty->driver->ops->unthrottle(
				ts27010mux_uart_tty);
	}

	clear_bit(RECV_RUNNING, &s_mux_recv_flags);

	FUNC_EXIT();
}

int ts27010_ldisc_uart_send(struct tty_struct *tty, u8 *data, int len)
{
	int sent = 0;
	int n;
	int i2 = 0;
	int (*tty_write)(struct tty_struct *, const u8*, int) = NULL;
	FUNC_ENTER();

	if (tty == NULL || tty->disc_data == NULL) {
		mux_print(MSG_ERROR,
			"try to send mux data while ttyHS3 is closed.\n");
		return -ENODEV;
	} else {
		tty_write = tty->driver->ops->write;
	}
	WARN_ON(tty->disc_data != s_ld);
	WARN_ON(tty != ts27010mux_uart_tty);

	
#ifdef TS27010_NET
	{
		int i = 0;
		int j = 15;
		if (len < j)
			j = (len / 16) * 16 - 1;
		mux_print(MSG_MSGDUMP, "Data length: %d bytes\n", len);
		for (i = 0; i <= j; i = i + 16) {
			mux_print(MSG_MSGDUMP, "Data %03d - %03d: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
					i, i + 15, *(data + i + 0), *(data + i + 1), *(data + i + 2), *(data + i + 3),
					*(data + i + 4), *(data + i + 5), *(data + i + 6), *(data + i + 7),
					*(data + i + 8), *(data + i + 9), *(data + i + 10), *(data + i + 11),
					*(data + i + 12), *(data + i +13), *(data + i + 14), *(data + i + 15));
		}
	}
#endif
	

	mutex_lock(&s_ld->send_lock);
	while (sent < len && ts27010mux_uart_tty) {
		
		mux_print(MSG_DEBUG, " %d: tx %d byte to BP\n", ++i2, len - sent);
		
		n = tty_write(tty, data + sent, len - sent);
		if (n == 0) {
			mux_print(MSG_WARNING, "write uart return 0, there may"
				"be a flow control, retry after 50ms\n");
			msleep_interruptible(50);
			if (signal_pending(current)) {
				mux_print(MSG_WARNING, "but got signal\n");
				break;
			}
			continue;
		} else if (n < 0) {
			mux_print(MSG_ERROR, "write uart failed: %d-%d-%d\n",
				n, sent, len);
			mux_uart_hexdump(MSG_ERROR, "send frame failed",
				__func__, __LINE__, data, len);
			break;
		} else if (n < len) {
			mux_print(MSG_WARNING, "partially write %d\n", n);
		}
		sent += n;
	}
	mutex_unlock(&s_ld->send_lock);
	if (!ts27010mux_uart_tty) {
		mux_print(MSG_ERROR, "try to send mux data "
			"while ttyHS3 is closed.\n");
		mux_print(MSG_ERROR, "sent = %d\n", sent);
		mux_uart_hexdump(MSG_ERROR, "send frame failed",
			__func__, __LINE__, data, len);
		
	}
#ifdef PROC_DEBUG_MUX_STAT
	if (g_nStatUARTDrvIO)
		s_nDrvSent += sent;
#endif

#ifdef DUMP_FRAME
	if (g_mux_uart_dump_seq)
#ifdef TS27010_UART_RETRAN
		mux_print(MSG_INFO, "tx %d to BP, sn(0x%x)\n",
			sent, data[SEQUENCE_OFFSET]);
#else
		mux_print(MSG_INFO, "tx %d to BP\n", sent);
#endif 
	else
		mux_print(MSG_DEBUG, "tx %d to BP\n", sent);
#endif 
	FUNC_EXIT();
	return sent;
}

static int ts27010_ldisc_open(struct tty_struct *tty)
{
	int err = 0;
	FUNC_ENTER();

	if (ts27010mux_uart_tty) {
		atomic_inc(&s_ld->ref_count);
		mux_print(MSG_WARNING, "ldisc re-open by process %s(%d)\n",
			current->comm, current->pid);
		return 0;
	}
	if (tty && tty->driver) {
		mux_print(MSG_INFO, "(%s:%d) set driver_name: %s, "
			"driver: %s to mux uart ldisc\n",
			current->comm, current->pid,
			tty->driver->driver_name, tty->driver->name);
	} else {
		mux_print(MSG_ERROR, "no uart tty devices %x:%x\n",
			(unsigned int)tty,
			(unsigned int)(tty ? tty->driver : NULL));
		return -ENODEV;
	}
	

	err = ts27010_mux_uart_mux_open();
	if (err)
		goto err1;
	tty->disc_data = s_ld;
	atomic_inc(&s_ld->ref_count);

	
	ts27010mux_uart_tty = tty;

	FUNC_EXIT();
	return 0;

err1:
	tty->disc_data = NULL;
	return err;
}

static void ts27010_ldisc_close(struct tty_struct *tty)
{
	FUNC_ENTER();

	atomic_dec(&s_ld->ref_count);
	if (atomic_read(&s_ld->ref_count) > 0) {
		mux_print(MSG_WARNING,
			"ldisc close non-zero refcount: process %s(%d)\n",
			current->comm, current->pid);
		return;
	}
	if (!ts27010mux_uart_tty) {
		atomic_set(&s_ld->ref_count, 0);
		mux_print(MSG_WARNING, "ldisc re-close by process %s(%d)\n",
			current->comm, current->pid);
		return;
	}
	if (tty && tty->driver) {
		mux_print(MSG_INFO, "driver_name: %s, "
			"driver: %s close mux uart ldisc\n",
			tty->driver->driver_name, tty->driver->name);
	}

	
	
	ts27010mux_uart_tty = NULL;
	atomic_set(&s_ld->ref_count, 0);

	ts27010_mux_uart_mux_close();
	mux_print(MSG_INFO, "process %s(%d) close MUX line discipline\n",
		current->comm, current->pid);

	tty->disc_data = NULL;
	
	FUNC_EXIT();
}

static int ts27010_ldisc_hangup(struct tty_struct *tty)
{
	FUNC_ENTER();
	
	ts27010_ldisc_close(tty);
	FUNC_EXIT();
	return 0;
}

static ssize_t ts27010_ldisc_read(struct tty_struct *tty, struct file *file,
				   unsigned char __user *buf, size_t count)
{
	FUNC_ENTER();
	return -EAGAIN;
}

static ssize_t ts27010_ldisc_write(struct tty_struct *tty, struct file *file,
				   const unsigned char *buf, size_t count)
{
	FUNC_ENTER();
	return -EAGAIN;
}

static int ts27010_ldisc_ioctl(struct tty_struct *tty, struct file *file,
				unsigned int cmd, unsigned long arg)
{
	int err;
	FUNC_ENTER();

	switch (cmd) {
	default:
		
		err = tty_mode_ioctl(tty, file, cmd, arg);
	}

	FUNC_EXIT();
	return err;
}

static unsigned int ts27010_ldisc_poll(struct tty_struct *tty,
				       struct file *file,
				       poll_table *wait)
{
	FUNC_ENTER();
	return 0;
}

#if !defined(UART_LOOP) && !defined(MUX_UART_UT)
static
#endif
void ts27010_ldisc_uart_receive(struct tty_struct *tty,
				  const unsigned char *data,
				  char *cflags, int count)
{
	int n;
	int left;
	unsigned long flags;
	FUNC_ENTER();
	
#ifdef TS27010_NET
	{
		int i = 0;
		int j = 15;
		if (count < j)
			j = (count / 16) * 16 -1;
		mux_print(MSG_MSGDUMP, "Data length: %d bytes\n", count);
		for (i = 0; i <= j; i = i + 16) {
			mux_print(MSG_MSGDUMP, "Data %03d - %03d: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
				i, i + 15, *(data + i + 0), *(data + i + 1), *(data + i + 2), *(data + i + 3),
				*(data + i + 4), *(data + i + 5), *(data + i + 6), *(data + i + 7),
				*(data + i + 8), *(data + i + 9), *(data + i + 10), *(data + i + 11),
				*(data + i + 12), *(data + i +13), *(data + i + 14), *(data + i + 15));
		}
	}
#endif
	

	WARN_ON(count == 0);
	if (count == 0) {
		mux_print(MSG_ERROR, "receive 0 data\n");
		return;
	}
	WARN_ON(s_ld == NULL);
	if (s_ld == NULL) {
		mux_print(MSG_ERROR, "no ldisc data allocated\n");
		return;
	}
	WARN_ON(s_ld->rbuf == NULL);
	if (s_ld->rbuf == NULL) {
		mux_print(MSG_ERROR, "no ring buffer allocated\n");
		return;
	}
	WARN_ON(tty != ts27010mux_uart_tty);

#ifdef PROC_DEBUG_MUX_STAT
	if (g_nStatUARTDrvIO)
		s_nUARTRecved += count;
#endif
	
#ifdef DUMP_FRAME
	if (g_mux_uart_dump_seq)
		mux_print(MSG_INFO, "rx %d from BP\n", count);
	else
		mux_print(MSG_DEBUG, "rx %d from BP\n", count);
#endif
	spin_lock_irqsave(&s_ld->recv_lock, flags);
	n = ts27010_ringbuf_write(s_ld->rbuf, data, count);
#ifdef PROC_DEBUG_MUX_STAT
	if (g_nStatUARTDrvIO)
		s_nDrvRecved += n;
#endif
	left = ts27010_ringbuf_room(s_ld->rbuf);
	spin_unlock_irqrestore(&s_ld->recv_lock, flags);
	if (left <= MUX_UART_RESERVE_BUFFERSIZE) {
		mux_print(MSG_WARNING, "MUX start flow control UART\n");
		s_bFC_UART = 1;
		
		if (tty && tty->driver && tty->driver->ops
			&& tty->driver->ops->throttle)
			tty->driver->ops->throttle(tty);
	}

	if (n < count) {
		mux_print(MSG_ERROR, "*** buffer overrun. "
			"receive %d, save %d, missing %d!\n",
			count, n, count - n);
		mux_uart_hexdump(MSG_WARNING, "dump buf",
			__func__, __LINE__, data, count);
		
	}

#ifdef MUX_UART_LOGGER
	ts27010_mux_uart_logger_logdata(
		g_mux_uart_logger, (u8 *)data, count, 0);
#endif
#ifdef DUMP_FRAME
	if (g_mux_uart_dump_frame)
		mux_uart_hexdump(MSG_ERROR, "dump recv data",
			__func__, __LINE__, data, count);
	else
		mux_uart_hexdump(MSG_MSGDUMP, "dump recv data",
			__func__, __LINE__, data, count);
#endif
	queue_delayed_work(g_mux_uart_queue, &s_ld->recv_work, 0);

	FUNC_EXIT();
}

static void ts27010_ldisc_wakeup(struct tty_struct *tty)
{
	FUNC_ENTER();
	mux_print(MSG_INFO, "Enter into ts27010mux_tty_wakeup.\n");
	FUNC_EXIT();
}

static struct tty_ldisc_ops ts27010_ldisc = {
	.owner  = THIS_MODULE,
	.magic	= TTY_LDISC_MAGIC,
	.name	= "n_ts27010",
	.open	= ts27010_ldisc_open,
	.close	= ts27010_ldisc_close,
	.hangup	= ts27010_ldisc_hangup,
	.read	= ts27010_ldisc_read,
	.write	= ts27010_ldisc_write,
	.ioctl	= ts27010_ldisc_ioctl,
	.poll	= ts27010_ldisc_poll,
	.receive_buf = ts27010_ldisc_uart_receive,
	.write_wakeup = ts27010_ldisc_wakeup,
};

#ifdef CONFIG_MOT_FEAT_TD_BP_CTRL
static void ts27010_mux_uart_on_wdi_intr(void)
{
	FUNC_ENTER();
	mux_print(MSG_INFO, "mux got bp reset\n");
	if (ts27010mux_uart_tty) {
		mux_print(MSG_INFO, "hangup called\n");
		ts27010_ldisc_hangup(ts27010mux_uart_tty);
	}
	FUNC_EXIT();
}

static struct td_bp_ext_interface s_mux_uart_intf = {
	.name = "ts27010_mux_uart_ctrl",
	.on_wdi_interrupt =  ts27010_mux_uart_on_wdi_intr,
};
#endif

int ts27010_ldisc_uart_init(void)
{
	int err;
	FUNC_ENTER();

#ifdef PROC_DEBUG_MUX_STAT
	g_nStatUARTDrvIO = 0;
	s_nDrvRecved = 0;
	s_nUARTRecved = 0;
	s_nDrvSent = 0;
#endif

	s_mux_recv_flags = 0;
	s_bFC_UART = 0;
	s_ld = NULL;

	s_ld = kzalloc(sizeof(*s_ld), GFP_KERNEL);
	if (s_ld == NULL) {
		mux_print(MSG_ERROR, "alloc ts27010_ldisc_data failed\n");
		err = -ENOMEM;
		goto err0;
	}

	s_ld->rbuf = ts27010_ringbuf_alloc(LDISC_BUFFER_SIZE);
	if (s_ld->rbuf == NULL) {
		err = ENOMEM;
		goto err1;
	}

	mutex_init(&s_ld->send_lock);
	s_ld->recv_lock = __SPIN_LOCK_UNLOCKED(s_ld->recv_lock);
	INIT_DELAYED_WORK(&s_ld->recv_work, ts27010_ldisc_recv_worker);
	atomic_set(&s_ld->ref_count, 0);

#ifdef CONFIG_MOT_FEAT_TD_BP_CTRL
	err = td_bp_register_ext_interface(&s_mux_uart_intf);
	if (err)
		mux_print(MSG_ERROR, "register interface to td_ctrl failed\n");
#endif

	err = tty_register_ldisc(N_TS2710, &ts27010_ldisc);
	if (err < 0)
		mux_print(MSG_ERROR,
			"ts27010: unable to register line discipline\n");
	FUNC_EXIT();
	return err;

err1:
	kfree(s_ld);
	s_ld = NULL;
err0:
	return err;
}

void ts27010_ldisc_uart_remove(void)
{
	FUNC_ENTER();
	tty_unregister_ldisc(N_TS2710);
	ts27010_ringbuf_free(s_ld->rbuf);
	kfree(s_ld);
	FUNC_EXIT();
}
