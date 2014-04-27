/* drivers/i2c/chips/cpld.c - CPLD driver
 *
 * Copyright (C) 2008-2012 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/module.h>
#include <linux/i2c/cpld.h>

#include "cpld_char.h"
#include <asm/htc_version.h>

#define D(x...) printk(KERN_DEBUG "[I2C CPLD] " x)
#define I(x...) printk(KERN_INFO "[I2C CPLD] " x)
#define E(x...) printk(KERN_ERR "[I2C CPLD] " x)
#define DIF(x...) \
	if (debug_flag) \
		printk(KERN_DEBUG "[I2C CPLD] " x)

#define I2C_RETRY_COUNT	10

struct cpld_info {
	struct i2c_client *i2c_client;
	uint16_t slave_address;
	struct workqueue_struct *lp_wq;
};

struct cpld_platform_data {
        int intr;
        uint16_t ALS_slave_address;
};

static int i2c_debug_flag;
static int i2c_data;
static struct cpld_info *mcpld_info;
static int cpld_slave;
static int iii;
static int gwdata;
static int grdata;
static uint32_t mcount;
static uint32_t stress_count;

static void report_near_do_work(struct work_struct *w);
static DECLARE_DELAYED_WORK(report_near_work, report_near_do_work);

static void handle_i2c_cpld_int_fn(struct work_struct *w);
static DECLARE_WORK(i2c_cpld_int_work, handle_i2c_cpld_int_fn);
static int flag_cpld_ready = 0;


int is_cpld_ready(void)
{
	return flag_cpld_ready;
}

static int I2C_TxData(uint16_t slaveAddr, uint8_t *txData, int length)
{
        uint8_t loop_i;
        
	int ret =0;
        
        struct i2c_msg msg[] = {
                {
                 .addr = slaveAddr,
                 .flags = 0,
                 .len = length,
                 .buf = txData,
                 },
        };

        for (loop_i = 0; loop_i < 10; loop_i++) {
                if ((ret=i2c_transfer(mcpld_info->i2c_client->adapter, msg, 1)) > 0)
                        break;
#ifdef CPLD_I2C_DEBUG
		printk(KERN_INFO "%s: cpld ret is %d\n", __func__, ret);
#endif
                
                if (loop_i == 0 || loop_i == I2C_RETRY_COUNT - 1)
                        E("[I2C CPLD] %s, i2c err, slaveAddr 0x%x, register 0x%x,loop i =%d , \n",
                                __func__, slaveAddr, txData[0],loop_i);

                msleep(10);
        }

        if (loop_i >= I2C_RETRY_COUNT) {
                printk(KERN_ERR "[I2C CPLD] %s retry over %d\n",
                        __func__, I2C_RETRY_COUNT);
                return -EIO;
        }

        return 0;
}

static int I2C_RxData(uint16_t slaveAddr, uint8_t *rxData, int length)
{
        uint8_t loop_i;
        
	int ret=0;
        

        struct i2c_msg msgs[] = {
                {
                 .addr = slaveAddr,
                 .flags = I2C_M_RD,
                 .len = length,
                 .buf = rxData,
                 },
        };

        for (loop_i = 0; loop_i < 10; loop_i++) {
                if ((ret=i2c_transfer(mcpld_info->i2c_client->adapter, msgs, 1)) > 0)
                        break;
#ifdef CPLD_I2C_DEBUG
		printk(KERN_INFO "%s: cpld ret is %d\n", __func__, ret);
#endif
                
                if (loop_i == 0 || loop_i == I2C_RETRY_COUNT - 1)
                        E("[I2C CPLD] %s, i2c err, slaveAddr 0x%x loop_i =%d \n",
                                __func__, slaveAddr,loop_i);

                msleep(10);
        }
        if (loop_i >= I2C_RETRY_COUNT) {
                printk(KERN_ERR "[I2C CPLD] %s retry over %d\n",
                        __func__, I2C_RETRY_COUNT);
                return -EIO;
        }

        return 0;
}

int CPLD_I2C_Write_Byte(uint16_t SlaveAddress,
                                uint8_t data)
{
        char buffer[2];
        int ret = 0;
#if 0
        
        printk(KERN_DEBUG
        "[CM3628] %s: _cm3628_I2C_Write_Byte[0x%x, 0x%x, 0x%x]\n",
                __func__, SlaveAddress, cmd, data);

/       buffer[0] = cmd;
        buffer[1] = data;
#endif
	buffer[0] = data;
        ret = I2C_TxData(SlaveAddress, buffer, 1);
        if (ret < 0) {
                pr_err("[CCPLD]%s: I2C_TxData fail\n", __func__);
                return -EIO;
        }
#ifdef CPLD_I2C_DEBUG
	printk("CPLD I2c slave is 0x%x ,Write data is 0x%x\n",SlaveAddress,buffer[0]);
#endif
        return ret;
}

int CPLD_I2C_Read_Byte(uint16_t slaveAddr, uint8_t *pdata)
{
        uint8_t buffer = 0;
        int ret = 0;

        if (pdata == NULL)
                return -EFAULT;

        ret = I2C_RxData(slaveAddr, &buffer, 1);
        if (ret < 0) {
                pr_err(
                        "[CCPLD]%s: I2C_RxData fail, slave addr: 0x%x\n",
                        __func__, slaveAddr);
                return ret;
        }
        *pdata = buffer;
#ifdef CPLD_I2C_DEBUG
	 printk("CPLD I2c slaveAddr is 0x%x Read data is 0x%x\n",slaveAddr,pdata[0]);
#endif
#if 0
        
        printk(KERN_DEBUG "[CM3628] %s: I2C_RxData[0x%x] = 0x%x\n",
                __func__, slaveAddr, buffer);
#endif
        return ret;
}


#if 0
static struct i2c_client *this_client;

struct bma250_data {
	struct input_dev *input_dev;
	struct work_struct work;
	struct early_suspend early_suspend;
};

#endif

static void report_near_do_work(struct work_struct *w)
{
	struct cpld_info *lpi = mcpld_info;
        uint8_t cdata[1];
        uint8_t ccdata =0;
	int i=0;
	int slave_cpld[6] = {0x70,0x61,0x52,0x43,0x3C,0x25};
	int data_i2c[6] = {0x11,0x22,0x33,0x44,0x55,0x04};
	int data2_i2c[6] = {0x65,0x54,0x43,0x32,0x01};
#if 0
	if(iii%2 == 0)
		i2c_data = 0x22;
	else
		i2c_data = 0x33;
#endif
	for(i=0;i<=5;i++){
		if(iii%2 == 0){
			CPLD_I2C_Write_Byte(slave_cpld[i],data_i2c[i]);
			gwdata = data_i2c[i];
		}
		else{
			CPLD_I2C_Write_Byte(slave_cpld[i],data2_i2c[i]);
			gwdata = data2_i2c[i];
		}
		stress_count++;
		cdata[0]=ccdata;
		CPLD_I2C_Read_Byte(slave_cpld[i],cdata);
		grdata = cdata[0];

		if(grdata == gwdata){
			mcount++;
			printk("CPLD read and Write match mcount is %d, stress count is %d\n",mcount,stress_count);
		}
	}

	queue_delayed_work(lpi->lp_wq, &report_near_work,
				msecs_to_jiffies(20));

	iii++;
}

extern void level_1_wq_fn(void);

static void handle_i2c_cpld_int_fn(struct work_struct *work)
{
#if 0
	uint8_t rdata;

	pr_info("[I2C CPLD ]%s: CPLD interrupt processing...\n", __func__);

	
	CPLD_I2C_Read_Byte(0x16, &rdata);
	pr_info("[I2C CPLD ]%s: CPLD GPI_LEVEL=0x%X...\n", __func__, rdata);

	
	CPLD_I2C_Read_Byte(0x07, &rdata);
	pr_info("[I2C CPLD ]%s: CPLD INT_STATUS=0x%X...\n", __func__, rdata);
#endif
	level_1_wq_fn();

	enable_irq(mcpld_info->i2c_client->irq);
}

static ssize_t i2c_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        char *s = buf;
        s += sprintf(s, "i2c_debug_flag = 0x%x i2c_data = 0x%x\n", i2c_debug_flag,i2c_data);
        return s - buf;
}

static ssize_t i2c_debug_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	struct cpld_info *lpi = mcpld_info;
        uint8_t cdata[1];
        uint8_t ccdata = 0;
	int i2c_data = 0;
        i2c_debug_flag = -1;
        sscanf(buf, "%d 0x%x", &i2c_debug_flag,&i2c_data);
        pr_info("%s: i2c_debug_flag = %d i2c_data = 0x%x\n", __func__, i2c_debug_flag,i2c_data);
        if(i2c_debug_flag ==1){
                CPLD_I2C_Write_Byte(cpld_slave,i2c_data);
        }
        if(i2c_debug_flag ==2){
                cdata[0]=ccdata;
                CPLD_I2C_Read_Byte(cpld_slave,cdata);
        }
	if(i2c_debug_flag ==3){
		queue_delayed_work(lpi->lp_wq, &report_near_work,
					msecs_to_jiffies(20));


	}

	if(i2c_debug_flag ==4){
		cancel_delayed_work(&report_near_work);
	}

        return count;
}

static DEVICE_ATTR(debug_en, 0664, i2c_debug_show, i2c_debug_store);

static ssize_t cpld_slave_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        char *s = buf;
        s += sprintf(s, "cpld slave is = 0x%x\n", cpld_slave);
        return s - buf;
}

static ssize_t cpld_slave_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{

        sscanf(buf, "%x", &cpld_slave);
        pr_info("%s: cpld slave is = %x\n", __func__, cpld_slave);

        return count;
}

static DEVICE_ATTR(cpld_s, 0664, cpld_slave_show, cpld_slave_store);


int i2c_registerAttr(void){
        int ret;
        struct class *htc_i2c_class;
        struct device *i2c_dev;
        htc_i2c_class = class_create(THIS_MODULE,"cpld_i2c");
        if (IS_ERR(htc_i2c_class)) {
                ret = PTR_ERR(htc_i2c_class);
                htc_i2c_class = NULL;
                goto err_create_class;
        }
        i2c_dev = device_create(htc_i2c_class,NULL, 0, "%s", "i2c");
        if (unlikely(IS_ERR(i2c_dev))) {
                ret = PTR_ERR(i2c_dev);
                i2c_dev = NULL;
                goto err_create_i2c_device;
        }
         
        ret = device_create_file(i2c_dev, &dev_attr_debug_en);
        if (ret){
                        goto err_create_i2c_device;
	}
        ret = device_create_file(i2c_dev, &dev_attr_cpld_s);
        if (ret){
                        goto err_create_i2c_device;
        }


        return 0;
err_create_i2c_device:
        class_destroy(htc_i2c_class);
err_create_class:
        return ret;
}


#if 0
static int cpld_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int cpld_release(struct inode *inode, struct file *file)
{
	return 0;
}

#endif

static int cpld_suspend(struct i2c_client *client, pm_message_t mesg)
{
	
	int ret;
#if 0
	cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 0);	
	cpld_gpio_write(CPLD_EXT_GPIO_AUD_SPK_RCV_SEL, 0);
	cpld_gpio_write(CPLD_EXT_GPIO_AUD_REV_EN, 0);
	cpld_gpio_write(CPLD_EXT_GPIO_BT_SHUTDOWN, 0);
#endif
	
	if(get_cpld_keyirq_wake()==1)
		{
			cpld_irq_unmask(CPLD_IRQ_VOL_UP);
			cpld_irq_unmask(CPLD_IRQ_VOL_DW);
		}
	else
		{
			cpld_irq_mask(CPLD_IRQ_VOL_UP);
			cpld_irq_mask(CPLD_IRQ_VOL_DW);
		}
	

#if 0 
	printk(KERN_INFO "=======CPLD GPIO========\n");
	printk(KERN_INFO "[I2C CPLD] %s\n", __func__);
	CPLD_I2C_Read_Byte(CPLD_REG_1, &rdata);
	printk(KERN_INFO "[I2C CPLD] reg01(GPO 01-08): 0x%02x\n", rdata);
	CPLD_I2C_Read_Byte(CPLD_REG_2, &rdata);
	printk(KERN_INFO "[I2C CPLD] reg02(GPO 09-16): 0x%02x\n", rdata);
	CPLD_I2C_Read_Byte(CPLD_REG_3, &rdata);
	printk(KERN_INFO "[I2C CPLD] reg03(GPO 17-24): 0x%02x\n", rdata);
	CPLD_I2C_Read_Byte(CPLD_REG_4, &rdata);
	printk(KERN_INFO "[I2C CPLD] reg04(GPO 25-32): 0x%02x\n", rdata);
	CPLD_I2C_Read_Byte(CPLD_REG_5, &rdata);
	printk(KERN_INFO "[I2C CPLD] reg05(GPO 33-40): 0x%02x\n", rdata);
	printk(KERN_INFO "===========END==========\n");
#endif 

	
        ret = gpio_request(GPIO_CPLD_CLK, "CPLD_CLK");
        if (ret < 0) {
                pr_err("[I2C CPLD][GPIO]%s: gpio %d request failed (%d)\n",
                        __func__, GPIO_CPLD_CLK, ret);
		
        }

	if(htc_get_board_revision() == BOARD_EVM) {
		#if defined(CONFIG_MACH_CP3DUG) || defined(CONFIG_MACH_CP3DCG) || defined(CONFIG_MACH_CP3DTG) || defined(CONFIG_MACH_CP3U)
			gpio_direction_output(GPIO_CPLD_CLK,   0); 
		#else
			gpio_direction_output(GPIO_CPLD_CLK,   1); 
		#endif
	}
	else
		gpio_direction_output(GPIO_CPLD_CLK,   1); 

	printk(KERN_INFO "[I2C CPLD] CPLD_CLK(%d):%d\n", GPIO_CPLD_CLK, gpio_get_value(GPIO_CPLD_CLK));

	if(!ret) gpio_free(GPIO_CPLD_CLK);

        
        
        ret = gpio_request(GPIO_CPLD_I2C_SCL, "CPLD_I2C_SCL");
        if(!gpio_direction_input(GPIO_CPLD_I2C_SCL))
                printk(KERN_INFO "[I2C CPLD] set GPIO:%d as input.\n", GPIO_CPLD_I2C_SCL);
        if(!ret) gpio_free(GPIO_CPLD_I2C_SCL);
        
        ret = gpio_request(GPIO_CPLD_I2C_SDA, "CPLD_I2C_SDA");
        if(!gpio_direction_input(GPIO_CPLD_I2C_SDA))
                printk(KERN_INFO "[I2C CPLD] set GPIO:%d as input.\n", GPIO_CPLD_I2C_SDA);
        if(!ret) gpio_free(GPIO_CPLD_I2C_SDA);

	return 0;
}

static int cpld_resume(struct i2c_client *client)
{
	int ret;
	printk(KERN_INFO "[I2C CPLD ]%s\n", __func__);
	ret = gpio_request(GPIO_CPLD_CLK, "CPLD_CLK");
        if (ret < 0) {
                pr_err("[I2C CPLD][GPIO]%s: gpio %d request failed (%d)\n",
                        		__func__, GPIO_CPLD_CLK, ret);
		
        }
	
	if(htc_get_board_revision() == BOARD_EVM) {
		#if defined(CONFIG_MACH_CP3DUG) || defined(CONFIG_MACH_CP3DCG) || defined(CONFIG_MACH_CP3DTG) || defined(CONFIG_MACH_CP3U)
			gpio_direction_output(GPIO_CPLD_CLK,   1); 
		#else
			gpio_direction_output(GPIO_CPLD_CLK,   0); 
		#endif
	}
	else
		gpio_direction_output(GPIO_CPLD_CLK,   0); 

	if(!ret) gpio_free(GPIO_CPLD_CLK);

#if 1
        
	
        ret = gpio_request(GPIO_CPLD_I2C_SCL, "CPLD I2C_SCL");
        if(!gpio_direction_output(GPIO_CPLD_I2C_SCL, 1))
                printk(KERN_INFO "[I2C CPLD] set GPIO:%d as output, high.\n", GPIO_CPLD_I2C_SCL);
	if(!ret) gpio_free(GPIO_CPLD_I2C_SCL);
	
	ret = gpio_request(GPIO_CPLD_I2C_SDA, "CPLD_I2C_SDA");
	if(!gpio_direction_output(GPIO_CPLD_I2C_SDA, 1))
		printk(KERN_INFO "[I2C CPLD] set GPIO:%d as output, high.\n", GPIO_CPLD_I2C_SDA);
        if (!ret) gpio_free(GPIO_CPLD_I2C_SDA);

#endif
	
	cpld_irq_unmask(CPLD_IRQ_VOL_UP);
	cpld_irq_unmask(CPLD_IRQ_VOL_DW);
	

#if 0
        cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 1);
        cpld_gpio_write(CPLD_EXT_GPIO_AUD_SPK_RCV_SEL, 1);
        cpld_gpio_write(CPLD_EXT_GPIO_AUD_REV_EN, 1);
        cpld_gpio_write(CPLD_EXT_GPIO_BT_SHUTDOWN, 1);
#endif
	return 0;
}

#if 0
static const struct file_operations bma_fops = {
	.owner = THIS_MODULE,
	.open = cpld_open,
	.release = cpld_release,
	
};

static struct miscdevice bma_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "cpld",
	.fops = &bma_fops,
};
#endif


static irqreturn_t cpld_int_handler(int irq, void *dev_id)
{
	disable_irq_nosync(mcpld_info->i2c_client->irq);

	pr_info("[I2C CPLD ]%s: CPLD interrupt received!\n", __func__);

	queue_work(mcpld_info->lp_wq, &i2c_cpld_int_work);

	return IRQ_HANDLED;
}


static int cpld_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct cpld_info *cpldi;
	struct cpld_platform_data *pdata;
	int ret =0 ;
	mcount = 0;
	stress_count =0;
	iii=0;

	printk("[VERSION] board_version: %x\n", htc_get_board_revision());

	printk(KERN_INFO "[I2C CPLD] %s\n", __func__);

	
        ret = gpio_request(4, "CPLD_CLK");
        if (ret < 0) {
                pr_err("[CPLD][GPIO]%s: gpio %d request failed (%d)\n",
                        __func__, 4, ret);
                return ret;
        }

        ret = gpio_request(49, "CPLD_RST");
        if (ret < 0) {
                pr_err("[CPLD][GPIO]%s: gpio %d request failed (%d)\n",
                        __func__, 49, ret);
                return ret;
        }
        ret = gpio_request(116, "CPLD_I2C_EN");
        if (ret < 0) {
                pr_err("[CPLD][GPIO]%s: gpio %d request failed (%d)\n",
                        __func__, 116, ret);
                return ret;
        }

	
	printk(KERN_INFO "gpio_CPLD_CLK:%d\n", gpio_get_value(4));
	printk(KERN_INFO "gpio_CPLD_RST:%d\n", gpio_get_value(49));
	printk(KERN_INFO "gpio_I2C_EN:%d\n", gpio_get_value(116));
	

	
	
	
	gpio_direction_output(GPIO_CPLD_RST,   1); 
	gpio_direction_output(GPIO_CPLD_I2CEN, 1); 

	
	gpio_set_value(GPIO_CPLD_I2CEN, 1);
#if 0
	
	
	gpio_set_value(GPIO_CPLD_CLK,   1);	
#endif
	
	gpio_set_value(GPIO_CPLD_RST,   1);


	gpio_free(GPIO_CPLD_RST);
	gpio_free(GPIO_CPLD_I2CEN);
	gpio_free(GPIO_CPLD_CLK);


	cpld_slave =0x70;

	cpldi= kzalloc(sizeof(struct cpld_info), GFP_KERNEL);
        if (!cpldi)
                return -ENOMEM;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "i2c_exit_check_functionality failed. \n");
	}
        cpldi->i2c_client = client;
        pdata = client->dev.platform_data;
        if (!pdata) {
                pr_err("[I2C CPLD ]%s: Assign platform_data error!!\n",
                        __func__);
        }
	ret = i2c_registerAttr();
	if (ret){
			printk("%s: set cpld_i2c_registerAttr fail!\n", __func__);
	}

	i2c_set_clientdata(client, cpldi);
	mcpld_info = cpldi;
        cpldi->lp_wq = create_singlethread_workqueue("cpld_wq");
        if (!cpldi->lp_wq) {
                pr_err("[I2C CPLD]%s: can't create workqueue\n", __func__);
        }


	
     	

	if (client->irq) {
		ret = request_irq(client->irq, cpld_int_handler,
				  IRQF_TRIGGER_LOW, "I2C CPLD INT", cpldi);
		if (ret == 0) {
			printk(KERN_INFO "[I2C CPLD]%s: irq enabled at qpio: %d\n", __func__, client->irq);
			
			enable_irq_wake(client->irq);
		} else {
			printk(KERN_ERR "[I2C CPLD][ERR]%s: request_irq failed\n", __func__);
		}
	} else {
		printk(KERN_INFO "[I2C CPLD][ERR]%s: client->irq is empty!!!\n", __func__);
	}

	flag_cpld_ready = 1;	
	printk(KERN_INFO "[I2C CPLD] %s: done\n", __func__);

	return 0;
}

static int cpld_remove(struct i2c_client *client)
{
	struct cpld_data *cpld = i2c_get_clientdata(client);

	flag_cpld_ready = 0;

	disable_irq_nosync(client->irq);

	kfree(cpld);
	return 0;
}

static const struct i2c_device_id cpld_id[] = {
	{ "cpld", 0 },
	{ }
};

static struct i2c_driver cpld_driver = {
	.probe = cpld_probe,
	.remove = cpld_remove,
	.id_table	= cpld_id,

#ifndef EARLY_SUSPEND_BMA
	.suspend = cpld_suspend,
	.resume = cpld_resume,
#endif
	.driver = {
		   .name = "cpld",
		   },
};


static int __init cpld_init(void)
{
#ifdef CONFIG_I2C_CPLD
	I("CPLD i2c driver: init\n");

	if( cpld_char_init() )
		return -1;

	
	return i2c_add_driver(&cpld_driver);
#else
	return -1;
#endif
}

static void __exit cpld_exit(void)
{
	cpld_char_exit();

	i2c_del_driver(&cpld_driver);
}

fs_initcall(cpld_init);


MODULE_DESCRIPTION("CPLD driver");
// MODULE_LICENSE("GPL");

