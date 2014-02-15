#include <linux/timer.h>
#include <linux/cdev.h> 

#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/module.h>


#include <linux/i2c/cpld.h>


#define DEBUG_LEVEL 0x00010001 
#define PK_DEBUG(a, format, arg...) if(a & DEBUG_LEVEL) printk(format, ##arg)
#define PK_INFO(a, format, arg...) if((a<<16) & DEBUG_LEVEL) pr_info(format, ##arg)

static DEFINE_MUTEX(cpld_char_mtx); 
static int dev_major;
static int dev_minor;
struct cdev *cdev_cpld = NULL;

#ifdef CPLD_TEST
#define TEST_DELAY_1 HZ/2   
#define TEST_DELAY_2 HZ/25  

#define READ_WIRTE_LOOP 20

struct workqueue_struct *timer_simulator_wq;
struct workqueue_struct *gpio_read_wq;
struct workqueue_struct *gpio_write_wq;

struct timer_list stress_test_timer;
struct timer_list gpio_read_timer;
struct timer_list gpio_write_timer;

static void cpld_test_work_fn(struct work_struct *work);
static void gpio_read_fn(struct work_struct *work);
static void gpio_write_fn(struct work_struct *work);

static DECLARE_WORK(cpld_test_work, cpld_test_work_fn);
static DECLARE_WORK(cpld_gpio_r_work, gpio_read_fn);
static DECLARE_WORK(cpld_gpio_w_work, gpio_write_fn);

extern void level_1_wq_fn(void);

uint8_t simulated_reg_int_st;
uint8_t stress_mode = 0;

static void cpld_test_work_fn(struct work_struct *work)
{
	level_1_wq_fn( );
}

static void gpio_read_fn(struct work_struct *work)
{
	int i;
	for(i=0; i<=READ_WIRTE_LOOP; i++){
		cpld_gpio_read(i);
        PK_DEBUG(0x2, "[gpio_read_fn] DONE %d\n", i);
    }
}

static void gpio_write_fn(struct work_struct *work)
{
	int i;
	static int  value = 1;
	for(i=0; i<=READ_WIRTE_LOOP; i++){
		cpld_gpio_write(i, value);
        PK_DEBUG(0x2, "[gpio_wirte_fn] DONE %d\n", i);
    }
	value = ~value;
}

static void stress_test_timer_fn(unsigned long data)
{
	pr_info("[I2C CPLD ]%s: CPLD timer of test expired!\n", __func__);

	
	
    simulated_reg_int_st = (simulated_reg_int_st+1 % 6);


    
    {
		int ret = queue_work(timer_simulator_wq, &cpld_test_work);
		PK_DEBUG(0x2, "WQ ret = %d\n", ret);
    }

	stress_test_timer.expires = jiffies + TEST_DELAY_2;

	add_timer(&stress_test_timer);

	return;
}

static void gpio_read_timer_fn(unsigned long data)
{
	PK_INFO(0x1, "[I2C CPLD ]%s!\n", __func__);

	queue_work(gpio_read_wq, &cpld_gpio_r_work );

	gpio_read_timer.expires = jiffies + TEST_DELAY_1;

	add_timer(&gpio_read_timer);

	return;
}

static void gpio_write_timer_fn(unsigned long data)
{
	PK_INFO(0x1, "[I2C CPLD ]%s!\n", __func__);

	queue_work(gpio_write_wq, &cpld_gpio_w_work );

	gpio_write_timer.expires = jiffies + TEST_DELAY_1;

	add_timer(&gpio_write_timer);

	return;
}

static void stress_test_timer_init(void)
{  
	PK_DEBUG(0x1, KERN_INFO "[CPLD] %s\n", __func__);

	init_timer(&stress_test_timer);
	stress_test_timer.function = stress_test_timer_fn;
	stress_test_timer.data = ((unsigned long) 0);
	stress_test_timer.expires = jiffies + TEST_DELAY_2;
	add_timer(&stress_test_timer);

	init_timer(&gpio_read_timer);
	gpio_read_timer.function = gpio_read_timer_fn;
	gpio_read_timer.data = ((unsigned long) 0);
	gpio_read_timer.expires = jiffies + TEST_DELAY_1;
	add_timer(&gpio_read_timer);

	init_timer(&gpio_write_timer);
	gpio_write_timer.function = gpio_write_timer_fn;
	gpio_write_timer.data = ((unsigned long) 0);
	gpio_write_timer.expires = jiffies + TEST_DELAY_1;
	add_timer(&gpio_write_timer);
}
#endif


extern int CPLD_I2C_Write_Byte(uint16_t SlaveAddress, uint8_t data);
extern int CPLD_I2C_Read_Byte(uint16_t slaveAddr, uint8_t *pdata);


