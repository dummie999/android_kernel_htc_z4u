#define pr_fmt(fmt) "[SPI CPLD] " fmt
#define pr_error(fmt) pr_err("ERROR: " fmt)

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/spi/cpld.h>
#include <linux/slab.h>
#include <linux/delay.h>

#ifdef CONFIG_MACH_DUMMY
#include "../../arch/arm/mach-msm/board-protodcg.h"
#elif (defined CONFIG_MACH_DUMMY)
#include "../../arch/arm/mach-msm/board-cp3dcg.h"
#elif (defined CONFIG_MACH_DUMMY)
#include "../../arch/arm/mach-msm/board-cp3dug.h"
#elif (defined CONFIG_MACH_DUMMY)
#include "../../arch/arm/mach-msm/board-cp3dtg.h"
#elif (defined CONFIG_MACH_DUMMY)
#include "../../arch/arm/mach-msm/board-cp3u.h"
#elif (defined CONFIG_MACH_DUMMY)
#include "../../arch/arm/mach-msm/board-z4dug.h"
#elif (defined CONFIG_MACH_DUMMY)
#include "../../arch/arm/mach-msm/board-z4dcg.h"
#elif (defined CONFIG_MACH_Z4U)
#include "../../arch/arm/mach-msm/board-z4u.h"
#endif



#define CPLD_BUFSIZ		128

#define INTF_SPI		0
#define INTF_UART		1

#define TMUX_EBI2_OFFSET		0x204
#define EBI2_XMEM_CS0_CFG1_OFFSET	0x10028

struct cpld_driver {
	struct kobject		kobj;
	struct device		*dev;

	void __iomem		*cfg_base;
	void __iomem		*clk_base;
	void __iomem		*reg_base;
	void __iomem		*gpio_base;
	void __iomem		*sdc4_base;

	phys_addr_t		cfg_start;

	struct spi_device	*spi;
	struct spi_message	msg;
	struct spi_transfer	xfer;
	uint8_t			buf[CPLD_BUFSIZ];

	uint8_t			sysfs_buf[CPLD_BUFSIZ];
	int			sysfs_cnt;
	int			cfg_offs;
	int			clk_offs;
	int			gpio_offs;
	uint8_t			reg_addr;

	struct cpld_platform_data *pdata;
};

typedef volatile struct _cpld_reg_ {

	unsigned short tx_buffer1_cs_low;	
	unsigned short tx_buffer2_cs_low;	
	unsigned short version1_1;		
	unsigned short rx_buffer1_cs_low;	
	unsigned short tx_buffer1_cs_high;	
	unsigned short tx_buffer2_cs_high;	
	unsigned short version1_2;		
	unsigned short rx_buffer1_cs_high;	

} cpld_reg, *p_cpld_reg;

typedef enum _buf_num_ {
	tx_buffer_1,
	tx_buffer_2,
}enum_buf_num;

typedef struct _cpld_manager_
{
	p_cpld_reg p_cpld_reg;
	enum_buf_num current_tx_buf_num;
}cpld_manager;

cpld_manager g_cpld_manager =  { 0 };
static struct cpld_driver *g_cpld;

#define GPIO_SPI 0
#define EBI2_SPI 1

#ifdef CONFIG_MACH_DUMMY
#define EBI2_ADDR_0    PROTODCG_GPIO_ADDR_0
#define EBI2_ADDR_1    PROTODCG_GPIO_ADDR_1
#define EBI2_ADDR_2    PROTODCG_GPIO_ADDR_2
#define EBI2_DATA_0    PROTODCG_GPIO_DATA_0
#define EBI2_DATA_1    PROTODCG_GPIO_DATA_1
#define EBI2_DATA_2    PROTODCG_GPIO_DATA_2
#define EBI2_DATA_3    PROTODCG_GPIO_DATA_3
#define EBI2_DATA_4    PROTODCG_GPIO_DATA_4
#define EBI2_DATA_5    PROTODCG_GPIO_DATA_5
#define EBI2_DATA_6    PROTODCG_GPIO_DATA_6
#define EBI2_DATA_7    PROTODCG_GPIO_DATA_7
#define EBI2_OE        PROTODCG_GPIO_OE
#define EBI2_WE        PROTODCG_GPIO_WE
#elif (defined CONFIG_MACH_DUMMY)
#define EBI2_ADDR_0    CP3DCG_GPIO_ADDR_0
#define EBI2_ADDR_1    CP3DCG_GPIO_ADDR_1
#define EBI2_ADDR_2    CP3DCG_GPIO_ADDR_2
#define EBI2_DATA_0    CP3DCG_GPIO_DATA_0
#define EBI2_DATA_1    CP3DCG_GPIO_DATA_1
#define EBI2_DATA_2    CP3DCG_GPIO_DATA_2
#define EBI2_DATA_3    CP3DCG_GPIO_DATA_3
#define EBI2_DATA_4    CP3DCG_GPIO_DATA_4
#define EBI2_DATA_5    CP3DCG_GPIO_DATA_5
#define EBI2_DATA_6    CP3DCG_GPIO_DATA_6
#define EBI2_DATA_7    CP3DCG_GPIO_DATA_7
#define EBI2_OE        CP3DCG_GPIO_OE
#define EBI2_WE        CP3DCG_GPIO_WE
#elif (defined CONFIG_MACH_DUMMY)
#define EBI2_ADDR_0    CP3DUG_GPIO_ADDR_0
#define EBI2_ADDR_1    CP3DUG_GPIO_ADDR_1
#define EBI2_ADDR_2    CP3DUG_GPIO_ADDR_2
#define EBI2_DATA_0    CP3DUG_GPIO_DATA_0
#define EBI2_DATA_1    CP3DUG_GPIO_DATA_1
#define EBI2_DATA_2    CP3DUG_GPIO_DATA_2
#define EBI2_DATA_3    CP3DUG_GPIO_DATA_3
#define EBI2_DATA_4    CP3DUG_GPIO_DATA_4
#define EBI2_DATA_5    CP3DUG_GPIO_DATA_5
#define EBI2_DATA_6    CP3DUG_GPIO_DATA_6
#define EBI2_DATA_7    CP3DUG_GPIO_DATA_7
#define EBI2_OE        CP3DUG_GPIO_OE
#define EBI2_WE        CP3DUG_GPIO_WE
#elif (defined CONFIG_MACH_DUMMY)
#define EBI2_ADDR_0    CP3DTG_GPIO_ADDR_0
#define EBI2_ADDR_1    CP3DTG_GPIO_ADDR_1
#define EBI2_ADDR_2    CP3DTG_GPIO_ADDR_2
#define EBI2_DATA_0    CP3DTG_GPIO_DATA_0
#define EBI2_DATA_1    CP3DTG_GPIO_DATA_1
#define EBI2_DATA_2    CP3DTG_GPIO_DATA_2
#define EBI2_DATA_3    CP3DTG_GPIO_DATA_3
#define EBI2_DATA_4    CP3DTG_GPIO_DATA_4
#define EBI2_DATA_5    CP3DTG_GPIO_DATA_5
#define EBI2_DATA_6    CP3DTG_GPIO_DATA_6
#define EBI2_DATA_7    CP3DTG_GPIO_DATA_7
#define EBI2_OE        CP3DTG_GPIO_OE
#define EBI2_WE        CP3DTG_GPIO_WE
#elif (defined CONFIG_MACH_DUMMY)
#define EBI2_ADDR_0    CP3U_GPIO_ADDR_0
#define EBI2_ADDR_1    CP3U_GPIO_ADDR_1
#define EBI2_ADDR_2    CP3U_GPIO_ADDR_2
#define EBI2_DATA_0    CP3U_GPIO_DATA_0
#define EBI2_DATA_1    CP3U_GPIO_DATA_1
#define EBI2_DATA_2    CP3U_GPIO_DATA_2
#define EBI2_DATA_3    CP3U_GPIO_DATA_3
#define EBI2_DATA_4    CP3U_GPIO_DATA_4
#define EBI2_DATA_5    CP3U_GPIO_DATA_5
#define EBI2_DATA_6    CP3U_GPIO_DATA_6
#define EBI2_DATA_7    CP3U_GPIO_DATA_7
#define EBI2_OE        CP3U_GPIO_OE
#define EBI2_WE        CP3U_GPIO_WE
#elif (defined CONFIG_MACH_DUMMY)
#define EBI2_ADDR_0    Z4DUG_GPIO_ADDR_0
#define EBI2_ADDR_1    Z4DUG_GPIO_ADDR_1
#define EBI2_ADDR_2    Z4DUG_GPIO_ADDR_2
#define EBI2_DATA_0    Z4DUG_GPIO_DATA_0
#define EBI2_DATA_1    Z4DUG_GPIO_DATA_1
#define EBI2_DATA_2    Z4DUG_GPIO_DATA_2
#define EBI2_DATA_3    Z4DUG_GPIO_DATA_3
#define EBI2_DATA_4    Z4DUG_GPIO_DATA_4
#define EBI2_DATA_5    Z4DUG_GPIO_DATA_5
#define EBI2_DATA_6    Z4DUG_GPIO_DATA_6
#define EBI2_DATA_7    Z4DUG_GPIO_DATA_7
#define EBI2_OE        Z4DUG_GPIO_OE
#define EBI2_WE        Z4DUG_GPIO_WE
#elif (defined CONFIG_MACH_DUMMY)
#define EBI2_ADDR_0    Z4DCG_GPIO_ADDR_0
#define EBI2_ADDR_1    Z4DCG_GPIO_ADDR_1
#define EBI2_ADDR_2    Z4DCG_GPIO_ADDR_2
#define EBI2_DATA_0    Z4DCG_GPIO_DATA_0
#define EBI2_DATA_1    Z4DCG_GPIO_DATA_1
#define EBI2_DATA_2    Z4DCG_GPIO_DATA_2
#define EBI2_DATA_3    Z4DCG_GPIO_DATA_3
#define EBI2_DATA_4    Z4DCG_GPIO_DATA_4
#define EBI2_DATA_5    Z4DCG_GPIO_DATA_5
#define EBI2_DATA_6    Z4DCG_GPIO_DATA_6
#define EBI2_DATA_7    Z4DCG_GPIO_DATA_7
#define EBI2_OE        Z4DCG_GPIO_OE
#define EBI2_WE        Z4DCG_GPIO_WE
#elif (defined CONFIG_MACH_Z4U)
#define EBI2_ADDR_0    Z4U_GPIO_ADDR_0
#define EBI2_ADDR_1    Z4U_GPIO_ADDR_1
#define EBI2_ADDR_2    Z4U_GPIO_ADDR_2
#define EBI2_DATA_0    Z4U_GPIO_DATA_0
#define EBI2_DATA_1    Z4U_GPIO_DATA_1
#define EBI2_DATA_2    Z4U_GPIO_DATA_2
#define EBI2_DATA_3    Z4U_GPIO_DATA_3
#define EBI2_DATA_4    Z4U_GPIO_DATA_4
#define EBI2_DATA_5    Z4U_GPIO_DATA_5
#define EBI2_DATA_6    Z4U_GPIO_DATA_6
#define EBI2_DATA_7    Z4U_GPIO_DATA_7
#define EBI2_OE        Z4U_GPIO_OE
#define EBI2_WE        Z4U_GPIO_WE
#endif

