/*
 * File: ts27010.h
 *
 * Portions derived from rfcomm.c, original header as follows:
 *
 * Copyright (C) 2000, 2001  Axis Communications AB
 * Copyright (C) 2002, 2004, 2009 Motorola
 *
 * Author: Mats Friden <mats.friden@axis.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Exceptionally, Axis Communications AB grants discretionary and
 * conditional permissions for additional use of the text contained
 * in the company's release of the AXIS OpenBT Stack under the
 * provisions set forth hereunder.
 *
 * Provided that, if you use the AXIS OpenBT Stack with other files,
 * that do not implement functionality as specified in the Bluetooth
 * System specification, to produce an executable, this does not by
 * itself cause the resulting executable to be covered by the GNU
 * General Public License. Your use of that executable is in no way
 * restricted on account of using the AXIS OpenBT Stack code with it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the provisions of the GNU
 * General Public License.
 *
 */
#ifndef __TS27010_H__
#define __TS27010_H__






#define DUMP_FRAME

#ifdef UART_MUX_LOG
#define LOG_BUFFER_SIZE (128 * 1024 - sizeof(struct ts27010_ringbuf))
#define MUX_LOG_WATERMARK_LOW (64*1024)
#define MUX_LOG_WATERMARK_HIGH (96*1024)
#define UART_LOG_CHANNEL 12
extern void mux_log_throttle(int on);
#endif

#define MUX_FORCE_CLOSE_BY_GPIO

#ifndef PROC_DEBUG_MUX
#define PROC_DEBUG_MUX
#ifndef PROC_DEBUG_MUX_STAT
#define PROC_DEBUG_MUX_STAT
#endif
#endif

#ifndef DEBUG
#define DEBUG
#endif

#ifndef TS27010_NET
#define TS27010_NET
#endif

#define QUEUE_SELF

#define TS0710_MAX_CHN 14
#define TS0710_MAX_MUX 13

#define TS0710MUX_TIME_OUT 250

#define DAEMONIZE(a) do { \
	daemonize(a); \
	allow_signal(SIGKILL); \
	allow_signal(SIGTERM); \
} while (0)

#define KILL_PROC(pid, sig) do { \
	struct task_struct *tsk; \
	tsk = find_task_by_vpid(pid); \
	if (tsk) \
		send_sig(sig, tsk, 1); \
} while (0)

#define SET_PF(ctr) ((ctr) | (1 << 4))
#define CLR_PF(ctr) ((ctr) & 0xef)
#define GET_PF(ctr) (((ctr) >> 4) & 0x1)

#define SHORT_PAYLOAD_SIZE 127

#define EA 1
#define FCS_SIZE 1
#define FLAG_SIZE 2

#define ADDRESS_OFFSET 1
#define CONTROL_OFFSET 2

#ifdef TS27010_UART_RETRAN
#define SEQUENCE_OFFSET 3
#define LENGTH_OFFSET 4
#define DEF_TS0710_MTU 512
#else
#define LENGTH_OFFSET 3
#define DEF_TS0710_MTU 2048
#endif

#define CTRL_CHAN 0

#define TS0710_FRAME_SIZE(len)						\
	((len) > SHORT_PAYLOAD_SIZE ?					\
	 (len) + FLAG_SIZE + sizeof(struct long_frame) + FCS_SIZE :	\
	 (len) + FLAG_SIZE + sizeof(struct short_frame) + FCS_SIZE)

#define TS0710_MCC_FRAME_SIZE(len) \
	TS0710_FRAME_SIZE((len) + sizeof(struct mcc_short_frame))

#define TS0710MUX_SEND_BUF_SIZE \
	((TS0710_FRAME_SIZE(DEF_TS0710_MTU) + 3) \
	& 0xFFFC)

#define TS0710_BASIC_FLAG 0xF9

#define SABM 0x2f
#define SABM_SIZE 4
#define UA 0x63
#define UA_SIZE 4
#define DM 0x0f
#define DISC 0x43
#define UIH 0xef

#define TEST 0x8
#define FCON 0x28
#define FCOFF 0x18
#define MSC 0x38
#define RPN 0x24
#define RLS 0x14
#define PN 0x20
#define NSC 0x4

#define FC 0x2
#define RTC 0x4
#define RTR 0x8
#define IC 0x40
#define DV 0x80

#define CTRL_CHAN 0		
#define MCC_CR 0x2
#define MCC_CMD 1		
#define MCC_RSP 0		

static inline int mcc_is_cmd(u8 type)
{
	return type & MCC_CR;
}

static inline int mcc_is_rsp(u8 type)
{
	return !(type & MCC_CR);
}


#ifdef __LITTLE_ENDIAN_BITFIELD
struct address_field {
	u8 ea:1;
	u8 cr:1;
	u8 dlci:6;
} __attribute__ ((packed));

static inline int ts0710_dlci(u8 addr)
{
	return (addr >> 2) & 0x3f;
}

struct short_length {
	u8 ea:1;
	u8 len:7;
} __attribute__ ((packed));

struct long_length {
	u8 ea:1;
	u8 l_len:7;
	u8 h_len;
} __attribute__ ((packed));

