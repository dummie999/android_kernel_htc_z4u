/* drivers/i2c/chips/cpld_isr.c - CPLD driver
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

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/i2c/cpld.h>
#include "cpld_char.h"


 


static struct tasklet_struct cpld_irq_desc[CPLD_IRQ_NUM];


static DEFINE_MUTEX(cpld_gpio_mtx); 
static DEFINE_MUTEX(cpld_irq_req_mtx); 
static DEFINE_MUTEX(cpld_irq_mask_mtx); 
				    

extern int CPLD_I2C_Write_Byte(uint16_t SlaveAddress, uint8_t data);
extern int CPLD_I2C_Read_Byte(uint16_t slaveAddr, uint8_t *pdata);

extern int is_cpld_ready(void);

static int flag_usb_output=0;
static int cpld_keyirq_wake_enable=0;

void level_1_wq_fn(void)
{
	int i, ret;
	uint8_t reg_int_st, rdata;

	pr_info("[CPLD]%s: CPLD interrupt processing...\n", __func__);

	
	ret = CPLD_I2C_Read_Byte(CPLD_REG_INTR_STATUS, &reg_int_st);
	if(ret < 0) goto failed_I2C;

	pr_info("[CPLD]%s: CPLD INT_STATUS=0x%X...\n", __func__, reg_int_st);

#ifdef CPLD_TEST
	reg_int_st = stress_mode ? simulated_reg_int_st : reg_int_st;
#endif

        
        ret = CPLD_I2C_Read_Byte(CPLD_REG_7, &rdata);
        if(ret < 0) goto failed_I2C;

        pr_info("[CPLD]%s: CPLD GPI_LEVEL=0x%X...\n", __func__, rdata);

		
	for(i=0; i<CPLD_IRQ_NUM; i++){
		if(reg_int_st & (1 << i)){
			if( cpld_irq_desc[i].func ){
				printk(KERN_INFO "[CPLD]%s is handling %d\n", __func__, i);
				tasklet_schedule(&cpld_irq_desc[i]);
			}else{
				printk(KERN_ERR "[CPLD][%s] cpld_irq_desc[%d].func is NULL\n", __func__, i);
			}
		}
	}
	

	return;

failed_I2C:
	printk("[CPLD] Failed to I2C R/W!!\n");
	return;
}

int cpld_gpio_config(cpld_gpio_pin_t gpio_num, cpld_dir_t dir)
{
	int ret = -1;
	uint8_t reg_val = 0x0;
	uint8_t value = (dir == CPLD_GPIO_OUT)? 1:0;

	if(!is_cpld_ready()){
		printk(KERN_ERR "cpld isn't ready.\n");
		return -1;
	}

	mutex_lock(&cpld_gpio_mtx);
	if(gpio_num == CPLD_EXT_GPIO_EXT_USB_ID_OUTPUT)
	{	
         	
         	
         	
         	ret = CPLD_I2C_Read_Byte(CPLD_REG_6, &reg_val);
        	if(ret < 0 ) goto i2c_fail;

                reg_val = (reg_val & ~(1 << 6)) | (value << 6);

                ret = CPLD_I2C_Write_Byte(CPLD_REG_6, reg_val);
                if(ret < 0 ) goto i2c_fail;
		
		if(dir == CPLD_GPIO_OUT)
			flag_usb_output = 1;
		else
			flag_usb_output = 0;

		mutex_unlock(&cpld_gpio_mtx);
		return 0;
	}
	
i2c_fail:
	mutex_unlock(&cpld_gpio_mtx);	
	printk(KERN_ERR "[CPLD] %s: only for USB_ID pin. %d.\n", __func__, gpio_num);
	return -1;
}


int cpld_gpio_read(cpld_gpio_pin_t gpio_num)
{
	int ret;
	uint8_t reg_val = 0x0;

	if(!is_cpld_ready()){ 
		printk(KERN_ERR "cpld isn't ready.\n");
		return -1;
	}

	
	if(gpio_num <= 0 || gpio_num >= CPLD_EXT_GPIO_MAX){
		printk(KERN_ERR "[CPLD] %s: beyond the min gpio num:%d\n",
			    __func__, gpio_num);
		return -1;
	}

	mutex_lock(&cpld_gpio_mtx);

	if(gpio_num <= 8){   
                ret = CPLD_I2C_Read_Byte(CPLD_REG_1, &reg_val);
                if(ret < 0 ) goto i2c_fail;
		
		reg_val = (reg_val >> (gpio_num-1)) & 1;
        }
	else if(gpio_num <= 16){ 
		gpio_num -= 8;
		ret = CPLD_I2C_Read_Byte(CPLD_REG_2, &reg_val);
		if(ret < 0 ) goto i2c_fail;

		reg_val = (reg_val >> (gpio_num-1)) & 1;
	}
	else if(gpio_num <= 24){ 
		gpio_num -= 8*2;
		ret = CPLD_I2C_Read_Byte(CPLD_REG_3, &reg_val);
		if(ret < 0 ) goto i2c_fail;
		
		reg_val = (reg_val >> (gpio_num-1)) & 1;
	}
	else if(gpio_num <= 32){ 
		gpio_num -= 8*3;
		ret = CPLD_I2C_Read_Byte(CPLD_REG_4, &reg_val);
		if(ret < 0 ) goto i2c_fail;
		
		reg_val = (reg_val >> (gpio_num-1)) & 1;
	}
	else if(gpio_num <= 40){ 
		gpio_num -= 8*4;
		ret = CPLD_I2C_Read_Byte(CPLD_REG_5, &reg_val);
		if(ret < 0 ) goto i2c_fail;

		reg_val = (reg_val >> (gpio_num-1)) & 1;
	}
	else if(gpio_num == CPLD_EXT_GPIO_EXT_GPS_LNA_EN){
		ret = CPLD_I2C_Read_Byte(CPLD_REG_6, &reg_val);
		if(ret < 0 ) goto i2c_fail;

		reg_val = (reg_val >> 6) & 1;
	}
	else if(gpio_num == CPLD_EXT_GPIO_GPS_2V85_EN){
		ret = CPLD_I2C_Read_Byte(CPLD_REG_6, &reg_val);
		if(ret < 0 ) goto i2c_fail;

		reg_val = (reg_val >> 7) & 1;
	}
	else if((gpio_num >= CPLD_GPI_START) && (gpio_num <= CPLD_GPI_END))
	{
		gpio_num -= 42;
		ret = CPLD_I2C_Read_Byte(CPLD_REG_7, &reg_val);
		if(ret < 0 ) goto i2c_fail;

		reg_val = (reg_val >> (gpio_num-1)) & 1;
	}

	mutex_unlock(&cpld_gpio_mtx);

	return reg_val;

i2c_fail:
	printk(KERN_ERR "[CPLD] %s: i2c_fail\n", __func__);
	mutex_unlock(&cpld_gpio_mtx);
	return -1;
}

int cpld_gpio_write(cpld_gpio_pin_t gpio_num, int value)
{
	int ret = -1;
	uint8_t reg_val = 0x0;
	uint8_t lv = (value > 0)? 1:0;

	if(!is_cpld_ready()){
		printk(KERN_ERR "cpld isn't ready.\n");
		return -1;
        }

	
	if(gpio_num <= 0 || gpio_num >= CPLD_EXT_GPIO_MAX){
		printk(KERN_ERR "[CPLD] %s: out of gpio num:%d\n",
			   __func__, gpio_num);
		return -1;
	}

    	mutex_lock(&cpld_gpio_mtx);

	if(gpio_num <= 8){   
		ret = CPLD_I2C_Read_Byte(CPLD_REG_1, &reg_val);
		if(ret < 0 ) goto i2c_fail;
				
		reg_val = (reg_val & ~(1 << (gpio_num-1))) | (lv << (gpio_num-1));

		ret = CPLD_I2C_Write_Byte(CPLD_REG_1, reg_val);
		if(ret < 0 ) goto i2c_fail;
	}
	else if(gpio_num <= 16){ 
		gpio_num -= 8;
		ret = CPLD_I2C_Read_Byte(CPLD_REG_2, &reg_val);
		if(ret < 0 ) goto i2c_fail;

		reg_val = (reg_val & ~(1 << (gpio_num-1))) | (lv << (gpio_num-1));		

		ret = CPLD_I2C_Write_Byte(CPLD_REG_2, reg_val);
		if(ret < 0 ) goto i2c_fail;
	}
	else if(gpio_num <= 24){ 
		gpio_num -= 8*2;
		ret = CPLD_I2C_Read_Byte(CPLD_REG_3, &reg_val);
		if(ret < 0 ) goto i2c_fail;
		
		reg_val = (reg_val & ~(1 << (gpio_num-1))) | (lv << (gpio_num-1));
		
		ret = CPLD_I2C_Write_Byte(CPLD_REG_3, reg_val);
		if(ret < 0 ) goto i2c_fail;
	}
	else if(gpio_num <= 32){ 
		gpio_num -= 8*3;
		ret = CPLD_I2C_Read_Byte(CPLD_REG_4, &reg_val);
		if(ret < 0 ) goto i2c_fail;

		reg_val = (reg_val & ~(1 << (gpio_num-1))) | (lv << (gpio_num-1));

                ret = CPLD_I2C_Write_Byte(CPLD_REG_4, reg_val);
		if(ret < 0 ) goto i2c_fail;
	}
	else if(gpio_num <= 40){ 
		gpio_num -= 8*4;

		ret = CPLD_I2C_Read_Byte(CPLD_REG_5, &reg_val);
		if(ret < 0 ) goto i2c_fail;

		reg_val = (reg_val & ~(1 << (gpio_num-1))) | (lv << (gpio_num-1));
		
		ret = CPLD_I2C_Write_Byte(CPLD_REG_5, reg_val);
		if(ret < 0 ) goto i2c_fail;	
	}
	else if(gpio_num == CPLD_EXT_GPIO_EXT_GPS_LNA_EN){
		
		
		
		ret = CPLD_I2C_Read_Byte(CPLD_REG_6, &reg_val);
		if(ret < 0 ) goto i2c_fail;

		reg_val = (reg_val & ~(1 << 6)) | (lv << 6);
		
		ret = CPLD_I2C_Write_Byte(CPLD_REG_6, reg_val);
		if(ret < 0 ) goto i2c_fail;
		
                if(lv == CPLD_GPIO_OUT)
                        flag_usb_output = 1;
                else
                        flag_usb_output = 0;

	}
	else if(gpio_num == CPLD_EXT_GPIO_GPS_2V85_EN){
		
		
		ret = CPLD_I2C_Read_Byte(CPLD_REG_6, &reg_val);
		if(ret < 0 ) goto i2c_fail;

		reg_val = (reg_val & ~(1 << 7)) | (lv << 7);

		ret = CPLD_I2C_Write_Byte(CPLD_REG_6, reg_val);
		if(ret < 0 ) goto i2c_fail;
	}
	else if(gpio_num == CPLD_EXT_GPIO_USB_ID_INPUT_LEVEL){
		
		if(!flag_usb_output){
			printk(KERN_ERR "[CPLD] %s: GPIO_USB_ID_INPUT not yet be configured as output.\n", __func__);
			goto conf_fail;
		}
		if(!lv){
			printk(KERN_ERR "[CPLD] %s: GPIO_USB_ID_INPUT can't be pull low.\n", __func__);
			goto conf_fail;
		}
		
		ret = 0;
#if 0
		ret = CPLD_I2C_Read_Byte(CPLD_REG_7, &reg_val);
		if(ret < 0 ) goto i2c_fail;
		
		reg_val = (reg_val & ~(1 << 7)) | (lv << 7);

		ret = CPLD_I2C_Write_Byte(CPLD_REG_6, reg_val);
		if(ret < 0 ) goto i2c_fail;
#endif			
	}

	mutex_unlock(&cpld_gpio_mtx);

	return ret;

i2c_fail:
	printk(KERN_ERR "[CPLD] %s: i2c_fail\n", __func__);
conf_fail:
	mutex_unlock(&cpld_gpio_mtx);
	return -1;
}

int cpld_irq_mask(cpld_irq_t cpld_irq_num)
{
	int ret = 0;	
	uint8_t reg_val;

        if(!is_cpld_ready()){
                printk(KERN_ERR "cpld isn't ready.\n");
                return -1;
        }

	if(cpld_irq_num >= CPLD_IRQ_NUM || cpld_irq_num < 0)
		return -1;

	
	
	mutex_lock(&cpld_irq_mask_mtx);

	ret = CPLD_I2C_Read_Byte(CPLD_REG_INTR_MASK, &reg_val);
	if(ret < 0) goto i2c_fail;
	
	reg_val = (reg_val | (1 << (cpld_irq_num)));
	printk(KERN_DEBUG "[CPLD] %s irq:%d reg_val:%x\n", __func__, cpld_irq_num, reg_val);
	ret = CPLD_I2C_Write_Byte(CPLD_REG_INTR_MASK, reg_val);
	if(ret < 0) goto i2c_fail;

	
	mutex_unlock(&cpld_irq_mask_mtx);
	return ret;

i2c_fail:
	
	mutex_unlock(&cpld_irq_mask_mtx);
	return -1;

}

int cpld_irq_unmask(cpld_irq_t cpld_irq_num)
{
        int ret = 0;
        uint8_t reg_val;

        if(!is_cpld_ready()){
                printk(KERN_ERR "cpld isn't ready.\n");
                return -1;
        }

	if(cpld_irq_num >= CPLD_IRQ_NUM || cpld_irq_num < 0)
                return -1;

	
	
	mutex_lock(&cpld_irq_mask_mtx);

        ret = CPLD_I2C_Read_Byte(CPLD_REG_INTR_MASK, &reg_val);
        if(ret < 0) goto i2c_fail;
	
	reg_val = (reg_val & ~(1 << (cpld_irq_num)));
	printk(KERN_DEBUG "[CPLD] %s irq:%d reg_val:%x\n", __func__, cpld_irq_num, reg_val);
        ret = CPLD_I2C_Write_Byte(CPLD_REG_INTR_MASK, reg_val);
        if(ret < 0) goto i2c_fail;

	
	mutex_unlock(&cpld_irq_mask_mtx);
        return ret;
i2c_fail:
	
	mutex_unlock(&cpld_irq_mask_mtx);
	return -1;
}
int enable_cpld_keyirq_wake(cpld_irq_t cpld_irq_num)
{
	int ret = 0;
	if(!is_cpld_ready()){
                printk(KERN_ERR "cpld isn't ready.\n");
                return -1;
        }

	if(cpld_irq_num != CPLD_IRQ_VOL_UP && cpld_irq_num != CPLD_IRQ_VOL_DW)
                return -1;

    cpld_keyirq_wake_enable=1;
	pr_info("[CPLD] cpld_keyirq_wake_enable=1\n");
	return ret;

}
int disable_cpld_keyirq_wake(cpld_irq_t cpld_irq_num)
{
	int ret = 0;
	if(!is_cpld_ready()){
                printk(KERN_ERR "cpld isn't ready.\n");
                return -1;
        }

	if(cpld_irq_num != CPLD_IRQ_VOL_UP && cpld_irq_num != CPLD_IRQ_VOL_DW)
                return -1;

    cpld_keyirq_wake_enable=0;
	pr_info("[CPLD] cpld_keyirq_wake_enable=0\n");
	return ret;

}
int get_cpld_keyirq_wake(void)
{
	return cpld_keyirq_wake_enable;
}

#if 0
void charge_int_isr(unsigned long data){
	unsigned int *irq_num = (unsigned int*)data;

	printk(KERN_INFO "[CPLD] %s, %d\n", __func__, *irq_num);
}

unsigned int test_irq_num;

static int __init cpld_init(void)
{
        printk(KERN_INFO "[CPLD] i2c driver: init\n");
	lp_wq = create_singlethread_workqueue("cpld_wq");

	
	test_irq_num = CPLD_IRQ_CHARGE_INT;
	if(!cpld_request_irq(CPLD_IRQ_CHARGE_INT, &charge_int_isr, &test_irq_num ))
		printk(KERN_INFO "[CPLD] request %d IRQ success!\n", CPLD_IRQ_CHARGE_INT);

	
	if(lp_wq){
		queue_work(lp_wq, &level_1_wq);
	}

        return 0;
}


static void __exit cpld_exit(void)
{
	printk(KERN_INFO "[CPLD] i2c driver: exit");
	return;
}
#endif

int cpld_release_irq(enum cpld_irq irq_from_CPLD)
{
  int ret=0;
 
  if(!is_cpld_ready()){
        printk(KERN_ERR "cpld isn't ready.\n");
        return -1;
  }
 
  mutex_lock(&cpld_irq_req_mtx);

  

  tasklet_kill( &cpld_irq_desc[irq_from_CPLD] );
  
  cpld_irq_desc[irq_from_CPLD].func = NULL;
  
#if 0
  

  
  
  

  
#endif
  mutex_unlock(&cpld_irq_req_mtx);

  return ret;
}

int cpld_request_irq(const cpld_irq_t cpld_irq_num,
                     cpld_irq_handler_t handler, void *dev)
{
  int error_num = -1;

  if(!is_cpld_ready()){
  	printk(KERN_ERR "cpld isn't ready.\n");
        return -1;
  }

  if(!handler)
	return error_num;

  
   mutex_lock(&cpld_irq_req_mtx);
            
  

  switch( cpld_irq_num ){
  case CPLD_IRQ_VOL_UP:
    if( !cpld_irq_desc[CPLD_IRQ_VOL_UP].func )
    {
      tasklet_init(&cpld_irq_desc[CPLD_IRQ_VOL_UP],
                   handler, (unsigned long) dev);
      
    }
    break;
  case CPLD_IRQ_VOL_DW:
    if( !cpld_irq_desc[CPLD_IRQ_VOL_DW].func )
    {
      tasklet_init(&cpld_irq_desc[CPLD_IRQ_VOL_DW],
                   handler, (unsigned long) dev);
      
    }
    break;
  case CPLD_IRQ_HP:
    if( !cpld_irq_desc[CPLD_IRQ_HP].func )
    {
      tasklet_init(&cpld_irq_desc[CPLD_IRQ_HP],
                   handler, (unsigned long) dev);
      
    }
    break;
  case CPLD_IRQ_USB_ID:
    if( !cpld_irq_desc[CPLD_IRQ_USB_ID].func )
    {
      tasklet_init(&cpld_irq_desc[CPLD_IRQ_USB_ID],
                   handler, (unsigned long) dev);
      
    }
    break;
  case CPLD_IRQ_CHARGE_STATE:
    if( !cpld_irq_desc[CPLD_IRQ_CHARGE_STATE].func )
    {
      tasklet_init(&cpld_irq_desc[CPLD_IRQ_CHARGE_STATE],
                   handler, (unsigned long) dev);
      
    }
    break;
  case CPLD_IRQ_CHARGE_INT:
    if( !cpld_irq_desc[CPLD_IRQ_CHARGE_INT].func )
    {
      tasklet_init(&cpld_irq_desc[CPLD_IRQ_CHARGE_INT],
                   handler, (unsigned long) dev);
      
    }
    break;
  default:
    printk(KERN_ERR "[CPLD] %s Error irq number %d!\n", __func__,  cpld_irq_num);
    break;
  }
 
  error_num = 0;

  mutex_unlock(&cpld_irq_req_mtx);
  
  return error_num;
}


EXPORT_SYMBOL(cpld_gpio_config);
EXPORT_SYMBOL(cpld_gpio_read);
EXPORT_SYMBOL(cpld_gpio_write);
EXPORT_SYMBOL(cpld_request_irq);
EXPORT_SYMBOL(cpld_release_irq);
EXPORT_SYMBOL(cpld_irq_mask);
EXPORT_SYMBOL(cpld_irq_unmask);
EXPORT_SYMBOL(enable_cpld_keyirq_wake);
EXPORT_SYMBOL(disable_cpld_keyirq_wake);
EXPORT_SYMBOL(get_cpld_keyirq_wake);

/*
module_init(cpld_init);
module_exit(cpld_exit);

MODULE_DESCRIPTION("CPLD driver");
MODULE_LICENSE("GPL");
*/