static void clock_setting(struct cpld_driver *cpld, int enable)
{
	pr_info("%s\n", __func__);
	
	if (cpld) {
		if(cpld->pdata->clock_set){
			pr_info("CPLD true: call clock_set(%d)\n", enable);
			cpld->pdata->clock_set(enable);

			if(enable) {
				writel(0x01, cpld->sdc4_base);			
				writel(0x18100, cpld->sdc4_base + 0x04);	
			} else {
				writel(0x00, cpld->sdc4_base);			
				writel(0x08000, cpld->sdc4_base + 0x04);	
			}
		}
	}
	else
	{
		pr_err("%s. Poiner is NULL.\n", __func__);
	}
}

int spi_set_route(int path)
{
#if !((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_Z4U))
	struct cpld_driver *cpld = g_cpld;

	clock_setting(cpld, 1);
	
	if (path == EBI2_SPI) {
		if(cpld->pdata->intf_select)
			gpio_direction_output(cpld->pdata->intf_select, 1);
	
		writel(0, cpld->gpio_base + TMUX_EBI2_OFFSET);
	} else {
		if(cpld->pdata->intf_select)
			gpio_direction_output(cpld->pdata->intf_select, 0);

		writel(0x10, cpld->gpio_base + TMUX_EBI2_OFFSET);
	}

	pr_info("TMUX_EBI2: 0x%08x\n", readl(cpld->cfg_base));
#endif
	return 0;
}

EXPORT_SYMBOL(spi_set_route);


int gpio_spi_write(int length, unsigned char * buffer)
{
	struct spi_device *spi = g_cpld->spi;
	struct spi_message msg;
	struct spi_transfer xfer;
	int err = -1;
	unsigned char *buf;
	int len = 0;

	if (!buffer) {
		pr_info("%s. Pointer is NULL.\n", __func__);
		return err;
	}
	
	buf = buffer;
	len = length;
	
	xfer.tx_buf = buf;
	pr_info("%s: len = %d, bits = %d\n", __func__,
				len, xfer.bits_per_word);

	memset(&xfer, 0, sizeof(xfer));
	xfer.tx_buf = buf;
	xfer.len = len;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	err = spi_sync(spi, &msg);
	pr_info("%s: err = %d\n", __func__, err);

	return err;
}
EXPORT_SYMBOL(gpio_spi_write);

#if 0
static cpld_gpiospi_single_write(struct cpld_driver *cpld,
				uint16_t index, uint8_t value)
{
#if 0
	struct spi_device *spi = cpld->spi;
	struct spi_message msg;
	struct spi_transfer xfer;
	uint8_t buf[4];
	int err;

	buf[0] = 0x61;
	buf[1] = (index >> 8) & 0xff;
	buf[2] = index & 0xff;
	buf[3] = value;

	xfer.bits_per_word = 0;
	xfer.tx_buf = buf;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	err = spi_sync(spi, &msg);

	pr_info("%s: err = %d\n", __func__, err);
	return err;
#else
	struct spi_device *spi = cpld->spi;
	struct spi_message *msg = &cpld.msg;
	struct spi_transfer *xfer = &cpld.xfer;
	uint8_t buf[4];
	int err;

	buf[0] = 0x60;
	buf[1] = (index >> 8) & 0xff;
	buf[2] = index & 0xff;
	buf[3] = value;

	xfer->tx_buf = buf;
	xfer->rx_buf = NULL;

	

	err = spi_sync(spi, msg);

	pr_info("%s: err = %d\n", __func__, err);
	return err;
#endif
}
#endif

static int cpld_gpiospi_single_read(struct cpld_driver *cpld,
				uint16_t index, uint8_t *value)
{
	struct spi_message msg;
	int err;
	uint8_t tx_buf[3] = { 0x60, (index >> 8) & 0xff, index & 0xff };
	uint8_t rx_cmd = 0x61;
	struct spi_transfer xfer[] = {
		{
			.tx_buf = tx_buf,
			.rx_buf = NULL,
			.len = 3,
			.cs_change = 1,
			.bits_per_word = 8,
		}, {
			.tx_buf = &rx_cmd,
			.rx_buf = NULL,
			.len = 1,
			.cs_change = 0,
			.bits_per_word = 8,
		}, {
			.tx_buf = NULL,
			.rx_buf = value,
			.len = 1,
			.cs_change = 1,
			.bits_per_word = 8,
		},
	};

	spi_message_init(&msg);

	spi_message_add_tail(&xfer[0], &msg);
	spi_message_add_tail(&xfer[1], &msg);
	spi_message_add_tail(&xfer[2], &msg);

	err = spi_sync(cpld->spi, &msg);

	pr_info("%s: err = %d, value = 0x%x\n", __func__, err, *value);
	return err;
}

