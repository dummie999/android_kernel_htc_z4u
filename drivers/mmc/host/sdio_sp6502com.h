#ifndef __SDIO_SP6502COM_H__
#define __SDIO_SP6502COM_H__

enum { MSG_MSGDUMP, MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_LIGHT, MSG_ERROR, MSG_NONE };

#define SDIO_SP6502COM_DEBUG 1

#ifdef SDIO_SP6502COM_DEBUG

#define SDIO_SP6502COM_DEBUG_LEVEL 0x5483
#define SDIO_SP6502COM_DUMP_BUF	0x5484

#define SDIO_TTY_DEBUG 1
int g_sdio_print_level = 3;

static const char Logstr[6] = {
	'M',
	'D',
	'I',
	'W',
	'L',
	'E',
};

#define sdio_print(level, fmt, args...) do { \
	if (level >= g_sdio_print_level) \
		printk(KERN_WARNING "[SDIO_SPRD]<%c>[%s:%d]: " fmt, \
			Logstr[level], __func__, __LINE__, ##args); \
} while (0)

#define sp6502com_pr_info(fmt, args...)  do { \
	sdio_print(MSG_INFO, fmt, ##args);\
} while (0)

#define sp6502com_pr_err(fmt, args...)  do { \
	sdio_print(MSG_ERROR, fmt, ##args);\
} while (0)

#define sp6502com_pr_warn(fmt, args...) do { \
	sdio_print(MSG_WARNING, fmt, ##args);\
} while (0)

#define sp6502com_pr_light(fmt, args...) do { \
	sdio_print(MSG_LIGHT, fmt, ##args);\
} while (0)

#define FUNC_ENTER() do { \
	sdio_print(MSG_MSGDUMP, "enter\n");\
} while (0)

#define FUNC_EXIT() do { \
	sdio_print(MSG_MSGDUMP, "exit\n");\
} while (0)


#else 

#define sdio_print(level, fmt, args...) do { } while (0)

#define sp6502com_pr_info(fmt, args...)  do { } while (0)

#define sp6502com_pr_err(fmt, args...)  do { } while (0)

#define sp6502com_pr_warn(fmt, args...) do { } while (0)

#define sp6502com_pr_light(fmt, args...) do { } while (0)

#define FUNC_ENTER() do { } while (0)

#define FUNC_EXIT() do { } while (0)

#endif 

int sdio_platform_tty_throttle_status(void);

static void sdio_tty_read(void);

static struct sdio_tty_dev *sdio_dev;

extern void modem_detect(bool,unsigned int);
void modem_sdio_reset(int on);

#define SDIO_SP6502_CONFLICT_METHOD	1
#define SDIO_SP6502_PERF_PROFILING	1
#define SDIO_RX_RESEND_ENABLED	1

#define AP_RSD				(8)
#define AP_RDY				(5)
#define AP_RTS				(6)

#define BP_RTS				(27)
#define BP_RDY				(17)
#define BP_RSD				(96)

#define SP6502COM_ADDR          	(0x00)
#define SDIO_BUFFER_READ_ONCE		(4096*2)
#define SDIO_SP6502COM_PACKET_HEADER_LEN (16)
#if 0
#define SDIO_BUFFER_READ_SIZE		(4096*4)
#define SDIO_BUFFER_WRITE_SIZE		(4096*2)
#define SDIO_TTY_WRITE_SIZE		(4096*2 - SDIO_SP6502COM_PACKET_HEADER_LEN)
#else
#define SDIO_BUFFER_READ_SIZE		(1024*16)
#define SDIO_BUFFER_WRITE_SIZE		(1024*8)
#define SDIO_TTY_WRITE_SIZE (1024*8 - SDIO_SP6502COM_PACKET_HEADER_LEN)
#define SPRD_FL_BUFFER_WRITE_SIZE	(1024*16)
#define SPRD_FL_TTY_WRITE_SIZE 		(1024*16 - SDIO_SP6502COM_PACKET_HEADER_LEN)
#endif
#define SP6502COM_VENDOR		(0x0)
#define SP6502COM_DEVICE		(8800)
#define SDIO_SP6502COM_MODEM_PACKET_HEADER_TAG 		0x7E7F
#define SDIO_SP6502COM_MODEM_PACKET_TYPE		0xAA55
#define SDIO_BLOCK_SIZE			(512)

#define SDIO_OPENING			(0x00)
#define SDIO_VALID			(0x01)
#define SDIO_IO_RECEIVING		(0X02)
#define	SDIO_RECEIVING_DATA		(0x04)
#define	SDIO_IO_SENDING			(0x08)

#define UART_NR			1	

#define	SPRD_DETECT_NONE  		(0x00)
#define	SPRD_DETECT_ONGOING 		(0x01)
#define	SPRD_DETECT_FINISHED 		(0x02)
#define	SPRD_DETECT_TIMEOUT 		(0x03)
#define	SPRD_DETECT_REMOVED 		(0x04)

#define SPRD_CARD_REMOVED	(1<<7)	
#define sprd_set_removed(c) ((c)->state |= SPRD_CARD_REMOVED)

struct sprd_wakelock {
	struct wake_lock	sprd_lock;
	bool			wakelock_held;
};

struct sdio_tty_dev {
	struct tty_port		port;
	struct kref		kref;
	struct tty_struct	*tty;
	unsigned int		index;
	unsigned int		status;
	struct sdio_func	*func;
	struct mutex		func_lock;
	struct task_struct	*in_sdio_uart_irq;
	unsigned int		regs_offset;

	union {
		char 		*buf_8;
		short 		*buf_16;
		int 		*buf_32;
	} rx_buf;

	char 			*rx_data_buf;

	volatile int 		throttle_flag;
	volatile int 		modem_rts;
	volatile int 		modem_rdy;
	volatile int 		modem_resend;

	wait_queue_head_t	sdio_sleep_wq;
	wait_queue_head_t 	sdio_read_wq;
	wait_queue_head_t	sdio_tty_unthrottle_wq;
	wait_queue_head_t	sdio_tty_close_wq;

	wait_queue_head_t	conf_wq;


	int			mdm_rts_irq;
	int 			mdm_rdy_irq;

	struct task_struct	*sdio_rx_task;
	unsigned char 		*rbuf;
	volatile unsigned short 		rx_data_len;
	struct timer_list 	buf_req_timer;
	int			open_count;
	bool  			suspended;
	bool   			opened;
	bool   			enable_irq;
	bool			header_read;
	bool			remove_pending;
	int			last_tx_len;
	struct sprd_wakelock	rx_wakelock;
	struct sprd_wakelock	tx_wakelock;

	union {
		char 		*buf_8;
		short 		*buf_16;
		int 		*buf_32;
	} tx_buf;

	char 			*tx_data_buf;
	volatile short 		tx_data_len;
	struct task_struct	*sdio_tx_task;

	unsigned char 		*tbuf;
	wait_queue_head_t 	sdio_write_wq;
	wait_queue_head_t 	sdio_tty_write_wq;
};

struct sdio_sp6502com_modem_packet_header_t {
	unsigned short head_tag;		
	unsigned short packet_type;		
	unsigned int   len;
	unsigned int   frame_num;
	unsigned int   reserved;
};

struct sdio_sp6502com_modem_packet_t {
	struct sdio_sp6502com_modem_packet_header_t header;
};

#endif  
