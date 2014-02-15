#ifndef CPLD_H
#define CPLD_H

#include <linux/irqreturn.h>

#define GPIO_CPLD_RST       (49)
#define GPIO_CPLD_CLK       (4)
#define GPIO_CPLD_INT       (39)
#define GPIO_CPLD_I2CEN     (116)
#define GPIO_CPLD_I2C_SCL   (33)
#define GPIO_CPLD_I2C_SDA   (82)

#define CPLD_REG_1 0x70
#define CPLD_REG_2 0x61
#define CPLD_REG_3 0x52
#define CPLD_REG_4 0x43
#define CPLD_REG_5 0x3C
#define CPLD_REG_6 0x25
#define CPLD_REG_7 0x16
#define CPLD_REG_8 0x07
#define CPLD_REG_INTR_STATUS CPLD_REG_8
#define CPLD_REG_INTR_MASK CPLD_REG_6



enum cpld_irq {
	CPLD_IRQ_USB_ID = 0,
	CPLD_IRQ_CHARGE_INT,
	CPLD_IRQ_CHARGE_STATE,
	CPLD_IRQ_VOL_UP,
	CPLD_IRQ_VOL_DW,
	CPLD_IRQ_HP,
	
	CPLD_IRQ_NUM,
};

typedef enum cpld_irq cpld_irq_t;
typedef void (*cpld_irq_handler_t)(unsigned long);

int
cpld_request_irq(const cpld_irq_t cpld_irq_num, cpld_irq_handler_t handler, void *dev);

int 
cpld_release_irq(enum cpld_irq irq_from_CPLD);

int
enable_cpld_keyirq_wake(cpld_irq_t cpld_irq_num);
int
disable_cpld_keyirq_wake(cpld_irq_t cpld_irq_num);
int
get_cpld_keyirq_wake(void);


int cpld_irq_mask(cpld_irq_t cpld_irq_num);
int cpld_irq_unmask(cpld_irq_t cpld_irq_num);

typedef enum {
       CPLD_GPIO_IN = 0,
       CPLD_GPIO_OUT =1,
}cpld_dir_t;

typedef enum {
        CPLD_EXT_GPIO_LCMIO_1V8_EN = 1,         
        CPLD_EXT_GPIO_LCM_3V_EN,                
        CPLD_EXT_GPIO_LCD_RST,                  
        CPLD_EXT_GPIO_TP_RST,                   
        CPLD_EXT_GPIO_LED_3V3_EN,               
        CPLD_EXT_GPIO_PS_2V85_EN,               
        CPLD_EXT_GPIO_MDM_DEBUG_SEL,            
        CPLD_EXT_GPIO_PS_HOLD_EN,

        CPLD_EXT_GPIO_AP2MDM_MODE1 =9,          
        CPLD_EXT_GPIO_AP2MDM_MODE2,             
        CPLD_EXT_GPIO_AP2MDM_MODE3,             
        CPLD_EXT_GPIO_NFC_VEN,                  
        CPLD_EXT_GPIO_NFC_DL_MODE,              
        CPLD_EXT_GPIO_CAM_A2V85_EN,             
        CPLD_EXT_GPIO_FRONT_CAM_RST,            
        CPLD_EXT_GPIO_CAMIO_1V8_EN,             
        CPLD_EXT_GPIO_CAM_VCM2V85_EN=17,        
        CPLD_EXT_GPIO_CAM2_D1V2_EN,             
        CPLD_EXT_GPIO_CAM_PWDN,                 
        CPLD_EXT_GPIO_CAM1_VCM_PD,              
        CPLD_EXT_GPIO_RAW_RSTN,                 
        CPLD_EXT_GPIO_RAW_1V8_EN,               
        CPLD_EXT_GPIO_RAW_1V2_EN,               
        CPLD_EXT_GPIO_CAM_SEL,                  

        CPLD_EXT_GPIO_FLASH_EN=25,              
        CPLD_EXT_GPIO_TORCH_FLASH,              
        CPLD_EXT_GPIO_QFUSE_1V8_EN,
        CPLD_EXT_GPIO_AUD_A3254_RST,            
        CPLD_EXT_GPIO_AUD_I2S_SW_SEL,           
        CPLD_EXT_GPIO_AUD_SPK_RCV_SEL,          
        CPLD_EXT_GPIO_AUD_REV_EN,               
        CPLD_EXT_GPIO_AUD_MIC_SELECT,           
        CPLD_EXT_GPIO_AUD_HP_EN=33,             
        CPLD_EXT_GPIO_MIC_2V85_EN,              
        CPLD_EXT_GPIO_GPS_ON_OFF,               
        CPLD_EXT_GPIO_GPS_1V8_EN,               
        CPLD_EXT_GPIO_WIFI_SHUTDOWN,            
        CPLD_EXT_GPIO_BT_SHUTDOWN,              
        CPLD_EXT_GPIO_HW_CHG_LED_OFF,           
        CPLD_EXT_GPIO_CDMA_RUIM_GSM_SIM_SW,     

	CPLD_EXT_GPIO_EXT_USB_ID_OUTPUT,	
	CPLD_EXT_GPIO_EXT_GPS_LNA_EN = CPLD_EXT_GPIO_EXT_USB_ID_OUTPUT, 
	CPLD_EXT_GPIO_GPS_2V85_EN,              

	CPLD_GPI_START,
        CPLD_EXT_GPIO_USB_ID_INPUT_LEVEL = CPLD_GPI_START,       
        CPLD_EXT_GPIO_CHG_INT_INPUT_LEVEL,      
        CPLD_EXT_GPIO_CHG_STAT_INPUT_LEVEL,     
        CPLD_EXT_GPIO_KEY_UP_INPUT_LEVEL,	
        CPLD_EXT_GPIO_KEY_DW_INPUT_LEVEL,	
        CPLD_EXT_GPIO_AUD_HP_IN_INPUT_LEVEL,    
        CPLD_EXT_GPIO_REGION_ID_INPUT_LEVEL,    
        CPLD_BIT7_RESERVED,
        CPLD_GPI_END = CPLD_BIT7_RESERVED,
	CPLD_EXT_GPIO_MAX,
        CPLD_INT_MASK_USB_ID,
        CPLD_INT_MASK_CHG_INT,
        CPLD_INT_MASK_CHG_STAT,
        CPLD_INT_MASK_AUD_HP_IN,

        CPLD_INT_STATUS_USB_ID,
        CPLD_INT_STATUS_CHG_INT,
        CPLD_INT_STATUS_CHG_STAT,
        CPLD_INT_STATUS_AUD_HP_IN,
} cpld_gpio_pin_t;

int cpld_gpio_read(cpld_gpio_pin_t gpio_num);
int cpld_gpio_write(cpld_gpio_pin_t gpio_num, int value);
int cpld_gpio_config(cpld_gpio_pin_t gpio_num, cpld_dir_t dir);

#endif