int gpio_spi_read(int length, unsigned char *buffer)
{
#if 1
	struct spi_device *spi = g_cpld->spi;
	struct spi_message msg;
	
	int err = -1;
	uint8_t tx_buf[3];
	uint8_t rx_cmd;
	struct spi_transfer xfer[3];
	unsigned char *buf;
	int len = 0;

	if (!buffer) {
		pr_info("%s. Pointer is NULL.\n", __func__);
		return err;
	}
	
	len = length;
	
	if (!buffer && len > 4){

		pr_err("%s.Failed. len:%d.\n", __func__, len);
		
		return err;
	}

	buf = buffer;

	memcpy(tx_buf, buf, sizeof(tx_buf));

	rx_cmd = *(buf+3);

	buf += 4;
	len = len - 4;

	xfer[0].tx_buf = tx_buf;
	xfer[0].rx_buf = NULL;
	xfer[0].len = 3;
	xfer[0].cs_change = 1;
	xfer[0].bits_per_word = 8;

	xfer[1].tx_buf = &rx_cmd;
	xfer[1].rx_buf = NULL;
	xfer[1].len = 1;
	xfer[1].cs_change = 0;
	xfer[1].bits_per_word = 8;

	
	xfer[2].tx_buf = NULL;
	xfer[2].rx_buf = buf;
	xfer[2].len = len;
	xfer[2].cs_change = 1;
	xfer[2].bits_per_word = 8;

	spi_message_init(&msg);

	spi_message_add_tail(&xfer[0], &msg);
	spi_message_add_tail(&xfer[1], &msg);
	spi_message_add_tail(&xfer[2], &msg);

	err = spi_sync(spi, &msg);

	pr_info("%s: err = %d\n", __func__, err);

	if (err)
	{
		err = -1;
	}

	return err;

#else
	struct spi_transfer *x = &lcd->xfer;
	int err = 0;

	if (len > CPLD_BUFSIZ)
		return -EINVAL;

	g_cpld->buf[0] = (data >> 8) & 0xff;
	g_cpld->buf[1] = data & 0xff;
	x->len = 2;
	err = spi_sync(g_cpld->spi, &g_cpld->msg);

	return 0;
#endif
}
EXPORT_SYMBOL(gpio_spi_read);


#if ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_Z4U))
#define DEBUG 0
#define GPIOHACK 0
#define GPIO_DIRECTION_OPTIMIZE 0
#endif

#if 1
int cpld_spi_write(int length, unsigned char *buffer)
{
	unsigned short data = 0;
	unsigned char *pBuf;
	unsigned short byte_high = 0;
	unsigned short byte_low = 0;
	int i = 0;
	int len = 0;
#if ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_Z4U))
	unsigned mask[]={EBI2_ADDR_0, EBI2_ADDR_1, EBI2_ADDR_2, EBI2_DATA_0, EBI2_DATA_1, EBI2_DATA_2, EBI2_DATA_3, EBI2_DATA_4, EBI2_DATA_5, EBI2_DATA_6, EBI2_DATA_7, EBI2_OE, EBI2_WE, 0};
	int     value[]={  1, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0};
#endif

	len = length;
	
	
	
	if (!buffer || len <= 0) {
		pr_info("The parameter is wrong!\n");
		return (-1);
	}

	pBuf = buffer;
	
	for (i = 0; i < len; i++) {
		#if 1
		
		byte_low =  (*pBuf) & 0x00ff;
		byte_high = ((*pBuf) & 0xff00);
		#else
		byte_low =  (*pBuf) & 0x003f;
		byte_high = ((*pBuf) & 0x00c0) << 4;
		#endif
		data = byte_high | byte_low;
#if 0
		pr_info("i:%d, len-1:%d\n", i, len-1);
		pr_info("*pBuf:0x%x, byte_high:0x%x, byte_low:0x%x\n", *pBuf, byte_high, byte_low);			
		pr_info("*pBuf:0x%x, data:0x%x, tx_buff_num:%d\n", *pBuf, data, g_cpld_manager.current_tx_buf_num);
#endif

#if ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_Z4U))
		
		
		mask[0]= EBI2_ADDR_0;	mask[1]= EBI2_ADDR_1;	mask[2]= EBI2_ADDR_2;
		
		mask[3]= EBI2_DATA_0;	mask[4]= EBI2_DATA_1;	mask[5]= EBI2_DATA_2;	mask[6]= EBI2_DATA_3;
		mask[7]= EBI2_DATA_4;	mask[8]= EBI2_DATA_5;	mask[9]= EBI2_DATA_6;	mask[10]= EBI2_DATA_7;
		
		mask[11]= EBI2_OE;
		
		mask[12]= EBI2_WE;
		
		mask[13]= 0;
		
		*(value +0) = 1;
		*(value +1) = 0;
		
		if (i == (len -1))	*(value + 2) = 1;
		else *(value + 2) = 0;
		
		*(value +3) = (*pBuf) & 0x0001;
		*(value +4) = ((*pBuf) & 0x0002) >> 1;
		*(value +5) = ((*pBuf) & 0x0004) >> 2;
		*(value +6) = ((*pBuf) & 0x0008) >> 3;
		*(value +7) = ((*pBuf) & 0x0010) >> 4;
		*(value +8) = ((*pBuf) & 0x0020) >> 5;
		*(value +9) = ((*pBuf) & 0x0040) >> 6;
		*(value +10) = ((*pBuf) & 0x0080) >> 7;
		
		*(value +11) = 1;
		
		*(value +12) = 0;
#if 0
#endif
		gpio_set_value_array(mask,value);

#if GPIOHACK
		
		mask[0]= EBI2_ADDR_0;	mask[1]= EBI2_ADDR_1;	mask[2]= EBI2_ADDR_2;
		
		mask[3]= EBI2_DATA_0;	mask[4]= EBI2_DATA_1;	mask[5]= EBI2_DATA_2;	mask[6]= EBI2_DATA_3;
		mask[7]= EBI2_DATA_4;	mask[8]= EBI2_DATA_5;	mask[9]= EBI2_DATA_6;	mask[10]= EBI2_DATA_7;
		
		mask[11]= EBI2_OE;
		
		mask[12]= EBI2_WE;
		
		mask[13]= 0;


		*(value +0) = 1;
		*(value +1) = 0;
		
		if (i == (len -1))	*(value + 2) = 1;
		else *(value + 2) = 0;
		
		*(value +3) = (*pBuf) & 0x0001;
		*(value +4) = ((*pBuf) & 0x0002) >> 1;
		*(value +5) = ((*pBuf) & 0x0004) >> 2;
		*(value +6) = ((*pBuf) & 0x0008) >> 3;
		*(value +7) = ((*pBuf) & 0x0010) >> 4;
		*(value +8) = ((*pBuf) & 0x0020) >> 5;
		*(value +9) = ((*pBuf) & 0x0040) >> 6;
		*(value +10) = ((*pBuf) & 0x0080) >> 7;
		
		*(value +11) = 1;
		
		*(value +12) = 1;
#if 0
#endif
		gpio_set_value_array(mask,value);
#endif
		pBuf++;

#else
		if (i == (len - 1)) {
			if (tx_buffer_1 == g_cpld_manager.current_tx_buf_num)
			{
				g_cpld_manager.current_tx_buf_num = tx_buffer_2;
				g_cpld_manager.p_cpld_reg->tx_buffer2_cs_high = data;
			}
			else if (tx_buffer_2 == g_cpld_manager.current_tx_buf_num)
			{
				g_cpld_manager.current_tx_buf_num = tx_buffer_1;
				g_cpld_manager.p_cpld_reg->tx_buffer1_cs_high = data;
			}
			else
			{
				g_cpld_manager.current_tx_buf_num = tx_buffer_1;
				g_cpld_manager.p_cpld_reg->tx_buffer1_cs_high = data;
			}
			udelay(5);

			break;
		}

		
		if (tx_buffer_1 == g_cpld_manager.current_tx_buf_num)
		{
			g_cpld_manager.current_tx_buf_num = tx_buffer_2;
			g_cpld_manager.p_cpld_reg->tx_buffer2_cs_low = data;
		}
		else if (tx_buffer_2 == g_cpld_manager.current_tx_buf_num)
		{
			g_cpld_manager.current_tx_buf_num = tx_buffer_1;
			g_cpld_manager.p_cpld_reg->tx_buffer1_cs_low = data;
		}
		else
		{
			g_cpld_manager.current_tx_buf_num = tx_buffer_1;
			g_cpld_manager.p_cpld_reg->tx_buffer1_cs_low = data;
		}

		udelay(1);
		pBuf++;
#endif
	}

	
	
	return len;
}

