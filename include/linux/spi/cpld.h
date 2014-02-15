#ifndef __CPLD_H
#define __CPLD_H

struct cpld_platform_data {
	int intf_select;
	int cpld_power_pwd;
	void (*power_func)(int);
	void (*init_cpld_clk)(int);
	int (*clock_set)(int);
	int (*switch_interface)(int);
};


#define GPIO_SPI 0
#define EBI2_SPI 1

int spi_set_route(int path);

int cpld_open_init(void);
int cpld_release(void);


int gpio_spi_write(int length, unsigned char *buffer);

int gpio_spi_read(int length, unsigned char *buffer);

int cpld_spi_write(int length, unsigned char *buffer);

int cpld_spi_read(int length, unsigned char *buffer);

#endif
