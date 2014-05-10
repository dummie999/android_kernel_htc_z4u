#ifndef __ASM_ARM_SYSTEM_H
#define __ASM_ARM_SYSTEM_H

#ifdef __KERNEL__
#include <asm/barrier.h>
#include <asm/compiler.h>
#include <asm/cmpxchg.h>
#include <asm/exec.h>
#include <asm/switch_to.h>
#include <asm/system_info.h>
#include <asm/system_misc.h>

#define CPU_ARCH_UNKNOWN	0
#define CPU_ARCH_ARMv3		1
#define CPU_ARCH_ARMv4		2
#define CPU_ARCH_ARMv4T		3
#define CPU_ARCH_ARMv5		4
#define CPU_ARCH_ARMv5T		5
#define CPU_ARCH_ARMv5TE	6
#define CPU_ARCH_ARMv5TEJ	7
#define CPU_ARCH_ARMv6		8
#define CPU_ARCH_ARMv7		9

#define CR_M	(1 << 0)	
#define CR_A	(1 << 1)	
#define CR_C	(1 << 2)	
#define CR_W	(1 << 3)	
#define CR_P	(1 << 4)	
#define CR_D	(1 << 5)	
#define CR_L	(1 << 6)	
#define CR_B	(1 << 7)	
#define CR_S	(1 << 8)	
#define CR_R	(1 << 9)	
#define CR_F	(1 << 10)	
#define CR_Z	(1 << 11)	
#define CR_I	(1 << 12)	
#define CR_V	(1 << 13)	
#define CR_RR	(1 << 14)	
#define CR_L4	(1 << 15)	
#define CR_DT	(1 << 16)
#define CR_IT	(1 << 18)
#define CR_ST	(1 << 19)
#define CR_FI	(1 << 21)	
#define CR_U	(1 << 22)	
#define CR_XP	(1 << 23)	
#define CR_VE	(1 << 24)	
#define CR_EE	(1 << 25)	
#define CR_TRE	(1 << 28)	
#define CR_AFE	(1 << 29)	
#define CR_TE	(1 << 30)	

#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"

#ifndef __ASSEMBLY__

#include <linux/linkage.h>
#include <linux/irqflags.h>

#include <asm/outercache.h>

void cpu_idle_wait(void);

struct thread_info;
struct task_struct;

extern unsigned int system_rev;
extern unsigned int system_serial_low;
extern unsigned int system_serial_high;
extern unsigned int mem_fclk_21285;

struct pt_regs;

void die(const char *msg, struct pt_regs *regs, int err);

struct siginfo;
void arm_notify_die(const char *str, struct pt_regs *regs, struct siginfo *info,
		unsigned long err, unsigned long trap);

void hook_fault_code(int nr, int (*fn)(unsigned long, unsigned int,
				       struct pt_regs *),
		     int sig, int code, const char *name);

void hook_ifault_code(int nr, int (*fn)(unsigned long, unsigned int,
				       struct pt_regs *),
		     int sig, int code, const char *name);

#define xchg(ptr,x) \
	((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))

extern asmlinkage void c_backtrace(unsigned long fp, int pmode);

struct mm_struct;
extern void show_pte(struct mm_struct *mm, unsigned long addr);
extern void __show_regs(struct pt_regs *);

extern int cpu_architecture(void);
extern void cpu_init(void);

void arm_machine_restart(char mode, const char *cmd);
extern void (*arm_pm_restart)(char str, const char *cmd);

#define UDBG_UNDEFINED	(1 << 0)
#define UDBG_SYSCALL	(1 << 1)
#define UDBG_BADABORT	(1 << 2)
#define UDBG_SEGV	(1 << 3)
#define UDBG_BUS	(1 << 4)

extern unsigned int user_debug;

#if __LINUX_ARM_ARCH__ >= 4
#define vectors_high()	(cr_alignment & CR_V)
#else
#define vectors_high()	(0)
#endif

#if __LINUX_ARM_ARCH__ >= 7 ||		\
	(__LINUX_ARM_ARCH__ == 6 && defined(CONFIG_CPU_32v6K))