#else
int cpld_ebi2spi_write(unsigned char *buf, int len)
{
	
	int i;

	switch_interface(INTF_SPI);

	for (i = 0; i < len; i++) {
		;	
	}

	switch_interface(INTF_UART);

	return 0;
}
#endif

EXPORT_SYMBOL(cpld_spi_write);

#if 1
int cpld_spi_read(int length, unsigned char *buffer)
{
	int i = 0;
	unsigned short data = 0;
	unsigned short byte_high = 0;
	unsigned short byte_low = 0;
	unsigned char *pBuf; 
    int count = 0;
	int write_count = 0;
	int len = 0;
#if ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_Z4U))
	unsigned mask[]={EBI2_ADDR_0, EBI2_ADDR_1, EBI2_ADDR_2, EBI2_DATA_0, EBI2_DATA_1, EBI2_DATA_2, EBI2_DATA_3, EBI2_DATA_4, EBI2_DATA_5, EBI2_DATA_6, EBI2_DATA_7, EBI2_OE, EBI2_WE, 0};
	int     value[]={  1, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0, 
					   0};
#if GPIO_DIRECTION_OPTIMIZE
	unsigned inputmask[]={EBI2_DATA_0, EBI2_DATA_1, EBI2_DATA_1, EBI2_DATA_3, EBI2_DATA_4, EBI2_DATA_5, EBI2_DATA_6, EBI2_DATA_7, 0};
#endif
#if DEBUG
	unsigned *masks;
	int *values;
#endif
#endif

	len = length;

	write_count = 3;

	
	 
	if (!buffer || len <= (write_count + 1)) {
		pr_error("The parameter is wrong!\n");
		return (-1);
	}

	pBuf = buffer;
	
	cpld_spi_write(write_count, pBuf);


	
	count = len - write_count;
	pBuf = pBuf + write_count;
#if 1
	
	
	byte_low =  (*pBuf);
	byte_high = ((*pBuf));

	data = byte_high | byte_low;
#else
	byte_low =  (*pBuf);
	byte_high = ((*pBuf)) << 4;

	data = byte_high | byte_low;
#endif

#if 0
	pr_info("*pBuf:0x%x, byte_high:0x%x, byte_low:0x%x\n", *pBuf, byte_high, byte_low);					
	pr_info("*pBuf:0x%x, data:0x%x\n", *pBuf, data);
#endif

#if ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_Z4U))
	
	mask[0]= EBI2_ADDR_0;	mask[1]= EBI2_ADDR_1;	mask[2]= EBI2_ADDR_2;
	
	mask[3]= EBI2_DATA_0;	mask[4]= EBI2_DATA_1;	mask[5]= EBI2_DATA_2;	mask[6]= EBI2_DATA_3;
	mask[7]= EBI2_DATA_4;	mask[8]= EBI2_DATA_5;	mask[9]= EBI2_DATA_6;	mask[10]= EBI2_DATA_7;
	
	mask[11]= EBI2_OE;
	
	mask[12]= EBI2_WE;
	
	mask[13]= 0;


	*(value +0) = 1;
	*(value +1) = 0;
	
	*(value + 2) = 0;
	
	*(value +3) = (*pBuf) & 0x0001;
	*(value +4) = ((*pBuf) & 0x0002) >> 1;
	*(value +5) = ((*pBuf) & 0x0004) >> 2;
	*(value +6) = ((*pBuf) & 0x0008) >> 3;
	*(value +7) = ((*pBuf) & 0x0010) >> 4;
	*(value +8) = ((*pBuf) & 0x0020) >> 5;
	*(value +9) = ((*pBuf) & 0x0040) >> 6;
	*(value +10) = ((*pBuf) & 0x0080) >> 7;
	
	*(value +11) = 1;
	
	*(value +12) = 0;
#if DEBUG
	printk("Simon 0\n");
	for ( masks = mask,values = value; *masks; masks++,values++ ){
		printk("{%3d} ",*masks);
	}
	printk("\n");
	for ( masks = mask,values = value; *masks; masks++,values++ ){
		printk("{%3d} ",*values);
	}
	printk("\n");
#endif
	gpio_set_value_array(mask,value);

#if GPIOHACK
	
	mask[0]= EBI2_ADDR_0;	mask[1]= EBI2_ADDR_1;	mask[2]= EBI2_ADDR_2;
	
	mask[3]= EBI2_DATA_0;	mask[4]= EBI2_DATA_1;	mask[5]= EBI2_DATA_2;	mask[6]= EBI2_DATA_3;
	mask[7]= EBI2_DATA_4;	mask[8]= EBI2_DATA_5;	mask[9]= EBI2_DATA_6;	mask[10]= EBI2_DATA_7;
	
	mask[11]= EBI2_OE;
	
	mask[12]= EBI2_WE;
	
	mask[13]= 0;


	*(value +0) = 1;
	*(value +1) = 0;
	
	*(value + 2) = 0;
	
	*(value +3) = (*pBuf) & 0x0001;
	*(value +4) = ((*pBuf) & 0x0002) >> 1;
	*(value +5) = ((*pBuf) & 0x0004) >> 2;
	*(value +6) = ((*pBuf) & 0x0008) >> 3;
	*(value +7) = ((*pBuf) & 0x0010) >> 4;
	*(value +8) = ((*pBuf) & 0x0020) >> 5;
	*(value +9) = ((*pBuf) & 0x0040) >> 6;
	*(value +10) = ((*pBuf) & 0x0080) >> 7;
	
	*(value +11) = 1;
	
	*(value +12) = 1;
	gpio_set_value_array(mask,value);
#endif
#if DEBUG
#endif
	pBuf++;
	count--;

	if (!count) {
		pr_info("failed. count==0.\n");
		return (-1);
	}
#if GPIO_DIRECTION_OPTIMIZE
	gpio_direction_input_array(inputmask);
#else
	
	gpio_direction_input(EBI2_DATA_0);

	gpio_direction_input(EBI2_DATA_1);

	gpio_direction_input(EBI2_DATA_2);

	gpio_direction_input(EBI2_DATA_3);

	gpio_direction_input(EBI2_DATA_4);

	gpio_direction_input(EBI2_DATA_5);

	gpio_direction_input(EBI2_DATA_6);

	gpio_direction_input(EBI2_DATA_7);
#endif

#if DEBUG
#endif

	for (i = 0; i < count; i++) {
			byte_high = (data & 0xff00);
			byte_low = data & 0x00ff;
			*pBuf = (unsigned char)(byte_low | byte_high);

#if DEBUG
#endif

			
			mask[0]= EBI2_ADDR_0;	mask[1]= EBI2_ADDR_1;	mask[2]= EBI2_ADDR_2;
			
			mask[3]= EBI2_DATA_0;	mask[4]= EBI2_DATA_1;	mask[5]= EBI2_DATA_2;	mask[6]= EBI2_DATA_3;
			mask[7]= EBI2_DATA_4;	mask[8]= EBI2_DATA_5;	mask[9]= EBI2_DATA_6;	mask[10]= EBI2_DATA_7;
			
			mask[11]= EBI2_OE;
			
			mask[12]= EBI2_WE;
			
			mask[13]= 0;


			*(value +0) = 1;
			*(value +1) = 0;
			
			if (i == (count -1))	*(value + 2) = 1;
			else *(value + 2) = 0;

			*(value +3) = 0;
			*(value +4) = 0;
			*(value +5) = 0;
			*(value +6) = 0;
			*(value +7) = 0;
			*(value +8) = 0;
			*(value +9) = 0;
			*(value +10) = 0;
			
			*(value +11) = 0;
			
			*(value +12) = 1;
#if DEBUG
#endif
			gpio_set_value_array(mask,value);
#if DEBUG
#endif
			
			mask[0]= EBI2_ADDR_0;	mask[1]= EBI2_ADDR_1;	mask[2]= EBI2_ADDR_2;
			
			mask[3]= EBI2_DATA_0;	mask[4]= EBI2_DATA_1;	mask[5]= EBI2_DATA_2;	mask[6]= EBI2_DATA_3;
			mask[7]= EBI2_DATA_4;	mask[8]= EBI2_DATA_5;	mask[9]= EBI2_DATA_6;	mask[10]= EBI2_DATA_7;
			
			mask[11]= EBI2_OE;
			
			mask[12]= EBI2_WE;
			
			mask[13]= 0;

			gpio_get_value_array(mask,value);
#if DEBUG
#endif
			(*pBuf)&= 0x0000;
			(*pBuf)|= *(value +3);
			(*pBuf)|= (*(value +4) << 1);
			(*pBuf)|= (*(value +5) << 2);
			(*pBuf)|= (*(value +6) << 3);
			(*pBuf)|= (*(value +7) << 4);
			(*pBuf)|= (*(value +8) << 5);
			(*pBuf)|= (*(value +9) << 6);
			(*pBuf)|= (*(value +10) << 7);
#if DEBUG
#endif
#if 1
			
			mask[0]= EBI2_ADDR_0;	mask[1]= EBI2_ADDR_1;	mask[2]= EBI2_ADDR_2;
			
			mask[3]= EBI2_DATA_0;	mask[4]= EBI2_DATA_1;	mask[5]= EBI2_DATA_2;	mask[6]= EBI2_DATA_3;
			mask[7]= EBI2_DATA_4;	mask[8]= EBI2_DATA_5;	mask[9]= EBI2_DATA_6;	mask[10]= EBI2_DATA_7;
			
			mask[11]= EBI2_OE;
			
			mask[12]= EBI2_WE;
			
			mask[13]= 0;


			*(value +0) = 1;
			*(value +1) = 0;
			
			if (i == (count -1))	*(value + 2) = 1;
			else *(value + 2) = 0;

			*(value +3) = 0;
			*(value +4) = 0;
			*(value +5) = 0;
			*(value +6) = 0;
			*(value +7) = 0;
			*(value +8) = 0;
			*(value +9) = 0;
			*(value +10) = 0;
			
			*(value +11) = 1;
			
			*(value +12) = 1;

#if DEBUG
#endif

			gpio_set_value_array(mask,value);
#endif


			pBuf++;
	}

	gpio_direction_output(EBI2_DATA_0, 0);

	gpio_direction_output(EBI2_DATA_1, 0);

	gpio_direction_output(EBI2_DATA_2, 0);

	gpio_direction_output(EBI2_DATA_3, 0);

	gpio_direction_output(EBI2_DATA_4, 0);

	gpio_direction_output(EBI2_DATA_5, 0);

	gpio_direction_output(EBI2_DATA_6, 0);

	gpio_direction_output(EBI2_DATA_7, 0);
#else

	if (tx_buffer_1 == g_cpld_manager.current_tx_buf_num)
	{
		g_cpld_manager.current_tx_buf_num = tx_buffer_2;
		g_cpld_manager.p_cpld_reg->tx_buffer2_cs_low = data;
	}
	else if (tx_buffer_2 == g_cpld_manager.current_tx_buf_num)
	{
		g_cpld_manager.current_tx_buf_num = tx_buffer_1;
		g_cpld_manager.p_cpld_reg->tx_buffer1_cs_low = data;
	}
	else
	{
		g_cpld_manager.current_tx_buf_num = tx_buffer_1;
		g_cpld_manager.p_cpld_reg->tx_buffer1_cs_low = data;
	}
	udelay(5);

	
	
	pBuf++;
	count--;
    
	if (!count) {
		pr_info("failed. count==0.\n");
		return (-1);
	}

	

	
	
	for (i = 0; i < count; i++) {
			
		
			if (i == (count - 1)) {
				
				data = g_cpld_manager.p_cpld_reg->rx_buffer1_cs_high;
				#if 1
				byte_high = (data & 0xff00);
				byte_low = data & 0x00ff;
				#else
				byte_high = (data & 0x0c00) >> 4;
				byte_low = data & 0x003f;
				#endif
				*pBuf = (unsigned char)(byte_low | byte_high);

#if 0
				pr_info("i:%d, count-1:%d\n", i, count-1);
				pr_info("*pBuf:0x%x, byte_high:0x%x, byte_low:0x%x\n", *pBuf, byte_high, byte_low);					
				printk("*pBuf:0x%x, data:0x%x\n", *pBuf, data);
#endif
				udelay(5);
				break;
			}

			
			data = g_cpld_manager.p_cpld_reg->rx_buffer1_cs_low;
			#if 1
			byte_high = (data & 0xff00);
			byte_low = data & 0x00ff;
			#else
			byte_high = (data & 0x0c00) >> 4;
			byte_low = data & 0x003f;
			#endif

			*pBuf = (unsigned char)(byte_low | byte_high);

#if 0
			pr_info("i:%d, count-1:%d\n", i, count-1);
			pr_info("*pBuf:0x%x, byte_high:0x%x, byte_low:0x%x\n", *pBuf, byte_high, byte_low);					
			printk("*pBuf:0x%x, data:0x%x\n", *pBuf, data);
#endif
			udelay(5);
			pBuf++;
		}
#endif

		
		
		return (0);
}
#else
int cpld_ebi2spi_read(unsigned char *buf, int len)
{
	
	int i;

	switch_interface(INTF_SPI);

	for (i = 0; i < len; i++) {
		;	
	}

	switch_interface(INTF_UART);

	return 0;
}
#endif

