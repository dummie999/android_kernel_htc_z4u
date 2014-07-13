/* linux/driver/mmc/card/sdio_sp6502com.c
 *
 *
 * Copyright(C) 2012 HTC Corporation.
 * Author: Bob Song <bob_song@htc.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/seq_file.h>
#include <linux/serial_reg.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/device.h>

#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/debugfs.h>
#include "sdio_sp6502com.h"

DEFINE_MUTEX(sdio_tx_lock);
DEFINE_MUTEX(sdio_write_buf_lock);
DEFINE_MUTEX(sdio_write_lock);
DEFINE_MUTEX(sdio_tty_lock);
DEFINE_MUTEX(sdio_rx_lock);
DEFINE_MUTEX(sdio_push_lock);
DEFINE_MUTEX(sdio_wakelock_lock);
DEFINE_MUTEX(sdio_irq_lock);
#ifdef	SDIO_RX_RESEND_ENABLED
DEFINE_MUTEX(sdio_reset_lock);
#endif

int push_sdio_tty_fail = 0;
int has_received_len = 0;
int modem_is_on = 1;
int sdio_removed = 0;
int begin_retry = 0;
int sdio_init_config = 0;
const char *modem_mmc_id = "msm_sdcc.4";
unsigned int tx_packet_count = 0;
unsigned int rx_packet_count = 1;
wait_queue_head_t init_wq;
int probe_finish = 0;
int bp_rdy_value = 0;
int flashless_mode = 0;

struct tty_driver *sdio_tty_driver;

#ifdef SDIO_TTY_DEBUG
static void sdio_tty_dbg_createhost(struct sdio_tty_dev *port);
static struct dentry *sdio_debugfs_dir;
static struct dentry *sdio_debugfs_file;
static int  sdio_tty_dbg_init(void);
unsigned int data_rx_count = 0;
unsigned int data_tx_count = 0;
unsigned long data_rx_size = 0;
unsigned long data_tx_size = 0;
unsigned int rts_irqcount = 0;
unsigned int rdy_irqcount = 0;
unsigned int reset_count = 0;
#endif
#ifdef SDIO_SP6502_PERF_PROFILING
ktime_t tran_start,rx_tran,tx_tran;
ktime_t total_start_rx,total_start_tx,rx_total,tx_total,rx_tty_time;
#endif
#ifdef SDIO_TX_RESEND_ENABLED
#define SDIO_SP6502_RESEND_MAX_COUNT 3
int sdio_sp6502com_resend_count = 0;
#endif

int sdio_platform_tty_throttle_status()
{
	return push_sdio_tty_fail ;
}

inline static void sp6502com_sdio_read_buf_clear(struct sdio_tty_dev *port)
{
	port->rx_data_len = 0;
}

inline static int sp6502com_sdio_write_buf_length(void)
{
	return sdio_dev->tx_data_len;
}

static void sp6502com_sdio_write_buf_clear(void)
{
	sdio_dev->tx_data_len = 0;
}

static inline int sprd_get_status(void)
{
	return sdio_dev->status;
}

static void sprd_set_status(int status)
{
	unsigned long flags;

	FUNC_ENTER();

	local_irq_save(flags);

	if (status == SDIO_VALID) {
		sdio_dev->status = status;
	} else {
		sdio_dev->status = sdio_dev->status | status;
	}
	local_irq_restore(flags);

	FUNC_EXIT();
}

static int ap_rts_set(int is_on)
{
	FUNC_ENTER();

	gpio_set_value(AP_RTS, is_on);

	sp6502com_pr_info("ap rts gpio[%d] level = %d\n", AP_RTS, is_on);

	FUNC_EXIT();

	return is_on;
}

static int ap_resend(int is_on)
{
	FUNC_ENTER();

	gpio_set_value(AP_RSD, is_on);

	sp6502com_pr_info("ap resend gpio[%d] level = %d\n", AP_RSD, is_on);

	FUNC_EXIT();

	return is_on;
}

static int bp_rts(void)
{
	int is_on;

	FUNC_ENTER();

	is_on = gpio_get_value(BP_RTS);

	sp6502com_pr_info("bp rts gpio[%d] level = %d\n", BP_RTS, is_on);

	FUNC_EXIT();

	return !is_on;
}

static int bp_rdy(void)
{
	int is_on;

	FUNC_ENTER();

	is_on = gpio_get_value(BP_RDY);

	sp6502com_pr_info("bp rdy gpio[%d] level = %d\n", BP_RDY, is_on);

	FUNC_EXIT();

	return !is_on;
}

#ifdef SDIO_RX_RESEND_ENABLED
static int ap_resend_set(int is_on)
{
	FUNC_ENTER();

	if (is_on != gpio_get_value(AP_RSD)) {
		gpio_set_value(AP_RSD, is_on);
		msleep(2);
	}

	sp6502com_pr_info("ap resend gpio[%d] level = %d\n", AP_RSD, is_on);

	FUNC_EXIT();

	return is_on;
}

static int ap_rdy_set(int is_on)
{
	FUNC_ENTER();

	gpio_set_value(AP_RDY, !is_on);

	sp6502com_pr_info("ap rdy gpio[%d] level = %d\n", AP_RDY, !is_on);

	FUNC_EXIT();

	return is_on;
}
#endif

#ifdef SDIO_TX_RESEND_ENABLED
static int bp_resend(void)
{
	int is_on;

	FUNC_ENTER();

	is_on = gpio_get_value(BP_RSD);

	sp6502com_pr_info("bp resend gpio[%d] level = %d\n", BP_RSD, is_on);

	FUNC_EXIT();

	return is_on;
}
#endif

static void buf_req_retry(unsigned long param)
{
	struct sdio_tty_dev *port = (struct sdio_tty_dev *)param;

	FUNC_ENTER();

	if (!port) {
		sp6502com_pr_err("NULL tty info\n");
		return ;
	}
	sp6502com_pr_info("require retry\n");

	FUNC_EXIT();
}

static int sdio_tty_claim_func(struct sdio_tty_dev *port)
{
	mutex_lock(&port->func_lock);
	if (unlikely(!port->func)) {
		mutex_unlock(&port->func_lock);
		sp6502com_pr_err("NULL func\n");
		return -ENODEV;
	}
	if (likely(port->in_sdio_uart_irq != current)) {
		sp6502com_pr_info("SDIO claim host\n");
		sdio_claim_host(port->func);
	}
	mutex_unlock(&port->func_lock);
	return 0;
}

static inline void sdio_tty_release_func(struct sdio_tty_dev *port)
{
	if (likely(port->in_sdio_uart_irq != current)) {
		sp6502com_pr_info("SDIO release host\n");
		sdio_release_host(port->func);
	}
}

static void sprd_wake_lock(struct sprd_wakelock *port)
{
	FUNC_ENTER();

	mutex_lock(&sdio_wakelock_lock);
	if (!port->wakelock_held) {
		wake_lock(&port->sprd_lock);
		port->wakelock_held = true;
	}
	mutex_unlock(&sdio_wakelock_lock);

	FUNC_EXIT();
}

static void sprd_wake_unlock(struct sprd_wakelock *port)
{
	FUNC_ENTER();

	mutex_lock(&sdio_wakelock_lock);
	if (port->wakelock_held) {
		wake_unlock(&port->sprd_lock);
		port->wakelock_held = false;
	}
	mutex_unlock(&sdio_wakelock_lock);

	FUNC_EXIT();
}

static void sdio_sp6502com_show_header(unsigned char * buf, int len)
{
	int i = 0;

	FUNC_ENTER();

	sp6502com_pr_info("show header: %d\n", len);

	for (i = 0; (i < len)&&(i<64); i++) {
		sp6502com_pr_warn("show header: buf[%d] = 0x%x\n", i, buf[i]);
	}

	FUNC_EXIT();
}

static int modem_detect_card(unsigned int mtime,bool force)
{
	struct mmc_host *mmc = NULL;
	struct class_dev_iter iter;
	struct device *dev;
	int ret = 0;

	mmc = mmc_alloc_host(0, NULL);
	if (!mmc) {
		sp6502com_pr_err("cannot alloc mmc\n");
		ret =  -ENOMEM;
		goto out;
	}

	BUG_ON(!mmc->class_dev.class);
	class_dev_iter_init(&iter, mmc->class_dev.class, NULL, NULL);
	for (;;) {
		dev = class_dev_iter_next(&iter);
		if (!dev) {
			sp6502com_pr_err("%s is not found.\n",
				modem_mmc_id);
			ret = -1;
			break;
		} else {
			struct mmc_host *host = container_of(dev,
				struct mmc_host, class_dev);
			if (dev_name(&host->class_dev) &&
				strcmp(dev_name(mmc_dev(host)),
					modem_mmc_id))
				continue;
			if (true == force && NULL != host->card) {
				sp6502com_pr_warn("Force Remove Card\n");
				sprd_set_removed(host->card);
			}
			mmc_detect_change(host, msecs_to_jiffies(mtime));

			ret = 0;
			break;
		}
	}
	mmc_free_host(mmc);
out:
	return ret;
}

#ifdef	SDIO_RX_RESEND_ENABLED
static void sprd_soft_reset(void)
{
	int ret = 0;

	mutex_lock(&sdio_reset_lock); 
	probe_finish = SPRD_DETECT_ONGOING;
	modem_sdio_reset(1);

	ret = wait_event_interruptible_timeout(init_wq,
					   (probe_finish == SPRD_DETECT_FINISHED), msecs_to_jiffies(5000));
	if (ret == 0) {
		modem_detect_card(0,true);
		ret = wait_event_interruptible_timeout(init_wq,
				   (probe_finish == SPRD_DETECT_FINISHED), msecs_to_jiffies(3000));
		if (ret == 0) {
			sp6502com_pr_warn("Sprd Reset Probe Timeout\n");
			probe_finish = SPRD_DETECT_TIMEOUT;
		}
	} else if (ret == -ERESTARTSYS) {
		sp6502com_pr_warn("Sp6502com Sdio Probe Interrupt\n");
		probe_finish = SPRD_DETECT_NONE;
	} else {
		sp6502com_pr_warn("Sprd Sdio_Slave Reset Done\n");
	}
	modem_sdio_reset(0);

	mutex_unlock(&sdio_reset_lock);
}
#endif
#ifdef CONFIG_SPRD_FLASHLESS
void flashless_mode_set(int mode)
{
	FUNC_ENTER();
	mutex_lock(&sdio_write_lock);

	if (mode == 0||mode == 1) {
		if (mode == 0)
			sp6502com_sdio_write_buf_clear();
		flashless_mode = mode;
		rx_packet_count = 1;
	} else {
		sp6502com_pr_err("Abnormal flashless mode set.\n");
	}
	mutex_unlock(&sdio_write_lock);
	FUNC_EXIT();
}
EXPORT_SYMBOL(flashless_mode_set);

int flashless_mode_get(void)
{
	return flashless_mode;
}
EXPORT_SYMBOL(flashless_mode_get);
#endif

int modem_detect_status(void)
{
	return probe_finish;
}
EXPORT_SYMBOL(modem_detect_status);

void modem_detect(bool on,unsigned int mtime)
{
	int ret;
	FUNC_ENTER();

	if (on) {
		modem_is_on = 1;
	} else {
		modem_is_on = 0;
	}

	if (probe_finish != SPRD_DETECT_FINISHED)
		probe_finish = SPRD_DETECT_ONGOING;

	ret = modem_detect_card(mtime,false);
	if (ret) {
		sp6502com_pr_err("modem sdio detect failed.\n");
		return;
	}
	if (on)
		ret = wait_event_interruptible_timeout(init_wq,
				   (probe_finish == SPRD_DETECT_FINISHED), msecs_to_jiffies(8000));
	else
		ret = wait_event_interruptible_timeout(init_wq,
				   (probe_finish == SPRD_DETECT_REMOVED), msecs_to_jiffies(3000));
	if (ret == 0) {
		if (on) {
			if (probe_finish == SPRD_DETECT_ONGOING) {
				sp6502com_pr_warn("sprd probe timeout\n");
				probe_finish = SPRD_DETECT_TIMEOUT;
			}
		} else {
			sp6502com_pr_warn("sprd remove called\n");
			probe_finish = SPRD_DETECT_REMOVED;
		}
	} else if (ret == -ERESTARTSYS) {
		sp6502com_pr_warn("sp6502com sdio probe interrupt\n");
		probe_finish = SPRD_DETECT_NONE;
	}

	FUNC_EXIT();
	return;
}
EXPORT_SYMBOL(modem_detect);

static inline unsigned int sdio_in(struct sdio_tty_dev *port, int offset)
{
	unsigned char c;
	c = sdio_readb(port->func, offset, NULL);
	return c;
}

static inline void sdio_out(struct sdio_tty_dev *port, int offset, int value)
{
	sdio_writeb(port->func, value, offset, NULL);
}

static unsigned int sdio_read(struct sdio_func *func, void *dst,
	unsigned int addr, int count)
{
	int ret = 0;
	int actual_cnt = 0;
	int remain_cnt = 0;

	FUNC_ENTER();

	if (count < SDIO_BLOCK_SIZE) {
		ret = sdio_set_block_size(sdio_dev->func, count);
		if (ret) {
			sp6502com_pr_err("set block size %d failed with ret = %d\n",count,ret);
			return ret;
		}

		msleep(1);

		ret = sdio_readsb(func, dst, addr, count);
		if (ret != 0)
			sp6502com_pr_err("read= %d,count = %d\n",ret,count);

		ret = sdio_set_block_size(sdio_dev->func, SDIO_BLOCK_SIZE);	
		if (ret) {
			sp6502com_pr_err("set block size failed with ret = %d\n",ret);
			return ret;
		}
		FUNC_EXIT();
		return ret;
	}

	if ((count >= SDIO_BLOCK_SIZE) && (count % SDIO_BLOCK_SIZE) != 0) {	
		ret = sdio_set_block_size(sdio_dev->func, SDIO_BLOCK_SIZE);
		if (ret) {
			sp6502com_pr_err("set block size %d failed with ret = %d\n",count,ret);
			return ret;
		}

		remain_cnt = count % SDIO_BLOCK_SIZE;
		actual_cnt = count - remain_cnt;

		ret = sdio_readsb(func, dst, addr, actual_cnt);		
		if (ret != 0)
			sp6502com_pr_err("read= %d,count = %d\n",ret,actual_cnt);

		ret = sdio_set_block_size(sdio_dev->func, remain_cnt);
		if (ret) {
			sp6502com_pr_err("set block size %d failed with ret = %d\n",remain_cnt,ret);
			return ret;
		}

		msleep(1);

		ret = sdio_readsb(func, dst+actual_cnt, addr, remain_cnt);	
		if (ret != 0)
			sp6502com_pr_err("read= %d,count = %d\n",ret,remain_cnt);

		ret = sdio_set_block_size(sdio_dev->func, SDIO_BLOCK_SIZE);	
		if (ret) {
			sp6502com_pr_err("set block size failed with ret = %d\n",ret);
			return ret;
		}

		FUNC_EXIT();
		return ret;
	}
#if 0
	ret = sdio_set_block_size(sdio_dev->func, SDIO_BLOCK_SIZE);
	if (ret) {
		sp6502com_pr_err("set block size failed with ret = %d\n",ret);
		return ret;
	}
#endif
	ret = sdio_readsb(func, dst, addr, count);
	if (ret == -110) {
		sp6502com_pr_err("read data timeout\n");
		msleep(20);
		ret = sdio_readsb(func, dst, addr, count);
	}
	if (ret != 0)
		sp6502com_pr_err("read= %d,count = %d\n",ret,count);

	FUNC_EXIT();

	return ret;
}

static unsigned int sdio_write(struct sdio_func *func, unsigned int addr,
	void *src, int count)
{
	int ret = 0;

	FUNC_ENTER();

	ret = sdio_memcpy_toio(func, addr, src, count);
	if (ret != 0)
		printk("ret write= %d,count = %d\n",ret,count);
	FUNC_EXIT();

	return ret;
}

void sdio_sp6502com_tty_retry_read(void)
{
	wake_up_interruptible(&(sdio_dev->sdio_tty_unthrottle_wq));
}

inline int sp6502com_sdio_tty_read_avail(struct sdio_tty_dev *port)
{
	int available = 0;
	available = sdio_dev->rx_data_len <= 512*3?sdio_dev->rx_data_len:512*3;
	return available;
}

inline int sp6502com_sdio_tty_write_avail(void)
{
	return SDIO_TTY_WRITE_SIZE - sdio_dev->tx_data_len;
}

ssize_t sp6502com_sdio_tty_read(struct file *file, unsigned char *buf, size_t count)
{
	int res = 0;
	int cpy_len  = 0;

	FUNC_ENTER();
	sp6502com_pr_info("sdio_dev->rx_data_len = %d\n", sdio_dev->rx_data_len);
	sp6502com_pr_info("count = %d\n", count);
	sp6502com_pr_info("has_received_len =%d\n", has_received_len);
	if (has_received_len >= SDIO_BUFFER_READ_SIZE) {
		sp6502com_pr_err("please check read buf, has reveived len is wrong\n");
		has_received_len = 0;
		sdio_dev->rx_data_len = 0;
		return 0;
	}

	cpy_len = count > sdio_dev->rx_data_len? sdio_dev->rx_data_len : count;
	if (cpy_len < count)
		sp6502com_pr_warn("Need to check cpy_len\n");

	if (has_received_len + cpy_len > SDIO_BUFFER_READ_SIZE) {
		sp6502com_pr_err("overflow, please check it\n");
		has_received_len = 0;
		sdio_dev->rx_data_len = 0;
		return 0;
	}
	memcpy(buf, sdio_dev->rx_data_buf + has_received_len, cpy_len);

	res = cpy_len;
	sdio_dev->rx_data_len -= cpy_len;

	if (sdio_dev->rx_data_len > 0) {
		sp6502com_pr_info("still need to read data from sdio driver\n");
		has_received_len += cpy_len;
	} else {
		has_received_len = 0;
	}

	FUNC_EXIT();

	return res;
}

ssize_t sp6502com_sdio_tty_write(struct file *file, unsigned char *buf, size_t count)
{
	int rc = 0;

	FUNC_ENTER();
	mutex_lock(&sdio_write_lock);
#ifdef CONFIG_SPRD_FLASHLESS
	if (sdio_dev->tx_data_len + count > (flashless_mode == 1?SPRD_FL_TTY_WRITE_SIZE:SDIO_TTY_WRITE_SIZE)) {
#else
	if (sdio_dev->tx_data_len + count > SDIO_TTY_WRITE_SIZE) {
#endif
		sp6502com_pr_info("enter block mode\n");
		rc = wait_event_interruptible_timeout(sdio_dev->sdio_tty_write_wq,
				(sp6502com_sdio_write_buf_length() == 0),msecs_to_jiffies(10000));
		
		if ((rc == 0) || (rc ==-ERESTARTSYS)) {
			sp6502com_pr_err("sdio tty write quit rc = %d\n", rc);
			sp6502com_pr_err("sdio write check mdm_rdy = %d,ap_rts =%d\n", bp_rdy(),bp_rts());
			sp6502com_sdio_write_buf_clear();
			mutex_unlock(&sdio_write_lock);
			return -1;
		}
	}

	mutex_lock(&sdio_write_buf_lock);
#ifdef CONFIG_SPRD_FLASHLESS
	if (sdio_dev->tx_data_len + count > (flashless_mode == 1?SPRD_FL_TTY_WRITE_SIZE:SDIO_TTY_WRITE_SIZE)) {
#else
	if (sdio_dev->tx_data_len + count > SDIO_TTY_WRITE_SIZE) {
#endif
		sp6502com_pr_err("tty write data too large\n");
		mutex_unlock(&sdio_write_buf_lock);
		mutex_unlock(&sdio_write_lock);
		return -1;
	}

	if (sdio_dev->tx_data_len != 0) {
		sp6502com_pr_info(" tty write data begin to pack data\n");
	}

	memcpy(sdio_dev->tx_data_buf + sdio_dev->tx_data_len, buf, count);
	sdio_dev->tx_data_len += count;

	rc = count;

	mutex_unlock(&sdio_write_buf_lock);

	wake_up_interruptible(&(sdio_dev->sdio_write_wq));

	mutex_unlock(&sdio_write_lock);

	FUNC_EXIT();

	return rc;
}

static void sdio_tty_read()
{
	unsigned char *ptr;
	int avail = 0;
	struct sdio_tty_dev *port;
	struct tty_struct *tty;

	FUNC_ENTER();

	port = sdio_dev;

	if (!port) {
		sp6502com_pr_err("NULL tty info\n");
		sp6502com_sdio_read_buf_clear(port);
		return;
	}

	tty = port->tty;
	if (!tty) {
		sp6502com_pr_err("NULL tty\n");
		sp6502com_sdio_read_buf_clear(port);
		return;
	}

	mutex_lock(&sdio_push_lock);

	if (port->opened == 0) {
		sp6502com_pr_warn("tty already closed\n");
		sp6502com_sdio_read_buf_clear(port);
		mutex_unlock(&sdio_push_lock);
		return;
	}

	push_sdio_tty_fail = 0;
	for (;;) {
		
		avail = sp6502com_sdio_tty_read_avail(port);
		if (avail == 0) {
			sp6502com_pr_info("tty read finished, no available\n");
			break;
		}

		if (port->opened == 0) {
			sp6502com_pr_warn("tty already closed\n");
			sp6502com_sdio_read_buf_clear(port);
			break;
		}

		
		avail = tty_prepare_flip_string(tty, &ptr, avail);
		sp6502com_pr_info("apply available space = %d\n", avail);

		if (avail <= 0) {
			push_sdio_tty_fail = 1;
			if (!timer_pending(&port->buf_req_timer)) {
				init_timer(&port->buf_req_timer);
				port->buf_req_timer.expires = jiffies +
							((300 * HZ)/1000);
				port->buf_req_timer.function = buf_req_retry;
				port->buf_req_timer.data = (unsigned long)port;
				add_timer(&port->buf_req_timer);
			}
			mutex_unlock(&sdio_push_lock);
			return;
		}

		avail = sp6502com_sdio_tty_read(NULL, ptr, avail);
		sp6502com_pr_info("real read avail = %d\n", avail);

		if (avail <= 0) {
			sp6502com_pr_warn("can not read\n");
		}

		tty_flip_buffer_push(tty);

	}
	mutex_unlock(&sdio_push_lock);

	FUNC_EXIT();
}

static size_t sdio_sp6502com_process_data(void)
{
	int ret = 0;
	int raw_data_len = 0;
	unsigned int checksum = 0;
	unsigned int cal_checksum = 0;

	unsigned char *data_buf=NULL;
	struct sdio_sp6502com_modem_packet_t *packet;

	FUNC_ENTER();

	packet = (struct sdio_sp6502com_modem_packet_t *)(sdio_dev->rx_buf.buf_8);

	memset(sdio_dev->rbuf, 0, 64);

#ifdef SDIO_SP6502_PERF_PROFILING
	tran_start = ktime_get();
#endif
#ifdef CONFIG_SPRD_FLASHLESS
	ret = sdio_read(sdio_dev->func, sdio_dev->rx_buf.buf_8, SP6502COM_ADDR,
						flashless_mode == 1?SDIO_BLOCK_SIZE:SDIO_BUFFER_READ_ONCE);
#else
	ret = sdio_read(sdio_dev->func, sdio_dev->rx_buf.buf_8, SP6502COM_ADDR, SDIO_BUFFER_READ_ONCE);
#endif

#ifdef SDIO_SP6502_PERF_PROFILING
	rx_tran = ktime_add(rx_tran,ktime_sub(ktime_get(), tran_start));
#endif
	if (ret < 0) {
		sp6502com_pr_err("header&data read failed\n");
		return ret;
	}

	if (!flashless_mode) {
		if (!packet->header.frame_num && !packet->header.head_tag && !packet->header.packet_type && !packet->header.len) {
			sp6502com_pr_err("Empty Package = 0x%08x,0x%08x,0x%08d,0x%08x\n",
						packet->header.head_tag,packet->header.packet_type,packet->header.len,packet->header.reserved);
			ret = 0;
			return ret;
		}

		if (rx_packet_count != packet->header.frame_num) {
			sp6502com_pr_warn("need to confirm rx_packet num = %d,ori = %d\n", packet->header.frame_num,rx_packet_count);
		}
		rx_packet_count = packet->header.frame_num;
		rx_packet_count++;
	}

	sp6502com_pr_info("frame number = %d\n",packet->header.frame_num);

	if (packet->header.head_tag != SDIO_SP6502COM_MODEM_PACKET_HEADER_TAG) {
		sp6502com_pr_err("header = 0x%08x,0x%08x,0x%08d,0x%08x\n",
					packet->header.head_tag,packet->header.packet_type,packet->header.len,packet->header.reserved);
		
		ret = -EIO;
		FUNC_EXIT();
		return ret;
	}

	if (packet->header.packet_type != SDIO_SP6502COM_MODEM_PACKET_TYPE) {
		sp6502com_pr_err("error header type = %08x, real type = %08x \n", packet->header.packet_type, SDIO_SP6502COM_MODEM_PACKET_TYPE);
		sdio_sp6502com_show_header((unsigned char*) packet, sizeof(struct sdio_sp6502com_modem_packet_t));
		ret = -EIO;
		FUNC_EXIT();
		return ret;
	}

	if (packet->header.len > SDIO_BUFFER_READ_SIZE) {
		sp6502com_pr_err("Receive data too larg %08d\n", packet->header.len);
		ret = -EIO;
		FUNC_EXIT();
		return ret;
	}

	if (packet->header.len > SDIO_BUFFER_READ_ONCE) {
		sp6502com_pr_err("Receive data length %d larger than %d\n", packet->header.len,SDIO_BUFFER_READ_ONCE);
		ret = -EIO;
		FUNC_EXIT();
		return ret;
	}

	sdio_dev->rx_data_len = packet->header.len;
	checksum = packet->header.reserved;

	sp6502com_pr_info("data_len = %d\n", sdio_dev->rx_data_len);

	raw_data_len = sdio_dev->rx_data_len;

	data_buf = sdio_dev->rx_data_buf;
	memcpy(data_buf, sdio_dev->rx_buf.buf_8+SDIO_SP6502COM_PACKET_HEADER_LEN, raw_data_len);

	cal_checksum = 0;

	
#ifdef SDIO_TTY_DEBUG
	data_rx_count++;
	data_rx_size += sdio_dev->rx_data_len;
#endif
	FUNC_EXIT();

	return ret;
}

static void sdio_sp6502com_rx(void)
{
	int ret = 0;
	FUNC_ENTER();

	if (sdio_removed == 1){
		sp6502com_pr_warn("Sdio_sp6502com_already removed");
		return;
	}

	ret = sdio_tty_claim_func(sdio_dev);
	if (ret)
		return;

	if (sdio_dev->opened == 1) {
		sprd_set_status(SDIO_IO_RECEIVING);
#ifdef SDIO_RX_RESEND_ENABLED
		if (!flashless_mode) {
			ap_rdy_set(1);
		}
#endif

		if (sdio_init_config == 0) {
			sp6502com_pr_info("Setup sdio");
			ret = sdio_enable_func(sdio_dev->func);
			if (ret) {
				sp6502com_pr_err("Enable func failed with ret = %d\n", ret);
				goto fail_enable;
			}
			ret = sdio_set_block_size(sdio_dev->func, SDIO_BLOCK_SIZE);
			if (ret) {
				sp6502com_pr_err("Set block size failed with ret = %d\n", ret);
				goto disable_func;
			}

			ret = sdio_set_block_size(sdio_dev->func, SDIO_BLOCK_SIZE);	
			if (ret) {
				sp6502com_pr_err("set block size failed with ret = %d\n", ret);
				goto disable_func;
			}

			sdio_init_config = 1;
		}

		sprd_set_status(SDIO_RECEIVING_DATA);
		ret = sdio_sp6502com_process_data();
#ifdef SDIO_SP6502_PERF_PROFILING
		rx_total = ktime_add(rx_total,ktime_sub(ktime_get(), total_start_rx));
#endif
		sp6502com_pr_info("Sdio_sp6502com_process_data");
		if (ret == 0) {
			if (sdio_dev->rx_data_len > 0) {
				sp6502com_pr_info("Sdio_sp6502com_rx sdio_dev->rx_data_len  = %d\n", sdio_dev->rx_data_len);
				sdio_tty_read();
			} else {
				sp6502com_pr_err("Empty package,Ignore\n");
			}
#ifdef SDIO_RX_RESEND_ENABLED
			if (!flashless_mode) {
				ap_resend_set(0);
				ap_rdy_set(0);
			}
#endif
		} else {
			if (ret < 0)
				sp6502com_sdio_read_buf_clear(sdio_dev);
			else if (sdio_dev->rx_data_len <= 0)
				sp6502com_pr_warn("No data read from modem\n");
#ifdef SDIO_RX_RESEND_ENABLED
			if (!flashless_mode) {
				ap_resend_set(0);
				sdio_tty_release_func(sdio_dev);
				sp6502com_pr_warn("Need modem resend & Reinit Sprd Sd_slave %d,Rx frame = %u,Rx irq = %u\n",
					ret,rx_packet_count,rts_irqcount);
				sprd_soft_reset();
				ap_resend_set(1);
				sdio_tty_claim_func(sdio_dev);
				reset_count++;
			}
#endif
		}
	}

	
	sdio_tty_release_func(sdio_dev);

	sprd_set_status(SDIO_VALID);

	if (sdio_dev->opened == 0) {
		 sp6502com_pr_info("system closed, sdio release\n");
		 sp6502com_sdio_read_buf_clear(sdio_dev);
		 wake_up(&(sdio_dev->sdio_tty_close_wq));
	}

	FUNC_EXIT();
	return;

disable_func:
	
	

fail_enable:
#ifdef SDIO_RX_RESEND_ENABLED
	if (!flashless_mode) {
		ap_rdy_set(0);
		ap_resend_set(0);
	}
#endif
	sdio_tty_release_func(sdio_dev);

	FUNC_EXIT();
	return;
}

static int sdio_sp6502com_rx_thread(void *data)
{
	FUNC_ENTER();

	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		wait_event_interruptible(sdio_dev->sdio_read_wq,
					   ((1 == sdio_dev->modem_rts)
						||(sdio_dev->rx_data_len > 0)
							|| (sdio_dev->remove_pending == 1)));


#ifdef SDIO_SP6502_PERF_PROFILING
		if (sdio_dev->modem_rts && sdio_dev->opened && !sdio_dev->rx_data_len)
			total_start_rx = ktime_get();
#endif
		if (sdio_dev->remove_pending == 1) {
			sp6502com_pr_warn("sdio_sp6502com_rx_thread closed\n");
			sdio_dev->remove_pending = 0;
			break;
		}

		if (sdio_dev->opened == 1) {
			mutex_lock(&sdio_rx_lock);
			if (sdio_dev->status == SDIO_OPENING) {
				sdio_dev->status = SDIO_VALID;
				msleep(50);
			}

			if (sdio_dev->rx_data_len > 0) {
				if (wait_event_interruptible_timeout(sdio_dev->sdio_tty_unthrottle_wq,
					(sdio_platform_tty_throttle_status() == 0), msecs_to_jiffies(200)) == 0) {
#ifdef SDIO_TTY_DEBUG
					sp6502com_pr_warn("Not receive unthrottle event\n");
#endif
				}
				if (sdio_dev->opened == 1)
					sdio_tty_read();
				else {
					sp6502com_pr_warn("Tty closed when pushing\n");
					sp6502com_sdio_read_buf_clear(sdio_dev);
				}
			}
			sprd_wake_lock(&(sdio_dev->rx_wakelock));

			wait_event_interruptible(sdio_dev->sdio_sleep_wq,
								(sdio_dev->suspended == false));

			if ((1 == sdio_dev->modem_rts) && (1 == sdio_dev->opened)) {
				if ((sdio_dev->rx_data_len == 0)) {
					sdio_dev->modem_rts = 0;
					sdio_sp6502com_rx();
#ifdef SDIO_SP6502_PERF_PROFILING
				rx_tty_time = ktime_add(rx_tty_time,ktime_sub(ktime_get(), total_start_rx));
#endif
				}
			}
			sprd_wake_unlock(&(sdio_dev->rx_wakelock));
			mutex_unlock(&sdio_rx_lock);
		} else {
			sp6502com_pr_warn("tty already closed\n");
			sdio_dev->modem_rts = 0;
			sdio_dev->rx_data_len = 0;
		}
	}
	set_current_state(TASK_RUNNING);
	FUNC_EXIT();

	return 0;
}

static size_t sdio_sp6502com_write_modem_data(void)
{
	int ret = 0;
	int send_len, len;
	int copy_len = 0;
	const unsigned char *send_buf;
	struct sdio_sp6502com_modem_packet_t *packet;

	FUNC_ENTER();

	mutex_lock(&sdio_write_buf_lock);
#if 0
#ifdef CONFIG_SPRD_FLASHLESS
	memset(sdio_dev->tx_buf.buf_8,0,
			flashless_mode == 1?SPRD_FL_BUFFER_WRITE_SIZE:SDIO_BUFFER_WRITE_SIZE);
#else
	memset(sdio_dev->tx_buf.buf_8,0,SDIO_BUFFER_WRITE_SIZE);
#endif
#endif
	len = sdio_dev->tx_data_len;
	send_buf = sdio_dev->tx_data_buf;
	send_len = len;

	packet = (struct sdio_sp6502com_modem_packet_t *)sdio_dev->tx_buf.buf_8;
	packet->header.head_tag = SDIO_SP6502COM_MODEM_PACKET_HEADER_TAG; 
	packet->header.packet_type = SDIO_SP6502COM_MODEM_PACKET_TYPE;    
	packet->header.len = len;
	packet->header.frame_num = tx_packet_count++;     
	packet->header.reserved = 0;

	copy_len = send_len;
	send_len += SDIO_SP6502COM_PACKET_HEADER_LEN;
	memcpy((u8 *) sdio_dev->tx_buf.buf_8 + SDIO_SP6502COM_PACKET_HEADER_LEN, (u8 *) send_buf, copy_len);
	sp6502com_sdio_write_buf_clear();

	wake_up_interruptible(&(sdio_dev->sdio_tty_write_wq));

	mutex_unlock(&sdio_write_buf_lock);

	ret = sdio_tty_claim_func(sdio_dev);
	if (ret)
		return ret;

	if (sdio_init_config == 0) {
		sp6502com_pr_info("Setup sdio");
		ret = sdio_enable_func(sdio_dev->func);
		if (ret) {
			sp6502com_pr_err("Enable func failed with ret = %d\n", ret);
			goto fail_enable;
		}

		ret = sdio_set_block_size(sdio_dev->func, SDIO_BLOCK_SIZE);
		if (ret) {
			sp6502com_pr_err("Set block size failed with ret = %d\n", ret);
			goto disable_func;
		}

		ret = sdio_set_block_size(sdio_dev->func, SDIO_BLOCK_SIZE);	
		if (ret) {
			sp6502com_pr_err("set block size failed with ret = %d\n", ret);
			goto disable_func;
		}

		sdio_init_config = 1;
	}

	send_len = sdio_align_size(sdio_dev->func, send_len);
	sdio_dev->last_tx_len = send_len;

	sp6502com_pr_info("Actual tx len = %d\n", send_len);

#ifdef	SDIO_SP6502_PERF_PROFILING
	tran_start = ktime_get();
#endif
	ret = sdio_write(sdio_dev->func, SP6502COM_ADDR, sdio_dev->tx_buf.buf_8, send_len);
#ifdef	SDIO_SP6502_PERF_PROFILING
	tx_tran = ktime_add(tx_tran,ktime_sub(ktime_get(), tran_start));
#endif
	sdio_tty_release_func(sdio_dev);

#ifdef SDIO_TTY_DEBUG
	if (!ret) {
		data_tx_count++;
		data_tx_size += copy_len;
	}
#endif

	FUNC_EXIT();
	return ret;

disable_func:
	
	

fail_enable:
	sdio_tty_release_func(sdio_dev);

	FUNC_EXIT();
	return ret;
}

#ifdef SDIO_TX_RESEND_ENABLED
size_t sdio_sp6502com_resend_modem_data(void)
{
	int ret = 0;
	static int count = 0;
	FUNC_ENTER();

	sp6502com_pr_warn("resend count = %d\n", ++count);

	ret = sdio_tty_claim_func(sdio_dev);
	if (ret)
		return ret;

	ret = sdio_write(sdio_dev->func, SP6502COM_ADDR, sdio_dev->tx_buf.buf_8, sdio_dev->last_tx_len);

	sdio_tty_release_func(sdio_dev);

	FUNC_EXIT();
	return ret;
}
#endif

static int sdio_sp6502com_tx(const unsigned char * buf, unsigned short len)
{
	int ret = 0;
	FUNC_ENTER();

	
	if (bp_rdy()) {
		if (wait_event_interruptible_timeout(sdio_dev->sdio_write_wq,
			   ((!bp_rdy()) || (sdio_dev->opened == 0)), msecs_to_jiffies(300)) == 0) {
			sp6502com_pr_err(" wait bp rdy invalid timeout\n");
			ret = -1;
			goto tx_done_no_lock;
		}
	}
	if (sdio_dev->opened == 0) {
		sp6502com_pr_warn("already closed\n");
		ret = -1;
		goto tx_done_no_lock;
	}

	ap_rts_set(1);	
	usleep(400);

	if (bp_rts()) {
		if (wait_event_interruptible_timeout(sdio_dev->conf_wq,!bp_rts(),msecs_to_jiffies(3000)) == 0) {
			sp6502com_pr_warn("Mdm rts not invalid status = %d,Rx frame = %u,Rx irq = %u\n",
				sdio_dev->status,rx_packet_count,rts_irqcount);
			ret = -1;
			goto tx_done_no_lock;
		}
	}

	if (!bp_rdy()) {	
#ifdef SDIO_SP6502_CONFLICT_METHOD
		if (wait_event_interruptible_timeout(sdio_dev->sdio_write_wq,
					  ((bp_rdy()) || (sdio_dev->opened == 0)),
					   msecs_to_jiffies(500)) == 0) {
			sp6502com_pr_err("can not receive bp response, ap gives up!\n");
			sp6502com_pr_err("bp_rdy = %d,bp_rts = %d,status = %d,Rx frame = %u,Rx irq = %u\n",
				bp_rdy(),bp_rts(),sdio_dev->status,rx_packet_count,rts_irqcount);
			ret = -1;
			goto tx_done_no_lock;
		}

		if (sdio_dev->opened == 0) {
			sp6502com_pr_warn("sprd tty already closed\n");
			ret = -1;
			goto tx_done_no_lock;
		}
#else
		if (bp_rts()) {
			
			ret = -1;
			goto tx_done_no_lock;
		}

		sdio_dev->modem_rdy = bp_rdy();
		if (!bp_rdy()) {
			if (wait_event_interruptible_timeout(sdio_dev->sdio_write_wq,
				(bp_rdy() || bp_rts()), msecs_to_jiffies(30)) == 0) {

				ret = -1;
				goto tx_done_no_lock;
			}
		}
#endif
	}

	if (!bp_rdy()) {
		sp6502com_pr_err("can not receive bp response2, ap gives up!\n");
		ret = -1;
		goto tx_done_no_lock;
	}

	if (sdio_dev->opened == 1) {
		sprd_set_status(SDIO_IO_SENDING);
#ifdef SDIO_TX_RESEND_ENABLED
		if (sdio_sp6502com_resend_count >= SDIO_SP6502_RESEND_MAX_COUNT) {
			sp6502com_pr_warn("resend %d times, but still failed!\n", SDIO_SP6502_RESEND_MAX_COUNT);
		}
		if ((bp_resend() && (sdio_sp6502com_resend_count < SDIO_SP6502_RESEND_MAX_COUNT)) && (begin_retry != 1)) {
			ret = sdio_sp6502com_resend_modem_data();
			sdio_sp6502com_resend_count++;
		} else {
			sdio_sp6502com_resend_count = 0;
			ret = sdio_sp6502com_write_modem_data();
		}

		sdio_dev->modem_resend = 0;
#else
		ret = sdio_sp6502com_write_modem_data();
#endif
	}

	if (ret != 0) {
		sp6502com_pr_err("sdio write fail\n");
		ret = -2;
	}

	ap_rts_set(0);	

	sprd_set_status(SDIO_VALID);
	if (sdio_dev->opened == 0) {
		sp6502com_pr_warn("system closed, sdio release\n");
		wake_up(&(sdio_dev->sdio_tty_close_wq));
	}

	FUNC_EXIT();

	return ret;

tx_done_no_lock:
	ap_rts_set(0);

	ret = -1;

	FUNC_EXIT();

	return ret;
}

static ssize_t sdio_sp6502com_xmit_buf(unsigned char *buf, ssize_t len)
{
	int ret = 0;
	int retry_count = 0;
	FUNC_ENTER();

	if (sdio_removed == 1){
		sp6502com_pr_warn("Sdio_sp6502com_already removed");
		sp6502com_sdio_write_buf_clear();
		return ret;
	}

	sp6502com_pr_info("tx_data_len = %d\n", len);

	begin_retry = 0;
	do {
		if (begin_retry != 0) {
			msleep(2);
		}
#ifdef	SDIO_RX_RESEND_ENABLED
		mutex_lock(&sdio_reset_lock);
#endif
		ret = sdio_sp6502com_tx(buf, len);
#ifdef	SDIO_RX_RESEND_ENABLED
		mutex_unlock(&sdio_reset_lock);
#endif
		if (sdio_dev->opened == 0) {
			sp6502com_pr_warn("system closed, sdio release\n");
			return ret;
		}
		if (ret == -2) {
			begin_retry = 2; 
		} else {
			begin_retry = 1;
		}
	}
	while ((ret != 0) && (retry_count++ < 10));

	begin_retry = 0;

	if (ret != 0) {
		sp6502com_pr_err("modem no response, ret = %d\n", ret);
		sp6502com_sdio_write_buf_clear();
		wake_up_interruptible(&(sdio_dev->sdio_tty_write_wq));
	}

	FUNC_EXIT();

	return ret;
}

static int sdio_sp6502com_tx_thread(void *data)
{
	FUNC_ENTER();

	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
#ifdef SDIO_TX_RESEND_ENABLED
		wait_event_interruptible(sdio_dev->sdio_write_wq,
				   ((((!bp_rdy()) && (sp6502com_sdio_write_buf_length() != 0))
					||((sdio_dev->modem_resend== 1) && (!bp_rdy()))) && (sdio_dev->opened == 1))
						|| (sdio_dev->remove_pending == 1));

#else
		wait_event_interruptible(sdio_dev->sdio_write_wq,
			(((!bp_rdy()) && sp6502com_sdio_write_buf_length() != 0)
					&& (sdio_dev->opened ==1))
						|| (sdio_dev->remove_pending == 1));
#endif

#ifdef	SDIO_SP6502_PERF_PROFILING
		total_start_tx = ktime_get();
#endif
		if (sdio_dev->remove_pending == 1) {
			sp6502com_pr_warn("sdio_sp6502com_rx_thread closed\n");
			sdio_dev->remove_pending = 0;
			break;
		}

		mutex_lock(&sdio_tx_lock);

		sprd_wake_lock(&(sdio_dev->tx_wakelock));

		wait_event_interruptible(sdio_dev->sdio_sleep_wq,
							(sdio_dev->suspended == false));

		sdio_sp6502com_xmit_buf(sdio_dev->tx_data_buf, sdio_dev->tx_data_len);
		sprd_wake_unlock(&(sdio_dev->tx_wakelock));
		mutex_unlock(&sdio_tx_lock);
#ifdef	SDIO_SP6502_PERF_PROFILING
		tx_total = ktime_add(tx_total,ktime_sub(ktime_get(), total_start_tx));
#endif
	}
	set_current_state(TASK_RUNNING);

	sp6502com_pr_err(" the thread is stop, need check\n");
	FUNC_EXIT();
	return 0;
}

static void sdio_tty_dev_destroy(struct kref *kref)
{
	struct sdio_tty_dev *port =
		container_of(kref, struct sdio_tty_dev, kref);
	kfree(port);
	port = NULL;
}

static void sdio_tty_dev_put(struct sdio_tty_dev *port)
{
	kref_put(&port->kref, sdio_tty_dev_destroy);
}

static void sdio_tty_dev_remove(struct sdio_tty_dev *port)
{
	struct sdio_func *func;

	mutex_lock(&port->func_lock);
	func = port->func;

	port->func = NULL;
	
	

	mutex_unlock(&port->func_lock);

	sdio_tty_dev_put(port);
}

static irqreturn_t modem_rts_handler(int irq, void *dev)
{
	struct sdio_tty_dev *port = dev;

	FUNC_ENTER();

	if (bp_rts() && modem_is_on && port->opened && !sdio_removed) {
		port->modem_rts = 1;
#ifdef SDIO_TTY_DEBUG
		rts_irqcount++;
#endif
		sp6502com_pr_info("modem_rts = %d\n", port->modem_rts);
		wake_up_interruptible(&(port->sdio_read_wq));
	} else if( modem_is_on && port->opened && !sdio_removed) {
#if 1
		wake_up_interruptible(&(port->conf_wq));
#endif
	}

	FUNC_EXIT();

	return IRQ_HANDLED;
}

static irqreturn_t modem_rdy_handler(int irq, void *dev)
{
	struct sdio_tty_dev *port = dev;

	FUNC_ENTER();

	bp_rdy_value = bp_rdy();

	if (bp_rdy_value && modem_is_on && port->opened && !sdio_removed) {
		port->modem_rdy = 1;
#ifdef SDIO_TTY_DEBUG
		rdy_irqcount++;
#endif
	} else if (modem_is_on) {
#ifdef SDIO_TX_RESEND_ENABLED
		sdio_dev->modem_resend = bp_resend();
#endif
	}

	wake_up_interruptible(&(sdio_dev->sdio_write_wq));

	FUNC_EXIT();
	return IRQ_HANDLED;
}


static int sp6502com_sdio_irq_enable(struct sdio_tty_dev *port)
{
	int retval = 0;
	FUNC_ENTER();
	mutex_lock(&sdio_irq_lock);

	if (port->enable_irq || sdio_removed) {
		mutex_unlock(&sdio_irq_lock);
		if (port->enable_irq)
			sp6502com_pr_warn("Modem RDY handler already enabled\n");
		else
			sp6502com_pr_err("Modem RDY handler install failed!\n");
		FUNC_EXIT();
		return 0;
	}
	port->mdm_rdy_irq = gpio_to_irq(BP_RDY);
	sp6502com_pr_info("mdm_rdy_irq %d\n", port->mdm_rdy_irq);
	retval = request_irq(port->mdm_rdy_irq, modem_rdy_handler,
				IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
					"modem_rdy", port);
	irq_set_irq_wake(port->mdm_rdy_irq, 1);

	if (retval) {
		sp6502com_pr_err("Modem RDY handler install failed!\n");
		goto err_bp_rdy_irq;
	}

	port->mdm_rts_irq = gpio_to_irq(BP_RTS);

	sp6502com_pr_info("mdm_rts_irq %d\n", port->mdm_rts_irq);
	retval = request_irq(port->mdm_rts_irq, modem_rts_handler,
				IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
					"modem_rts", port);
	irq_set_irq_wake(port->mdm_rts_irq, 1);

	if (retval) {
		sp6502com_pr_err("Modem RTS handler install failed retval = %d!\n", retval);
		goto err_bp_rts_irq;
	}

	port->enable_irq = true;

	mutex_unlock(&sdio_irq_lock);

	return retval;

err_bp_rts_irq:
	free_irq(port->mdm_rdy_irq,NULL);

err_bp_rdy_irq:
	mutex_unlock(&sdio_irq_lock);

	return retval;
}

static void sp6502com_sdio_irq_disable(struct sdio_tty_dev *port)
{
	FUNC_ENTER();

	mutex_lock(&sdio_irq_lock);
	if (port->enable_irq) {
		port->enable_irq = false;
		irq_set_irq_wake(port->mdm_rts_irq, 0);
		irq_set_irq_wake(port->mdm_rdy_irq, 0);
		free_irq(port->mdm_rts_irq, port);
		free_irq(port->mdm_rdy_irq, port);
	}
	mutex_unlock(&sdio_irq_lock);

	FUNC_EXIT();
}

static int sdio_tty_open(struct tty_struct *tty, struct file *filp)
{
	struct sdio_tty_dev *port;

	if (!tty) {
		sp6502com_pr_err("NULL tty\n");
		return -ENODEV;
	}
	FUNC_ENTER();

	mutex_lock(&sdio_tty_lock);

	port = tty->driver_data = sdio_dev;

	if (port->open_count++ == 0)
		port->tty = tty;
	else {
		sp6502com_pr_warn("already open %d times\n", port->open_count);
		mutex_unlock(&sdio_tty_lock);
		FUNC_EXIT();
		return 0;
	}

	ap_rts_set(0);
	ap_resend(0);
	ap_rdy_set(0);

	push_sdio_tty_fail = 0;
	has_received_len = 0;
	sdio_init_config = 0;

	port->throttle_flag = 0;
	port->rx_data_len = 0;
	port->last_tx_len = 0;
	port->modem_rdy = 0;
	port->modem_rts = 0;
	port->modem_resend = 0;
	sprd_set_status(SDIO_OPENING);

	sp6502com_pr_info("sp6502com sdio_tty opencount %d\n", port->open_count);

	set_bit(TTY_IO_ERROR, &tty->flags);

	port->remove_pending = 0;

	sp6502com_sdio_irq_enable(port);

	port->opened = 1;

	clear_bit(TTY_IO_ERROR, &tty->flags);

	sp6502com_pr_warn("sp6502com sdio_tty opened\n");

	mutex_unlock(&sdio_tty_lock);

	FUNC_EXIT();

	return 0;
}

static void sdio_tty_close(struct tty_struct *tty, struct file * filp)
{
	struct sdio_tty_dev *port;
	int ret = 0;
	FUNC_ENTER();

	if (!tty) {
		sp6502com_pr_err("NULL tty\n");
		return;
	}

	port = tty->driver_data;

	if (!port) {
		sp6502com_pr_err("NULL tty driver data\n");
		return;
	}

	if (port->opened == 0) {
		sp6502com_pr_warn("please check why still need be closed!!!!\n");
		sp6502com_pr_warn("current pid = %d\n", current->pid);
		return;
	}

	if (--port->open_count == 0) {
		mutex_lock(&sdio_tty_lock);
		
		
		del_timer(&port->buf_req_timer);
	} else {
		sp6502com_pr_warn("still need close more time = %d\n", port->open_count);
		return;
	}

	sp6502com_sdio_irq_disable(port);

	mutex_lock(&sdio_push_lock);

	ap_rts_set(0);
	ap_resend(0);
	ap_rdy_set(0);

	push_sdio_tty_fail = 0;

	port->opened = 0;

	wake_up_interruptible(&(sdio_dev->sdio_write_wq));

	ret = wait_event_timeout(port->sdio_tty_close_wq,
			   (sprd_get_status() == SDIO_VALID), msecs_to_jiffies(100));
	if (ret == 0) {
		sp6502com_pr_warn("sp6502com tty wait sdio release timeout status = %d\n",sprd_get_status());
	} else if (ret == -ERESTARTSYS) {
		sp6502com_pr_warn("sp6502com tty system interrupt\n");
	}

	port->suspended = false;
	wake_up_interruptible(&(port->sdio_sleep_wq));

	has_received_len = 0;
	sdio_init_config = 0;
	port->rx_data_len = 0;
	port->tx_data_len = 0;
	port->last_tx_len = 0;

	mutex_unlock(&sdio_push_lock);
	mutex_unlock(&sdio_tty_lock);

	port->modem_rts = 0;
	port->modem_rdy = 0;
	port->modem_rts = 0;
	port->modem_resend= 0;

	sp6502com_pr_warn("Rx frame = %u,Rx irq = %u\n",rx_packet_count,rts_irqcount);
	sp6502com_pr_warn("sp6502com sdio_tty closed\n");

	FUNC_EXIT();
}

static int sdio_tty_write(struct tty_struct *tty, const unsigned char *buf,
			   int count)
{
	int rc = 0;

	FUNC_ENTER();

	if (!tty) {
		sp6502com_pr_err("NULL tty\n");
		return -ENODEV;
	}

	sp6502com_pr_info("tty write count %d\n",count);
	rc = sp6502com_sdio_tty_write((struct file *)NULL, (unsigned char*) buf, count);

	FUNC_EXIT();
	return rc;
}

static int sdio_tty_write_room(struct tty_struct *tty)
{
	FUNC_ENTER();

	if (!tty) {
		sp6502com_pr_err("NULL tty");
		return -ENODEV;
	}
	FUNC_EXIT();

	return sp6502com_sdio_tty_write_avail();
}

static int sdio_tty_chars_in_buffer(struct tty_struct *tty)
{
	FUNC_ENTER();
	sp6502com_pr_info("sdio_tty_chars_in_buffer\n");
	FUNC_EXIT();
	return 0;
}

static void sdio_tty_throttle(struct tty_struct *tty)
{
	struct sdio_tty_dev *port;

	FUNC_ENTER();

	port = sdio_dev;

	port->throttle_flag = 1;

	sp6502com_pr_info("sdio_tty_throttle\n");

	FUNC_EXIT();
}

static void sdio_tty_unthrottle(struct tty_struct *tty)
{
	struct sdio_tty_dev *port;
	FUNC_ENTER();

	if (!tty) {
		 sp6502com_pr_err("NULL tty\n");
		 return;
	}

	port = sdio_dev;

	port->throttle_flag = 0;

	sp6502com_pr_info("sdio_tty_unthrottle\n");

	if (push_sdio_tty_fail == 1) {
		push_sdio_tty_fail = 0;
		sp6502com_pr_info("unthrottle trigger\n");
		sdio_sp6502com_tty_retry_read();
	}

	FUNC_EXIT();
}

static int sdio_tty_tiocmget(struct tty_struct *tty)
{
	FUNC_ENTER();

	if (!tty) {
		sp6502com_pr_err("NULL tty\n");
		return -ENODEV;
	}
	FUNC_EXIT();

	return 0;
}

static int sdio_tty_tiocmset(struct tty_struct *tty,
				  unsigned int set, unsigned int clear)
{
	FUNC_ENTER();

	if (!tty) {
		sp6502com_pr_err("NULL tty\n");
		return -ENODEV;
	}
	FUNC_EXIT();

	return 0;
}


static int sdio_tty_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	if (!tty) {
		sp6502com_pr_err("NULL tty\n");
		return -ENODEV;
	}
	FUNC_ENTER();

	switch (cmd) {
		case SDIO_SP6502COM_DEBUG_LEVEL:
			if ((arg >= MSG_MSGDUMP) && (arg <= MSG_NONE)) {
				g_sdio_print_level = arg;
				sp6502com_pr_info("SDIO_SP6502COM: new debug level = %d\n",
					g_sdio_print_level);
			}
			break;
		case SDIO_SP6502COM_DUMP_BUF:
			if (arg == 0){
				print_hex_dump(KERN_CONT, "", DUMP_PREFIX_OFFSET,
								16, 1,
								sdio_dev->rbuf, 1024*8, false);
			}
			break;
		default:
			ret = -ENOIOCTLCMD;
			break;
		}

	return ret;
	FUNC_EXIT();
}

static const struct tty_operations sdio_tty_ops = {
	.open			= sdio_tty_open,
	.close			= sdio_tty_close,
	.write			= sdio_tty_write,
	.write_room		= sdio_tty_write_room,
	.chars_in_buffer	= sdio_tty_chars_in_buffer,
	.throttle		= sdio_tty_throttle,
	.unthrottle		= sdio_tty_unthrottle,
	.tiocmget		= sdio_tty_tiocmget,
	.tiocmset		= sdio_tty_tiocmset,
	.ioctl 			= sdio_tty_ioctl,
};

static int sdio_sp6502com_probe(struct sdio_func *func,
			   const struct sdio_device_id *id)
{
	struct sdio_tty_dev *port;
	int ret = 0;
	struct device *dev;

	FUNC_ENTER();

	if (func->device != 8800) {
		return -EINVAL;
	}

	sdio_removed = 0;

	if (sdio_dev) {
		sp6502com_pr_warn("Sdio_dev exists , skip probing\n");
		if (sdio_dev->opened == 1) {
			sdio_init_config = 0;
			sp6502com_sdio_irq_enable(sdio_dev);
		}
		probe_finish = SPRD_DETECT_FINISHED;
		wake_up_interruptible(&(init_wq));

		return ret;
	}

	sdio_dev = port = kzalloc(sizeof(struct sdio_tty_dev), GFP_KERNEL);

	if (!port)
		return -ENOMEM;

	port->func = func;
	port->index = 0;

	sdio_set_drvdata(func, port);

	dev = tty_register_device(sdio_tty_driver,
					port->index, &func->dev);

	if (IS_ERR(dev)) {
		kfree(port);
		port = NULL;
		ret = PTR_ERR(dev);
		return ret;
	}

	port->rbuf = (u8 *) kmalloc(SDIO_BUFFER_READ_SIZE, GFP_KERNEL);
		if (port->rbuf == NULL) {
		sp6502com_pr_err("sdio_dev->rbuf can not alloc buffer\n");
		ret = -ENOMEM;
		goto remove_tty;
	}

	port->rx_buf.buf_8 =  (unsigned char*)((((unsigned int)(port->rbuf) + 63) >> 6) << 6);

	port->rx_data_buf = (u8 *) kmalloc(SDIO_BUFFER_READ_SIZE, GFP_KERNEL);
	if (NULL == port->rx_data_buf) {
		sp6502com_pr_err("sdio_dev->rx_data_buf can not alloc buffer\n");
		ret =  -ENOMEM;
		goto free_rbuf;
	}
#ifdef CONFIG_SPRD_FLASHLESS
	port->tbuf = (u8 *) kmalloc(SPRD_FL_BUFFER_WRITE_SIZE, GFP_KERNEL);
#else
	port->tbuf = (u8 *) kmalloc(SDIO_BUFFER_WRITE_SIZE, GFP_KERNEL);
#endif
	if (port->tbuf == NULL) {
		sp6502com_pr_err("sdio_dev->tbuf can not alloc buffer\n");
		goto free_rx_data_buf;
	}
	port->tx_buf.buf_8 = (unsigned char*)((((unsigned int)(port->tbuf) + 63) >> 6) << 6);
#ifdef CONFIG_SPRD_FLASHLESS
	port->tx_data_buf = (u8 *) kmalloc(SPRD_FL_TTY_WRITE_SIZE, GFP_KERNEL);
#else
	port->tx_data_buf = (u8 *) kmalloc(SDIO_TTY_WRITE_SIZE, GFP_KERNEL);
#endif
	if (NULL == port->tx_data_buf) {
		sp6502com_pr_err("sdio_dev->tx_data_buf can not alloc buffer\n");
		goto free_tbuf;
	}

	port->tx_data_len = 0;
	port->rx_data_len = 0;
	port->last_tx_len= 0;
	port->enable_irq = false;
	port->modem_rts = 0;
	port->modem_rdy = 0;
	port->modem_rts = 0;
	port->modem_resend= 0;

	mutex_init(&port->func_lock);
	kref_init(&port->kref);

	init_waitqueue_head(&port->sdio_sleep_wq);
	init_waitqueue_head(&port->sdio_read_wq);
	init_waitqueue_head(&port->sdio_tty_unthrottle_wq);
	init_waitqueue_head(&port->sdio_tty_close_wq);
#if 1
	init_waitqueue_head(&port->conf_wq);
#endif
	port->rx_wakelock.wakelock_held= false;
	port->tx_wakelock.wakelock_held= false;
	wake_lock_init(&port->rx_wakelock.sprd_lock, WAKE_LOCK_SUSPEND,
				"sprd_rx");
	wake_lock_init(&port->tx_wakelock.sprd_lock, WAKE_LOCK_SUSPEND,
				"sprd_tx");

	init_waitqueue_head(&port->sdio_write_wq);
	init_waitqueue_head(&port->sdio_tty_write_wq);

#ifdef SDIO_TTY_DEBUG
	sdio_tty_dbg_createhost(port);
#endif
	port->sdio_tx_task = kthread_create(sdio_sp6502com_tx_thread, NULL, "sdio_sp6502_tx");
	wake_up_process(port->sdio_tx_task);

	port->sdio_rx_task = kthread_create(sdio_sp6502com_rx_thread, NULL, "sdio_sp6502_rx");

	wake_up_process(port->sdio_rx_task);
	sp6502com_pr_warn("sdio_sp6502com_probed\n");

	probe_finish = SPRD_DETECT_FINISHED;
	wake_up_interruptible(&(init_wq));

	FUNC_EXIT();
	return ret;

free_tbuf:
	kfree(port->tbuf);
	port->tbuf = NULL;

free_rx_data_buf:
	kfree(port->rx_data_buf);
	port->rx_data_buf = NULL;

free_rbuf:
	kfree(port->rbuf);
	port->rbuf = NULL;

remove_tty:
	tty_unregister_device(sdio_tty_driver, port->index);
	sdio_tty_dev_remove(port);
	FUNC_EXIT();
	return ret;
}

static void sdio_sp6502com_remove(struct sdio_func *func)
{
	FUNC_ENTER();
	sp6502com_pr_warn("sdio_sp6502com_removed\n");
	sdio_removed = 1;
	sdio_init_config = 0;
	sp6502com_sdio_irq_disable(sdio_dev);
	probe_finish = SPRD_DETECT_REMOVED;
	wake_up_interruptible(&(init_wq));

#if 0	
	struct sdio_tty_dev *port = sdio_get_drvdata(func);

	sdio_dev->remove_pending = 1;
	wake_up_interruptible(&(sdio_dev->sdio_read_wq));

	kthread_stop(port->sdio_rx_task);
	wake_up_interruptible(&(sdio_dev->sdio_write_wq));
	kthread_stop(port->sdio_tx_task);
	kfree(port->tx_data_buf);
	kfree(port->tbuf);
	kfree(port->rx_data_buf);
	kfree(port->rbuf);

	tty_unregister_device(sdio_tty_driver, port->index);

	sdio_tty_dev_remove(port);
#endif
	FUNC_EXIT();

}

static const struct sdio_device_id sdio_tty_ids[] = {
	{ SDIO_DEVICE(SP6502COM_VENDOR, SP6502COM_DEVICE)	},
	{ 					},
};

MODULE_DEVICE_TABLE(sdio, sdio_tty_ids);

static struct sdio_driver sdio_sp6502com_driver = {
	.name		= "sdio_sp6502com",
	.probe		= sdio_sp6502com_probe,
	.remove		= __devexit_p(sdio_sp6502com_remove),
	.id_table	= sdio_tty_ids,
};

static int sdio_tty_init(void)
{
	int ret;
	struct tty_driver *tty_drv;
	FUNC_ENTER();

	sdio_tty_driver = tty_drv = alloc_tty_driver(UART_NR);
	if (!tty_drv)
		return -ENOMEM;

	tty_drv->owner = THIS_MODULE;
	tty_drv->driver_name = "sdio_tty";
	tty_drv->name =   "ttyMux";
	tty_drv->major = 0;  
	tty_drv->minor_start = 0;
	tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
	tty_drv->subtype = SERIAL_TYPE_NORMAL;
	tty_drv->flags = TTY_DRIVER_RESET_TERMIOS |
		TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
	tty_drv->init_termios = tty_std_termios;
	tty_drv->init_termios.c_iflag = 0;
	tty_drv->init_termios.c_oflag = 0;
	tty_drv->init_termios.c_cflag = B115200 | CS8 |CREAD;
	tty_drv->init_termios.c_lflag = 0;

	tty_set_operations(tty_drv, &sdio_tty_ops);

	ret = tty_register_driver(tty_drv);
	if (ret)
		goto err1;

	ret = sdio_register_driver(&sdio_sp6502com_driver);
	if (ret)
		goto err2;

	init_waitqueue_head(&init_wq);

#ifdef SDIO_TTY_DEBUG
	ret = sdio_tty_dbg_init();
	if (ret) {
		pr_err("[SDIO] Failed to create debug fs dir \n");
		return ret;
	}

#endif
	FUNC_EXIT();
	return 0;

err2:
	tty_unregister_driver(tty_drv);
err1:
	put_tty_driver(tty_drv);
	return ret;
}

static void sdio_tty_exit(void)
{
	sdio_unregister_driver(&sdio_sp6502com_driver);
	tty_unregister_driver(sdio_tty_driver);
	put_tty_driver(sdio_tty_driver);
}

static int __devinit sprd_probe(struct platform_device *pdev)
{
	int ret = 0;

	ret =  sdio_tty_init();

	return ret;
}

static int __devinit sprd_remove(struct platform_device *pdev)
{
	sdio_tty_exit();

	return 0;
}

#ifdef CONFIG_PM
static int sprd_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;

	FUNC_ENTER();
	if (!sdio_dev) {
		sp6502com_pr_warn("sprd device removed\n");
		return ret;
	}
	sdio_dev->suspended = true;

	if (wake_lock_active(&(sdio_dev->rx_wakelock.sprd_lock)) ||
				wake_lock_active(&(sdio_dev->tx_wakelock.sprd_lock))) {
		sdio_dev->suspended = false;
		wake_up_interruptible(&(sdio_dev->sdio_sleep_wq));
		ret = -EBUSY;
	}

	FUNC_EXIT();

	return ret;
}

static int sprd_resume(struct platform_device *pdev)
{
	FUNC_ENTER();
	if (!sdio_dev) {
		sp6502com_pr_warn("sprd device removed\n");
		return 0;
	}

	sdio_dev->suspended = false;

	wake_up_interruptible(&(sdio_dev->sdio_sleep_wq));

	FUNC_EXIT();

	return 0;
}
#else
#define sprd_suspend	NULL
#define sprd_resume	NULL
#endif

static struct platform_device sprd_device = {
	.name = "sprd_ipc",
	.id	= -1,
};

static struct platform_driver sprd_driver = {
	.driver = {
		.name = "sprd_ipc",
		.owner = THIS_MODULE,
	},
	.probe		= sprd_probe,
	.suspend	= sprd_suspend,
	.resume		= sprd_resume,
	.remove		= __devexit_p(sprd_remove),
};

static int __init sprd_sdio_init(void)
{
	int ret;
	ret = platform_device_register(&sprd_device);
	if (ret) {
		sp6502com_pr_err("platform_device_register failed\n");
		goto err_platform_device_register;
	}

	ret = platform_driver_register(&sprd_driver);
	if (ret) {
		sp6502com_pr_err("platform_driver_register failed\n");
		goto err_platform_driver_register;
	}
	return ret;
err_platform_driver_register:
	platform_device_unregister(&sprd_device);
err_platform_device_register:
	return ret;
}

static void __exit sprd_sdio_exit(void)
{
	platform_driver_unregister(&sprd_driver);
	platform_device_unregister(&sprd_device);
}

module_init(sprd_sdio_init);
module_exit(sprd_sdio_exit);

MODULE_DESCRIPTION("HTC with spreadtrum SC6502/8502 SDIO Controller driver");
MODULE_AUTHOR("Bob Song <bob_song@htc.com>");
MODULE_LICENSE("GPL");

#ifdef SDIO_TTY_DEBUG
static int
sdio_tty_dbg_state_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t
sdio_tty_dbg_state_read(struct file *file, char __user *ubuf,
			   size_t count, loff_t *ppos)
{
	pr_info("Sdio_sp6502com:bp_rts() = %d\n", bp_rts());
	sdio_dev->modem_rts = 1;
	wake_up_interruptible(&(sdio_dev->sdio_read_wq));

	return -1;
}

static ssize_t
sdio_tty_dbg_state_write(struct file *file,
				 const char __user *ubuf,
				 size_t count, loff_t *ppos)
{
	char buf[200];

	if (copy_from_user(buf, ubuf, min(count, sizeof(buf))))
		return -EFAULT;

	switch (buf[0]) {

		case '1':				
			pr_info("Sdio_sp6502com:bp_rts() = %d\n", bp_rts());
			sdio_dev->modem_rts = 1;
			wake_up_interruptible(&(sdio_dev->sdio_read_wq));
			break;

		case '2':
			g_sdio_print_level = MSG_MSGDUMP;
			break;

		case '3':
			g_sdio_print_level = MSG_WARNING;
			break;

		case '4':
			pr_info("[SDIO_SP6502COM]:Sdio_sp6502com: status = %d\n",sprd_get_status());
			pr_info("[SDIO_SP6502COM]:Mdm rts Irq count = %u\n",rts_irqcount);
			pr_info("[SDIO_SP6502COM]:Mdm rdy Irq count = %u\n",rdy_irqcount);
			pr_info("[SDIO_SP6502COM]:Data packet read count = %u\n",data_rx_count);
			pr_info("[SDIO_SP6502COM]:Data packet write count = %u\n",data_tx_count);
			pr_info("[SDIO_SP6502COM]:Data read total size = %lu KByte\n",data_rx_size/1024);
			pr_info("[SDIO_SP6502COM]:Data write total size = %lu KByte\n",data_tx_size/1024);
			pr_info("[SDIO_SP6502COM]:Tx packet frame num = %u\n",tx_packet_count);
			pr_info("[SDIO_SP6502COM]:Rx packet frame num = %u\n",rx_packet_count);
			pr_info("[SDIO_SP6502COM]:Rx reset count = %u\n",reset_count);
#ifdef SDIO_SP6502_PERF_PROFILING
			pr_info("[SDIO_SP6502COM]:Tx transfer time = %lld ms\n",ktime_to_ms(tx_tran));
			pr_info("[SDIO_SP6502COM]:Rx transfer time = %lld ms\n",ktime_to_ms(rx_tran));
			pr_info("[SDIO_SP6502COM]:Tx total time = %lld ms\n",ktime_to_ms(tx_total));
			pr_info("[SDIO_SP6502COM]:Rx total time = %lld ms\n",ktime_to_ms(rx_total));
			pr_info("[SDIO_SP6502COM]:Rx total(tty) time = %lld ms\n",ktime_to_ms(rx_tty_time));
#endif
			break;
		case 'b':
			print_hex_dump(KERN_CONT, "", DUMP_PREFIX_OFFSET,
							16, 1,
							sdio_dev->rbuf, SDIO_BUFFER_READ_SIZE, false);
			break;
		default :
			pr_info("Unknown case\n");
			break;
	}

	return count;
}

static const struct file_operations sdio_tty_dbg_state_ops = {
	.read	= sdio_tty_dbg_state_read,
	.write	= sdio_tty_dbg_state_write,
	.open	= sdio_tty_dbg_state_open,
};

static void sdio_tty_dbg_createhost(struct sdio_tty_dev *port)
{
	if (sdio_debugfs_dir) {
		sdio_debugfs_file = debugfs_create_file("sdio_sp6502com",
							0644, sdio_debugfs_dir, port,
							&sdio_tty_dbg_state_ops);
	}
}

static int __init sdio_tty_dbg_init(void)
{
	int err;

	sdio_debugfs_dir = debugfs_create_dir("sdio_tty", 0);
	if (IS_ERR(sdio_debugfs_dir)) {
		err = PTR_ERR(sdio_debugfs_dir);
		sdio_debugfs_dir = NULL;
		return err;
	}

	return 0;
}
#endif