static ssize_t cpld_read(struct file *fp, char __user *buf,
                         size_t size, loff_t *off)
{
	int i, status;
	int ret;
	uint8_t reg_int_st,rdata;

	
	ret = CPLD_I2C_Read_Byte(CPLD_REG_INTR_STATUS, &reg_int_st);
	if(ret >= 0)
		printk(KERN_INFO "[CPLD]%s: CPLD INT_STATUS=0x%X...\n", __func__, reg_int_st);
        
        ret = CPLD_I2C_Read_Byte(CPLD_REG_7, &rdata);
        if(ret >= 0) 
		printk(KERN_INFO "[CPLD]%s: CPLD GPI_LEVEL=0x%X...\n", __func__, rdata);

	for( i=1 ; i < CPLD_EXT_GPIO_MAX ; ++i )
	{
		status = cpld_gpio_read(i);
		if( status == -1 )
		{
			PK_DEBUG(0x1, "[CPLD] E/ %s(): read: out of range of MAX_GPIO_NUM\n",
				   __FUNCTION__);
			goto out__cpld_read;
		}
		PK_DEBUG(0x2, "Result from gpio_read(%d): %d\n", i, status);
	}

	return 0;

out__cpld_read:
	return -EFAULT;
}


#define USER_BUF_SIZE 32
static ssize_t cpld_write(struct file *fp, const char __user *args,
                          size_t size, loff_t *off)
{
	int gpio_num = -1, direct = -1, gpio_level = -1;
	int status = 0;
	char tmp[USER_BUF_SIZE];


	if( size > USER_BUF_SIZE )
	{
		PK_DEBUG(0x1, "[CPLD] E/ %s(): input size exceed the 32 char(s)!\n",
			   __FUNCTION__ );
		goto out__cpld_write;
	}

	if( copy_from_user(tmp, args, size) )
	{
		PK_DEBUG(0x1, "[CPLD] E/ %s(): copy_from_user ERROR!\n",
			   __FUNCTION__);
		goto out__cpld_write;
	}
	sscanf(tmp, "%d,%d,%d\n", &gpio_num, &direct, &gpio_level);

#ifdef CPLD_TEST
	
	if(gpio_level == -1){
		if( tmp[0]=='t' && tmp[1]=='e' && tmp[2]=='s' && tmp[3]=='t' )
		{
			if( stress_mode )
			{
				PK_INFO(0x1, "[CPLD] stress test already on!\n");
				return size;
			}

			simulated_reg_int_st = 1;
			stress_mode = 1;

			timer_simulator_wq = create_singlethread_workqueue("timer_simulator_wq");
			gpio_read_wq = create_singlethread_workqueue("gpio_read_wq");
			gpio_write_wq = create_singlethread_workqueue("gpio_write_wq");
			stress_test_timer_init( );
		
			return size;
		}
		else if( tmp[0]=='u' && tmp[1]=='n' && tmp[2]=='t' && tmp[3]=='e' && tmp[4]=='s' && tmp[5]=='t' )
		{
			if( !stress_mode )
			{
				PK_INFO(0x1, "[CPLD] stress test already off!\n");
				return size;
			}
			stress_mode = 0;
			del_timer(&stress_test_timer);	
			del_timer(&gpio_read_timer);
			del_timer(&gpio_write_timer);
			return size;
		}
		else if( tmp[0]=='u' && tmp[1]=='n' && tmp[2]=='m' && tmp[3]=='a' && tmp[4]=='s' && tmp[5]=='k' )
		{
			int irq = tmp[6]-48;
			if( irq >= 0 && irq <= 5 )
			{
				PK_DEBUG(0x2, "<CPLD> To unmask irq %d\n", irq);
				cpld_irq_unmask(irq);
			}
		}
		else if( tmp[0]=='m' && tmp[1]=='a' && tmp[2]=='s' && tmp[3]=='k' )
		{
			int irq = tmp[4]-48;
			if( irq >= 0 && irq <= 5 )
			{
				PK_DEBUG(0x2, "<CPLD> To mask irq %d\n", irq);
				cpld_irq_mask(irq);
			}
		}
	}
#endif
	mutex_lock(&cpld_char_mtx);

	if(gpio_num <= 0 || gpio_num >= CPLD_EXT_GPIO_MAX) goto fail_cpld_write; 

	if( direct == 0 ) 
	{
		
		status = cpld_gpio_read( gpio_num );
#if 0
		
        	if( status == 0 )
            		ret = copy_to_user(args, " 0\n", 3);
        	else if( status == 1 )
            		ret = copy_to_user(args, " 1\n", 3);
        	else
            		ret = copy_to_user(args, "-1\n", 3);

        	if (ret != 0)
        	{
            		PK_DEBUG(0x1, "[CPLD] E/ %s(): read: copy_to_user error.\n",
                   		__FUNCTION__);
            		goto fail_cpld_write;
        	}
#endif
		if( status == -1 )
		{
			PK_DEBUG(0x1, "[CPLD] E/ %s(): read: out of range of MAX_GPIO_NUM\n",
				   __FUNCTION__);
			goto fail_cpld_write;
		}
		PK_DEBUG(0x2, "Result from gpio_read(%d): %d\n", gpio_num, status);
    	}
	else if( direct == 1 ) 
	{
		status = cpld_gpio_write( gpio_num, gpio_level );
		if( status == -1 )
		{
			PK_DEBUG(0x1, "[CPLD] E/ %s(): write: out of range of MAX_GPIO_NUM\n",
				   __FUNCTION__);
			goto fail_cpld_write;
		}
		PK_DEBUG(0x2, "Result from gpio_write(%d): %d\n", gpio_num, status);
	}
	else
	{
		PK_DEBUG(0x1, "[CPLD] E/ %s(): Wrong direction assigned! (%d)\n",
			   __FUNCTION__, direct);
		goto fail_cpld_write;
	}

	mutex_unlock(&cpld_char_mtx);
	return size;

fail_cpld_write:
	mutex_unlock(&cpld_char_mtx);
	
out__cpld_write:
	return -EFAULT;
}