EXPORT_SYMBOL(cpld_spi_read);

#if 0
static void printb(char *buff, size_t size, const char *title)
{
	int i;

	if (title)
		printk(KERN_INFO "%s:\n", title);
	for (i = 0; i < size; i++) {
		if (i % 16 == 0) {
			if (i)
				printk(KERN_INFO "\n");
			printk(KERN_INFO "  %04x:", i);
		}
		if (i % 2 == 0)
			printk(KERN_INFO " ");
		printk(KERN_INFO "%02x", buff[i]);
	}
	printk(KERN_INFO "\n");
}
#endif

#if 0
static int sprintb(char *dest, char *src, size_t size, const char *title)
{
	int i, count = 0;

	if (title)
		count += sprintf(dest, "%s:\n", title);
	for (i = 0; i < size; i++) {
		if (i % 16 == 0) {
			if (i)
				count += sprintf(dest + count, "\n");
			count += sprintf(dest + count, "  %08x:", i);
		}
		if (i % 2 == 0)
			count += sprintf(dest + count, " ");
		count += sprintf(dest + count, "%02x", src[i]);
	}
	count += sprintf(dest + count, "\n");

	return  count;
}
#endif

static int parse_arg(const char *buf, int buf_len,
			void *argv, int max_size, int arg_size)
{
	char string[buf_len + 1];
	unsigned long result = 0;
	char *str = string;
	char *p;
	int n = 0;

	if (buf_len < 0)
		return -1;

	memcpy(string, buf, buf_len);
	string[buf_len] = '\0';

	while ((p = strsep(&str, " :"))) {
		if (!strlen(p))
			continue;
		if (!strict_strtoul(p, 16, &result)) {
			if (arg_size == 1)
				((uint8_t *)argv)[n] = (uint8_t)result;
			else if (arg_size == 2)
				((uint16_t *)argv)[n] = (uint16_t)result;
			else if (arg_size == 4)
				((uint32_t *)argv)[n] = (uint32_t)result;
		} else {
			break;
		}
		if (++n == max_size / arg_size)
			break;
	}

	return n;
}

static ssize_t gpio_spi_store(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	uint8_t *argv = (uint8_t *)cpld->sysfs_buf;
	int n;

	n = parse_arg(buf + 2, count - 2, argv, CPLD_BUFSIZ, 1);

	pr_info("%s: n = %d\n", __func__, n);

	if (n > 0) {
		cpld->reg_addr = argv[0];
		if (buf[0] == 'w') {
			 gpio_spi_write(n, argv);
		} else {
			cpld->sysfs_cnt = (n == 1) ? 1 : argv[1];
		}
	}

	return count;
}

static ssize_t gpio_spi_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	ssize_t count = 0;
	int err;
	uint8_t data;

	

	err = cpld_gpiospi_single_read(cpld, cpld->reg_addr, &data);
	pr_info("%s: err = %d, rd_data = 0x%x\n", __func__, err, data);

	
		
	

	return count;
}