#define sev()	__asm__ __volatile__ ("sev" : : : "memory")
#define wfe()	__asm__ __volatile__ ("wfe" : : : "memory")
#define wfi()	__asm__ __volatile__ ("wfi" : : : "memory")
#endif

#if __LINUX_ARM_ARCH__ >= 7
#define isb() __asm__ __volatile__ ("isb" : : : "memory")
#define dsb() __asm__ __volatile__ ("dsb" : : : "memory")
#define dmb() __asm__ __volatile__ ("dmb" : : : "memory")
#elif defined(CONFIG_CPU_XSC3) || __LINUX_ARM_ARCH__ == 6
#define isb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 4" \
				    : : "r" (0) : "memory")
#define dsb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" \
				    : : "r" (0) : "memory")
#define dmb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" \
				: : "r" (0) : "memory")
#elif defined(CONFIG_CPU_FA526)
#define isb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 4" \
				    : : "r" (0) : "memory")
#define dsb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" \
				    : : "r" (0) : "memory")
#define dmb() __asm__ __volatile__ ("" : : : "memory")
#else
#define isb() __asm__ __volatile__ ("" : : : "memory")
#define dsb() __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" \
				    : : "r" (0) : "memory")
#define dmb() __asm__ __volatile__ ("" : : : "memory")
#endif

#ifdef CONFIG_ARCH_HAS_BARRIERS
#include <mach/barriers.h>
#elif defined(CONFIG_ARM_DMA_MEM_BUFFERABLE) || defined(CONFIG_SMP)
#define mb()		do { dsb(); outer_sync(); } while (0)
#define rmb()		dsb()
#define wmb()		mb()
#else
#include <asm/memory.h>
#define mb()	do { if (arch_is_coherent()) dmb(); else barrier(); } while (0)
#define rmb()	do { if (arch_is_coherent()) dmb(); else barrier(); } while (0)
#define wmb()	do { if (arch_is_coherent()) dmb(); else barrier(); } while (0)
#endif

#ifndef CONFIG_SMP
#define smp_mb()	barrier()
#define smp_rmb()	barrier()
#define smp_wmb()	barrier()
#else
#define smp_mb()	dmb()
#define smp_rmb()	dmb()
#define smp_wmb()	dmb()
#endif

#define read_barrier_depends()		do { } while(0)
#define smp_read_barrier_depends()	do { } while(0)

#define set_mb(var, value)	do { var = value; smp_mb(); } while (0)
#define nop() __asm__ __volatile__("mov\tr0,r0\t@ nop\n\t");

extern unsigned long cr_no_alignment;	
extern unsigned long cr_alignment;	

#ifndef __ASM_ARM_CP15_H
static inline unsigned int get_cr(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r" (val) : : "cc");
	return val;
}

static inline void set_cr(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR"
	  : : "r" (val) : "cc");
	isb();
}

#ifndef CONFIG_SMP
extern void adjust_cr(unsigned long mask, unsigned long set);
#endif

#define CPACC_FULL(n)		(3 << (n * 2))
#define CPACC_SVC(n)		(1 << (n * 2))
#define CPACC_DISABLE(n)	(0 << (n * 2))

static inline unsigned int get_copro_access(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c1, c0, 2 @ get copro access"
	  : "=r" (val) : : "cc");
	return val;
}

static inline void set_copro_access(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 2 @ set copro access"
	  : : "r" (val) : "cc");
	isb();
}
#endif 

#define __ARCH_WANT_INTERRUPTS_ON_CTXSW

extern struct task_struct *__switch_to(struct task_struct *, struct thread_info *, struct thread_info *);

#define switch_to(prev,next,last)					\
do {									\
	last = __switch_to(prev,task_thread_info(prev), task_thread_info(next));	\
} while (0)

#if defined(CONFIG_CPU_SA1100) || defined(CONFIG_CPU_SA110)
#define swp_is_buggy
#endif

extern void disable_hlt(void);
extern void enable_hlt(void);

void cpu_idle_wait(void);

#include <asm-generic/cmpxchg-local.h>

#endif 

#define arch_align_stack(x) (x)

#endif 

#endif