static int drv_open(struct inode *inode, struct file *filp)
{
	PK_DEBUG(0x4, "device open\n");
	return 0;
}

static int drv_release(struct inode *inode, struct file *filp)
{
	PK_DEBUG(0x4, "device close\n");
	return 0;
}

struct file_operations dev_fops =
{
  read:           cpld_read,
  write:          cpld_write,
  open:           drv_open,
  release:        drv_release,
};

#ifdef CPLD_TEST
static unsigned long data_for_isr[6] = {0, 1, 2, 3, 4, 5};

static void charge_int_isr(unsigned long data){
	unsigned int *irq_num = (unsigned int*)data;
	PK_INFO(0x1, "[CPLD] %s, %d\n", __func__, *irq_num);
}

#include <linux/proc_fs.h>
static const struct file_operations cpld_proc_fops = {
  .read       = cpld_read,
  .write      = cpld_write,
};

#endif

static struct class *fc;

int cpld_char_init(void)
{
	dev_t dev;
	int ret;

	ret = alloc_chrdev_region(&dev, 0, 1, "cpld_gpio");
	if (ret < 0)
	{
		PK_DEBUG(0x1, "[CPLD] E/ can't alloc chrdev\n");
		return ret;
	}
	dev_major = MAJOR(dev);
	dev_minor = MINOR(dev);
	printk(KERN_INFO "[CPLD] I/ register chrdev(%d,%d)\n", dev_major, dev_minor);

	cdev_cpld = kmalloc(sizeof(struct cdev), GFP_KERNEL);
	if (cdev_cpld == NULL) {
		PK_DEBUG(0x1, "[CPLD] E/ kmalloc failed\n");
		goto failed;
	}
	cdev_init(cdev_cpld, &dev_fops);
	cdev_cpld->owner = THIS_MODULE;
	ret = cdev_add(cdev_cpld, MKDEV(dev_major, dev_minor), 1);
	if (ret < 0) {
		PK_DEBUG(0x1, "[CPLD] E/ add chr dev failed\n");
		goto failed;
	}

#ifdef CPLD_TEST
	if(!cpld_request_irq(CPLD_IRQ_USB_ID, &charge_int_isr, data_for_isr ))
		PK_INFO(0x2, "[CPLD] request %d IRQ success!\n", CPLD_IRQ_USB_ID);
	if(!cpld_request_irq(CPLD_IRQ_CHARGE_INT, &charge_int_isr, data_for_isr+1 ))
		PK_INFO(0x2, "[CPLD] request %d IRQ success!\n", CPLD_IRQ_CHARGE_INT);
	if(!cpld_request_irq(CPLD_IRQ_CHARGE_STATE, &charge_int_isr, data_for_isr+2 ))
		PK_INFO(0x2, "[CPLD] request %d IRQ success!\n", CPLD_IRQ_CHARGE_STATE);
	if(!cpld_request_irq(CPLD_IRQ_VOL_UP, &charge_int_isr, data_for_isr+3 ))
		PK_INFO(0x2,  "[CPLD] request %d IRQ success!\n", CPLD_IRQ_VOL_UP);
	if(!cpld_request_irq(CPLD_IRQ_VOL_DW, &charge_int_isr, data_for_isr+4 ))
		PK_INFO(0x2, "[CPLD] request %d IRQ success!\n", CPLD_IRQ_VOL_DW);
	if(!cpld_request_irq(CPLD_IRQ_HP, &charge_int_isr, data_for_isr+5 ))
		PK_INFO(0x2, "[CPLD] request %d IRQ success!\n", CPLD_IRQ_HP);
#endif

	fc = class_create(THIS_MODULE, "my_class");
    	device_create(fc, NULL, MKDEV(dev_major, dev_minor), "%s", "cpld");

#ifdef CPLD_TEST
    	proc_create("cpld", 0777, NULL, &cpld_proc_fops);
#endif

	return 0;

failed:
	printk(KERN_ERR "%s: register failed.\n", __func__);
	if (cdev_cpld) {
		kfree(cdev_cpld);
		cdev_cpld = NULL;
	}
	return ret;
}


void cpld_char_exit(void)
{
	dev_t dev;

	dev = MKDEV(dev_major, dev_minor);
	if (cdev_cpld) {
		cdev_del(cdev_cpld);
		kfree(cdev_cpld);
	}
	unregister_chrdev_region(dev, 1);
	PK_DEBUG(0x2, "[CPLD] I/ unregister chrdev\n");


	device_destroy(fc, dev);
	class_destroy(fc);
}