static ssize_t ebi2_spi_store(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	uint8_t *argv = (uint8_t *)cpld->sysfs_buf;
	int n;

	n = parse_arg(buf + 2, count - 2, argv, CPLD_BUFSIZ, 1);
	
	pr_info("0201: %s: n = %d\n", __func__, n);

	if (n > 0) {
		cpld->reg_addr = argv[0];
		if (buf[0] == 'w') {
			spi_set_route(EBI2_SPI);
			cpld_spi_write(n, argv);
		} 
		else if (buf[0] == 'r') {
			spi_set_route(EBI2_SPI);
			cpld_spi_read(argv[n-1], argv);
		} 
		else {
			cpld->sysfs_cnt = (n == 1) ? 1 : argv[1];
		}
	}

	return count;
}

static ssize_t ebi2_spi_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
#if 0
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	uint8_t *argv = (uint8_t *)cpld->sysfs_buf;
	int n;
	int count;

	count = strlen(buf);

	n = parse_arg(buf + 2, count - 2, argv, CPLD_BUFSIZ, 1);

	pr_info("%s: n = %d, count:%d\n", __func__, n, count);

	if (n > 0) {
		cpld->reg_addr = argv[0];
		if (buf[0] == 'r') {
			cpld_spi_read(argv[n-1], argv);
		} 
		else {
			cpld->sysfs_cnt = (n == 1) ? 1 : argv[1];
		}
	}
#endif

	return 0;
}
	
static ssize_t cpld_clock_store(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);


	clock_setting(cpld, 1);
	
	pr_info("%s\n", __func__);
	
	return count;
}

static ssize_t cpld_clock_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	

	pr_info("%s\n", __func__);
	return 0;
}

static ssize_t switch_store(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	

	pr_info("%s. switch to ebi2 or uart2dm:%s.\n", __func__, &buf[0]);
	
	if (buf[0] == 'e') {
		pr_info("EBI2_SPI\n");
		spi_set_route(EBI2_SPI);
	}
	else if(buf[0] == 'u') {
		pr_info("GPIO_SPI\n");
		spi_set_route(GPIO_SPI);
	}
		
	return count;
}

static ssize_t switch_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	

	pr_info("%s\n", __func__);
	
	return 0;
}


static ssize_t config_store(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	uint32_t *argv = (uint32_t *)cpld->sysfs_buf;
	int n, i;

	pr_info("%s\n", __func__);

	n = parse_arg(buf + 2, count - 2, argv, CPLD_BUFSIZ, 4);

	if (n > 0) {
		cpld->cfg_offs = argv[0];
		if (buf[0] == 'w') {
			cpld->sysfs_cnt = (n == 1) ? 0 : (n - 1);
			for (i = 0; i < cpld->sysfs_cnt; i++)
				writel(argv[1 + i], cpld->cfg_base +
						cpld->cfg_offs + i * 4);
		} else {
			cpld->sysfs_cnt = (n == 1) ? 1 : argv[1];
		}
	}

	return count;
}

static ssize_t config_show(struct kobject *kobj, struct kobj_attribute *attr,
						char *buf)
{
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	
	uint32_t value;
	ssize_t count = 0;
	int i;

	pr_info("%s\n", __func__);

	if (cpld->sysfs_cnt > 0) {
		for (i = 0; i < cpld->sysfs_cnt; i++) {
			value = readl(cpld->cfg_base + cpld->cfg_offs + i * 4);
#if 0
			if (i % 4 == 0) {
				if (i)
					count += sprintf(buf + count, "\n");
				count += sprintf(buf + count, "  %08x:",
						cpld->cfg_start +
						cpld->cfg_offs + i * 4);
			}
			count += sprintf(buf + count, " %08x", value);
#endif
			count += sprintf(buf + count, "  %08x: %08x\n",
					cpld->cfg_start +
					cpld->cfg_offs + i * 4, value);
		}
		

		cpld->sysfs_cnt = 0;
	}

	return count;
}

static ssize_t gpio_store(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	uint32_t *argv = (uint32_t *)cpld->sysfs_buf;
	int n, i;

	pr_info("%s\n", __func__);

	n = parse_arg(buf + 2, count - 2, argv, CPLD_BUFSIZ, 4);

	if (n > 0) {
		cpld->gpio_offs = argv[0];
		if (buf[0] == 'w') {
			cpld->sysfs_cnt = (n == 1) ? 0 : (n - 1);
			for (i = 0; i < cpld->sysfs_cnt; i++)
				writel(argv[1 + i], cpld->gpio_base +
						cpld->gpio_offs + i * 4);
		} else {
			cpld->sysfs_cnt = (n == 1) ? 1 : argv[1];
		}
	}

	return count;
}

static ssize_t clock_store(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	uint32_t *argv = (uint32_t *)cpld->sysfs_buf;
	int n, i;

	pr_info("%s\n", __func__);

	n = parse_arg(buf + 2, count - 2, argv, CPLD_BUFSIZ, 4);

	if (n > 0) {
		cpld->clk_offs = argv[0];
		if (buf[0] == 'w') {
			cpld->sysfs_cnt = (n == 1) ? 0 : (n - 1);
			for (i = 0; i < cpld->sysfs_cnt; i++)
				writel(argv[1 + i], cpld->clk_base +
						cpld->clk_offs + i * 4);
		} else {
			cpld->sysfs_cnt = (n == 1) ? 1 : argv[1];
		}
	}

	return count;
}

static ssize_t clock_show(struct kobject *kobj, struct kobj_attribute *attr,
						char *buf)
{
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	void __iomem *base = cpld->clk_base + cpld->clk_offs;
	uint32_t value;
	ssize_t count = 0;
	int i;

	pr_info("%s\n", __func__);

	if (cpld->sysfs_cnt > 0) {
		for (i = 0; i < cpld->sysfs_cnt; i++) {
			value = readl(base + i * 4);

			if (i % 8 == 0) {
				if (i)
					count += sprintf(buf + count, "\n");
				count += sprintf(buf + count, "  %08x:",
						(unsigned int)base + i * 4);
			}
			count += sprintf(buf + count, " %08x", value);
		}
		count += sprintf(buf + count, "\n");

		cpld->sysfs_cnt = 0;
	}

	return count;
}

static ssize_t
gpio_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	void __iomem *base = cpld->gpio_base + cpld->gpio_offs;
	uint32_t value;
	ssize_t count = 0;
	int i;

	pr_info("%s\n", __func__);

	if (cpld->sysfs_cnt > 0) {
		for (i = 0; i < cpld->sysfs_cnt; i++) {
			value = readl(base + i * 4);

			if (i % 8 == 0) {
				if (i)
					count += sprintf(buf + count, "\n");
				count += sprintf(buf + count, "  %08x:",
						(unsigned int)base + i * 4);
			}
			count += sprintf(buf + count, " %08x", value);
		}
		count += sprintf(buf + count, "\n");

		cpld->sysfs_cnt = 0;
	}

	return count;
}

static ssize_t register_store(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
#if 0
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	uint8_t *argv = (uint8_t *)cpld->sysfs_buf;
	int n, i;


	pr_info("0113: %s.\n", __func__);

	n = parse_arg(buf + 2, count - 2, argv, CPLD_BUFSIZ, 1);

	if (n > 0) {
		cpld->reg_addr = argv[0];
		if (buf[0] == 'w') {
			cpld->sysfs_cnt = (n == 1) ? 0 : (n - 1);
			for (i = 0; i < cpld->sysfs_cnt; i++)
				writeb(argv[1 + i],
				       cpld->reg_base + cpld->reg_addr + i);
		} else {
			cpld->sysfs_cnt = (n == 1) ? 1 : argv[1];
		}
	}

	return count;
#else
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	uint32_t *argv = (uint32_t *)cpld->sysfs_buf;
	int n, i;

	pr_info("%s\n", __func__);

	n = parse_arg(buf + 2, count - 2, argv, CPLD_BUFSIZ, 4);

	if (n > 0) {
		cpld->reg_addr = argv[0];
		
		if (buf[0] == 'w') {

			spi_set_route(EBI2_SPI);
			
			cpld->sysfs_cnt = (n == 1) ? 0 : (n - 1);
			for (i = 0; i < cpld->sysfs_cnt; i++){
				
				writel(argv[1 + i], cpld->reg_base +
						cpld->reg_addr + i * 4);
			}
		} else {
			cpld->sysfs_cnt = (n == 1) ? 1 : argv[1];
		}
	}

	return count;
#endif
}

