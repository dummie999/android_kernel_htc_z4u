#ifndef CPLD_CHAR_H
#define CPLD_CHAR_H

extern uint8_t simulated_reg_int_st;
extern uint8_t stress_mode;

int cpld_char_init(void);
void cpld_char_exit(void);

#endif 