struct short_frame_head {
	struct address_field addr;
	u8 control;
#ifdef TS27010_UART_RETRAN
	u8 sn;
#endif
	struct short_length length;
} __attribute__ ((packed));

struct short_frame {
	struct short_frame_head h;
	u8 data[0];
} __attribute__ ((packed));

struct long_frame_head {
	struct address_field addr;
	u8 control;
#ifdef TS27010_UART_RETRAN
	u8 sn;
#endif
	struct long_length length;
	u8 data[0];
} __attribute__ ((packed));

struct long_frame {
	struct long_frame_head h;
	u8 data[0];
} __attribute__ ((packed));

struct mcc_type {
	u8 ea:1;
	u8 cr:1;
	u8 type:6;
} __attribute__ ((packed));

struct mcc_short_frame_head {
	struct mcc_type type;
	struct short_length length;
	u8 value[0];
} __attribute__ ((packed));

struct mcc_short_frame {
	struct mcc_short_frame_head h;
	u8 value[0];
} __attribute__ ((packed));

struct mcc_long_frame_head {
	struct mcc_type type;
	struct long_length length;
	u8 value[0];
} __attribute__ ((packed));

struct mcc_long_frame {
	struct mcc_long_frame_head h;
	u8 value[0];
} __attribute__ ((packed));

struct v24_sigs {
	u8 ea:1;
	u8 fc:1;
	u8 rtc:1;
	u8 rtr:1;
	u8 reserved:2;
	u8 ic:1;
	u8 dv:1;
} __attribute__ ((packed));

struct brk_sigs {
	u8 ea:1;
	u8 b1:1;
	u8 b2:1;
	u8 b3:1;
	u8 len:4;
} __attribute__ ((packed));

struct msc_msg_data {
	struct address_field dlci;
	u8 v24_sigs;
} __attribute__ ((packed));

struct msc_msg {
	struct short_frame_head s_head;
	struct mcc_short_frame_head mcc_s_head;
	struct address_field dlci;
	u8 v24_sigs;
	u8 fcs;
} __attribute__ ((packed));

struct pn_msg_data {
	u8 dlci:6;
	u8 res1:2;

	u8 frame_type:4;
	u8 credit_flow:4;

	u8 prior:6;
	u8 res2:2;

	u8 ack_timer;
	u8 frame_sizel;
	u8 frame_sizeh;
	u8 max_nbrof_retrans;
	u8 credits;
} __attribute__ ((packed));


#else
#error Only littel-endianess supported now!
#endif

enum {
	DISCONNECTED = 0,
	DISCONNECTING,
	NEGOTIATING,
	CONNECTING,
	CONNECTED,
	FLOW_STOPPED,
	REJECTED
};


struct dlci_struct {
	int clients;
	struct mutex lock; 
	wait_queue_head_t open_wait;
	wait_queue_head_t close_wait;
	wait_queue_head_t mux_write_wait;

	u8 state;
	u8 flow_control;
	u8 initiator;
	u8 dummy;
	u16 mtu;
	u16 dummy2;
	
#ifdef TS27010_NET
	struct net_device *net; 
	int line_no;
#endif
	
};

struct chan_struct {
	struct mutex	write_lock; 
	u8		buf[TS0710MUX_SEND_BUF_SIZE];
};

struct ts0710_con {
	struct chan_struct	chan[TS0710_MAX_MUX];
	struct dlci_struct	dlci[TS0710_MAX_CHN];

	wait_queue_head_t test_wait;
	u32 test_errs;
	u8 initiator;
	u8 be_testing;
	u16 dummy;
};

enum { MSG_MSGDUMP, MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_LIGHT, MSG_ERROR, MSG_CRIT, MSG_NONE };

#ifdef DEBUG
extern int g_mux_uart_print_level;
extern void sched_show_task(struct task_struct *p);

#define kassert(cond) do { \
	if (!(cond)) { \
		printk(KERN_WARNING "MUX_UART_assert: %s failed at %s:%d\n", \
			#cond, __FILE__, __LINE__); \
		sched_show_task(current); \
	} \
} while (0)

static const char strLog[7] = {
	'M',
	'D',
	'I',
	'W',
	'L',
	'E',
	'C'
};
#define mux_print(level, fmt, args...) do { \
	if (level >= g_mux_uart_print_level) \
		printk(KERN_WARNING "MUX_UART<%c>[%s:%d]: " fmt, \
			strLog[level], __func__, __LINE__, ##args); \
} while (0)
#define FUNC_ENTER() do { \
	mux_print(MSG_MSGDUMP, "enter\n");\
} while (0)
#define FUNC_EXIT() do { \
	mux_print(MSG_MSGDUMP, "exit\n");\
} while (0)
void mux_uart_hexdump(int level, const char *title,
			const char *function, u32 line,
			const u8 *buf, u32 len);
#else 
#define mux_print(level, fmt, args...) do { } while (0)
#define mux_uart_hexdump(lev, t, f, line, buf, len) do { } while (0)
#define kassert(cond) do { } while (0)
#define FUNC_ENTER() do { } while (0)
#define FUNC_EXIT() do { } while (0)
#endif 

#endif 