static ssize_t
register_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
#if 0
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	uint8_t *rd_buf = (uint8_t *)cpld->sysfs_buf;
	ssize_t count = 0;
	int i;
	
	pr_info("0113: %s.\n", __func__);

	if (cpld->sysfs_cnt > 0) {
		memset(rd_buf, 0, cpld->sysfs_cnt);
		for (i = 0; i < cpld->sysfs_cnt; i++)
			rd_buf[i] = readb(cpld->reg_base + cpld->reg_addr + i);
		count += sprintb(buf, rd_buf, cpld->sysfs_cnt, NULL);
	}
	cpld->sysfs_cnt = 0;

	return count;
#else
	struct cpld_driver *cpld = container_of(kobj, struct cpld_driver, kobj);
	uint32_t value;
	ssize_t count = 0;
	int i;

	pr_info("0113-1:%s\n", __func__);

	spi_set_route(EBI2_SPI);
	
	if (cpld->sysfs_cnt > 0) {
		for (i = 0; i < cpld->sysfs_cnt; i++) {
			
			value = readl(cpld->reg_base + cpld->reg_addr + i * 4);

#if 0
			if (i % 4 == 0) {
				if (i)
					count += sprintf(buf + count, "\n");
				count += sprintf(buf + count, "  %08x:",
						cpld->cfg_start +
						cpld->cfg_offs + i * 4);
			}
			count += sprintf(buf + count, " %08x", value);
#endif
			count += sprintf(buf + count, "%08x\n", value);
		}
		

		cpld->sysfs_cnt = 0;
	}

	return count;
#endif
}

static struct kobj_attribute cpld_attrs[] = {
	__ATTR(gpio_spi, (S_IWUSR | S_IRUGO), gpio_spi_show, gpio_spi_store),
	__ATTR(ebi2_spi, (S_IWUSR | S_IRUGO), ebi2_spi_show, ebi2_spi_store),
	__ATTR(config, (S_IWUSR | S_IRUGO), config_show, config_store),
	__ATTR(gpio, (S_IWUSR | S_IRUGO), gpio_show, gpio_store),
	__ATTR(register, (S_IWUSR | S_IRUGO), register_show, register_store),
	__ATTR(clock, (S_IWUSR | S_IRUGO), clock_show, clock_store),
	__ATTR(cpld_clock, (S_IWUSR | S_IRUGO), cpld_clock_show,cpld_clock_store),
	__ATTR(switch, (S_IWUSR | S_IRUGO), switch_show,switch_store),
	__ATTR_NULL
};

static struct kobj_type cpld_ktype = {
	.sysfs_ops = &kobj_sysfs_ops,
};

static int cpld_sysfs_create(struct cpld_driver *cpld)
{
	struct kobj_attribute *p;
	int err;

	err = kobject_init_and_add(&cpld->kobj, &cpld_ktype, NULL, "cpld");
	if (err == 0) {
		for (p = cpld_attrs; p->attr.name; p++)
			err = sysfs_create_file(&cpld->kobj, &p->attr);
	}

	return err;
}

static void cpld_sysfs_remove(struct cpld_driver *cpld)
{
	struct kobj_attribute *p;

	if (cpld->kobj.state_in_sysfs) {
		for (p = cpld_attrs; p->attr.name; p++)
			sysfs_remove_file(&cpld->kobj, &p->attr);
		kobject_del(&cpld->kobj);
	}
}

static int __devinit cpld_gpiospi_probe(struct spi_device *spi)
{
	struct cpld_driver *cpld = g_cpld;
	int err;

	pr_info("%s entry\n", __func__);

	spi->bits_per_word = 8;
	err = spi_setup(spi);
	if (err) {
		pr_error("spi_setup error\n");
		return err;
	}

	cpld->spi = spi;
	cpld->xfer.bits_per_word = 0;
	dev_set_drvdata(&spi->dev, cpld);

	

	return 0;
}

static int __devexit cpld_gpiospi_remove(struct spi_device *spi)
{
	

	pr_info("%s\n", __func__);

	return 0;
}

static struct spi_driver cpld_gpiospi_driver = {
	.probe	= cpld_gpiospi_probe,
	.remove	= __devexit_p(cpld_gpiospi_remove),
	.driver	= {
		.name	= "gpio-cpld",
		.owner	= THIS_MODULE,
	},
};

static int cpld_gpio_init(struct cpld_driver *cpld)
{
#if !((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_Z4U))
	unsigned int value;
#endif
	
#if 1
	pr_info("%s\n", __func__);
	
	
	
#else
	if (cpld->pdata->init_gpio){
		cpld->pdata->init_gpio();
	}
#endif

#if !((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_Z4U))
	value = readl(cpld->clk_base + 0xb8);
	pr_info("0131-1: cpld_gpio_init. SDC4_MD: 0x%08x\n", value);

	value = readl(cpld->clk_base + 0xbc);
	pr_info("cpld_gpio_init. SDC4_NS: 0x%08x\n", value);

	writel(0, cpld->gpio_base + TMUX_EBI2_OFFSET);

	value = readl(cpld->cfg_base + EBI2_XMEM_CS0_CFG1_OFFSET);
	value |= (1 << 31);
	writel(value, cpld->cfg_base + EBI2_XMEM_CS0_CFG1_OFFSET);

	writel(0x02, cpld->cfg_base);

	writel(0x031F3200, cpld->cfg_base + 0x10008);
#endif

#if 0
	if(cpld->pdata->cpld_power_pwd){
		err = gpio_request(cpld->pdata->cpld_power_pwd, "cpld_power");
		if (err) {
			pr_error("failed to request gpio cpld_power\n");
			return -EINVAL;
		}
	}

	if(cpld->pdata->intf_select){
		err = gpio_request(cpld->pdata->intf_select, "interface_select");
		if (err) {
			pr_error("failed to request gpio interface_select\n");
			gpio_free(cpld->pdata->cpld_power_pwd);
			return -EINVAL;
		}
	}


	if(cpld->pdata->cpld_power_pwd)
		gpio_direction_output(cpld->pdata->cpld_power_pwd, 1);	
	if(cpld->pdata->intf_select)
		gpio_direction_output(cpld->pdata->intf_select, 1);
#endif

#if ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_Z4U))
	gpio_request(EBI2_ADDR_0, "EBI2_ADDR_0");
	gpio_direction_output(EBI2_ADDR_0, 0);
	gpio_free(EBI2_ADDR_0);

	gpio_request(EBI2_ADDR_1, "EBI2_ADDR_1");
	gpio_direction_output(EBI2_ADDR_1, 0);
	gpio_free(EBI2_ADDR_1);

	gpio_request(EBI2_ADDR_2, "EBI2_ADDR_2");
	gpio_direction_output(EBI2_ADDR_2, 0);
	gpio_free(EBI2_ADDR_2);

	gpio_request(EBI2_DATA_0, "EBI2_DATA_0");
	gpio_direction_output(EBI2_DATA_0, 0);

	gpio_request(EBI2_DATA_1, "EBI2_DATA_1");
	gpio_direction_output(EBI2_DATA_1, 0);

	gpio_request(EBI2_DATA_2, "EBI2_DATA_2");
	gpio_direction_output(EBI2_DATA_2, 0);

	gpio_request(EBI2_DATA_3, "EBI2_DATA_3");
	gpio_direction_output(EBI2_DATA_3, 0);

	gpio_request(EBI2_DATA_4, "EBI2_DATA_4");
	gpio_direction_output(EBI2_DATA_4, 0);

	gpio_request(EBI2_DATA_5, "EBI2_DATA_5");
	gpio_direction_output(EBI2_DATA_5, 0);

	gpio_request(EBI2_DATA_6, "EBI2_DATA_6");
	gpio_direction_output(EBI2_DATA_6, 0);

	gpio_request(EBI2_DATA_7, "EBI2_DATA_7");
	gpio_direction_output(EBI2_DATA_7, 0);

	gpio_request(EBI2_OE, "EBI2_OE");
	gpio_direction_output(EBI2_OE, 1);
	gpio_free(EBI2_OE);

	gpio_request(EBI2_WE, "EBI2_WE");
	gpio_direction_output(EBI2_WE, 1);
	gpio_free(EBI2_WE);
#else
	g_cpld_manager.p_cpld_reg = cpld->reg_base;
	g_cpld_manager.current_tx_buf_num = tx_buffer_1;
#endif
	return 0;
}

int cpld_open_init(void)
{
	pr_info("%s\n", __func__);
	if (g_cpld->pdata->init_cpld_clk){
		pr_info("%s: Enable SDMC4 CLK\n", __func__);
		g_cpld->pdata->init_cpld_clk(1);
	}

	clock_setting(g_cpld, 1);

	cpld_gpio_init(g_cpld);

	if (g_cpld->pdata->power_func){
		pr_info("%s: Enable CPLD power\n", __func__);
		g_cpld->pdata->power_func(1);
	}

	return 0;
}

int cpld_release(void)
{
	pr_info("%s\n", __func__);
	if (g_cpld->pdata->init_cpld_clk){
		pr_info("%s: Disable SDMC4 CLK\n", __func__);
		g_cpld->pdata->init_cpld_clk(0);
	}

	clock_setting(g_cpld, 0);
	
	if (g_cpld->pdata->power_func){
		pr_info("%s: Disable CPLD power\n", __func__);
		g_cpld->pdata->power_func(0);
	}

	return 0;
}


static int cpld_probe(struct platform_device *pdev)
{
	struct cpld_platform_data *pdata = pdev->dev.platform_data;
	struct cpld_driver *cpld;
	int err;
#ifdef CONFIG_MACH_DUMMY
	struct resource *sdc4_mem;
#elif ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY))
	struct resource *cfg_mem, *reg_mem, *gpio_mem, *clk_mem, *sdc4_mem;
	int cfg_value;
#endif

	pr_info("%s\n", __func__);

#ifdef CONFIG_MACH_DUMMY
	sdc4_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!sdc4_mem) {
		pr_error("no sdc4 mem resource!\n");
		return -ENODEV;
	}
#elif ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY))
	cfg_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!cfg_mem) {
		pr_error("no config mem resource!\n");
		return -ENODEV;
	}

	reg_mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!reg_mem) {
		pr_error("no reg mem resource!\n");
		return -ENODEV;
	}

	gpio_mem = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!gpio_mem) {
		pr_error("no gpio mem resource!\n");
		return -ENODEV;
	}

	clk_mem = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (!clk_mem) {
		pr_error("no clock mem resource!\n");
		return -ENODEV;
	}

	sdc4_mem = platform_get_resource(pdev, IORESOURCE_MEM, 4);
	if (!sdc4_mem) {
		pr_error("no sdc4 mem resource!\n");
		return -ENODEV;
	}
#endif

	cpld = kzalloc(sizeof(struct cpld_driver), GFP_KERNEL);
	if (!cpld){
		pr_error("create cpld memory fail\n");
		return -ENOMEM;
	}

	g_cpld = cpld;
	cpld->pdata = pdata;

#ifdef CONFIG_MACH_DUMMY
	cpld->sdc4_base = ioremap(sdc4_mem->start, resource_size(sdc4_mem));
	if (!cpld->sdc4_base) {
		pr_error("no sdc4 mem resource\n");
		err = -ENOMEM;
		goto err_sdc4_ioremap;
	}
#elif ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY))
	cpld->cfg_base = ioremap(cfg_mem->start, resource_size(cfg_mem));
	if (!cpld->cfg_base) {
		pr_error("no config mem resource\n");
		err = -ENOMEM;
		goto err_config_ioremap;
	}

	cpld->reg_base = ioremap(reg_mem->start, resource_size(reg_mem));
	if (!cpld->reg_base) {
		pr_error("no reg mem resource\n");
		err = -ENOMEM;
		goto err_register_ioremap;
	}

	cpld->gpio_base = ioremap(gpio_mem->start, resource_size(gpio_mem));
	if (!cpld->gpio_base) {
		pr_error("no gpio mem resource\n");
		err = -ENOMEM;
		goto err_gpio_ioremap;
	}

	cpld->clk_base = ioremap(clk_mem->start, resource_size(clk_mem));
	if (!cpld->clk_base) {
		pr_error("no clock mem resource\n");
		err = -ENOMEM;
		goto err_clk_ioremap;
	}

	cpld->sdc4_base = ioremap(sdc4_mem->start, resource_size(sdc4_mem));
	if (!cpld->sdc4_base) {
		pr_error("no sdc4 mem resource\n");
		err = -ENOMEM;
		goto err_sdc4_ioremap;
	}

	cfg_value = readl(cpld->cfg_base);
	pr_info("cfg_value = %08x\n", cfg_value);
	cfg_value = readl(cpld->cfg_base + 0x0008);
	pr_info("cfg_value = %08x\n", cfg_value);
#endif

	err = spi_register_driver(&cpld_gpiospi_driver);
	if (err)
		goto err_1;

#if 0
	err = platform_driver_probe(ebi2_driver, ebi2_probe);
	if (err)
		goto err_2;
#endif

#if ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY))
	
	if (cpld->pdata->init_cpld_clk){
		pr_info("%s: Enable SDMC4 CLK\n", __func__);
		cpld->pdata->init_cpld_clk(1);
	}
#endif
	
	if (pdata->power_func){
		pr_info("%s: Enable CPLD power\n", __func__);
		pdata->power_func(1);
	}

	clock_setting(cpld, 1);
	
	cpld_gpio_init(cpld);

	cpld_sysfs_create(cpld);

	pr_info("CPLD ready\n");

	cpld_release();
	return 0;

#if 0
err_2:
	spi_unregister_driver(&cpld_spi_driver);
#endif
err_1:
#if ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY))
err_sdc4_ioremap:
#ifndef CONFIG_MACH_DUMMY
	iounmap(cpld->clk_base);
err_clk_ioremap:
	iounmap(cpld->gpio_base);
err_gpio_ioremap:
	iounmap(cpld->reg_base);
err_register_ioremap:
	iounmap(cpld->cfg_base);
err_config_ioremap:
#endif
#endif
	kfree(cpld);

	return err;
}

static int __exit cpld_remove(struct platform_device *pdev)
{
	struct cpld_driver *cpld = g_cpld;
	struct cpld_platform_data *pdata = pdev->dev.platform_data;
	pr_info("%s\n", __func__);

	spi_unregister_driver(&cpld_gpiospi_driver);
	cpld_sysfs_remove(cpld);
#if ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY))
#ifndef CONFIG_MACH_DUMMY
	iounmap(cpld->clk_base);
	iounmap(cpld->gpio_base);
	iounmap(cpld->reg_base);
	iounmap(cpld->cfg_base);
#endif
	iounmap(cpld->sdc4_base);
#endif
	kfree(cpld);

	if (pdata->init_cpld_clk){
		pr_info("%s: Disable SDMC4 CLK\n", __func__);
		pdata->init_cpld_clk(0);
	}

	clock_setting(cpld, 0);

	if (pdata->power_func){
		pr_info("%s: Disable CPLD power\n", __func__);
		pdata->power_func(0);
	}

	return 0;
}

static struct platform_driver cpld_platform_driver = {
	.driver.name	= "cpld",
	.driver.owner	= THIS_MODULE,
	.probe		= cpld_probe,
	.remove		= __exit_p(cpld_remove),
};

static int __init cpld_init(void)
{
	pr_info("%s\n",__func__);
	return platform_driver_register(&cpld_platform_driver);
}
module_init(cpld_init);

static void __exit cpld_exit(void)
{
	pr_info("%s\n",__func__);

	platform_driver_unregister(&cpld_platform_driver);
}
module_exit(cpld_exit);

MODULE_DESCRIPTION("Driver for CPLD");
MODULE_LICENSE("GPL");
